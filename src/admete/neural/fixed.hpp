/* Zero-overhead fixed point arithmetic implementation
*/
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <cmath>

namespace Neural {

template<std::size_t N, bool Signed = true>
struct smallest_int_with_bits {
private:
    // Try different integer types
    template<typename T>
    static constexpr bool has_enough_bits =
        std::numeric_limits<T>::digits + (Signed ? 1 : 0) >= N;
public:
    using type = std::conditional_t<has_enough_bits<int8_t>, int8_t,
                std::conditional_t<has_enough_bits<int16_t>, int16_t,
                std::conditional_t<has_enough_bits<int32_t>, int32_t,
                std::conditional_t<has_enough_bits<int64_t>, int64_t,
                void>>>>;
};

// Specialization for unsigned types
template<std::size_t N>
struct smallest_int_with_bits<N, false> {
private:
    template<typename T>
    static constexpr bool has_enough_bits =
        std::numeric_limits<T>::digits >= N;
public:
    using type = std::conditional_t<has_enough_bits<uint8_t>, uint8_t,
                std::conditional_t<has_enough_bits<uint16_t>, uint16_t,
                std::conditional_t<has_enough_bits<uint32_t>, uint32_t,
                std::conditional_t<has_enough_bits<uint64_t>, uint64_t,
                void>>>>;
};

// Helper constexpr function to handle both positive and negative scale shifts

template <typename T>
constexpr T invert_scale_shift(T value, int8_t shift, uint8_t bits) {
    // From internal fixed point representation to floating point
    static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
    auto total_shift = shift - bits;
    if (total_shift > 0) {
        return value * T(1 << total_shift);
    } else if (total_shift < 0) {
        return value / T(1 << (-total_shift));
    } else {
        return value;
    }
}


template <typename T>
constexpr T apply_scale_shift(T value, int8_t shift, uint8_t bits) {
    // From floating point to internal fixed point representation
    static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
    auto total_shift = shift - bits;
    if (total_shift > 0) {
        return value / T(1 << total_shift);
    } else if (total_shift < 0) {
        return value * T(1 << (-total_shift));
    } else {
        return value;
    }
}

template <uint8_t bits, int8_t scale_shift>
class Fixed {
public:
    using int_type = typename smallest_int_with_bits<bits, true>::type;
    
private:
    int_type value;
    // The represented value == value * (2^scale_shift) / (2^bits) == value * 2^(scale_shift - bits)
    
public:
    Fixed() : value(0) {}
    Fixed(int_type v) : value(v) {}
    
    // Fixed-point from floating point
    Fixed(float v) : value(static_cast<int_type>(apply_scale_shift(v, scale_shift, bits))) {}
    Fixed(double v) : value(static_cast<int_type>(apply_scale_shift(v, scale_shift, bits))) {}
    // Promoting from smaller fixed point types
    template<uint8_t other_bits>
    Fixed(const Fixed<other_bits, scale_shift>& other) {
        value = static_cast<int_type>(other.raw_value());
    }
    
    template<typename T>
    T as() const {
        static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
        return invert_scale_shift(static_cast<T>(value), scale_shift, bits);
    }
    
    Fixed operator+(const Fixed& other) const {
        return Fixed(value + other.value);
    }
    
    Fixed operator-(const Fixed& other) const {
        return Fixed(value - other.value);
    }

    // increment and decrement operators
    Fixed& operator+=(const Fixed& other) {
        value += other.value;
        return *this;
    }
    Fixed& operator-=(const Fixed& other) {
        value -= other.value;
        return *this;
    }
    Fixed& operator++() {
        value++;
        return *this;
    }
    Fixed& operator--() {
        value--;
        return *this;
    }
    
    // Static multiplication method that creates a Fixed point of this type
    template <uint8_t lhs_bits, int8_t lhs_scale_shift,
              uint8_t rhs_bits, int8_t rhs_scale_shift>
    static Fixed multiply(const Fixed<lhs_bits, lhs_scale_shift>& lhs,
                          const Fixed<rhs_bits, rhs_scale_shift>& rhs) {
        static_assert(lhs_scale_shift + rhs_scale_shift == scale_shift,
                     "Scale shift mismatch in Fixed::multiply");
                     
        using intermediate_type = typename smallest_int_with_bits
            <bits + std::max(lhs_bits, rhs_bits), true>::type;
            
        // Perform multiplication with wider intermediate type
        intermediate_type result = static_cast<intermediate_type>(lhs.raw_value()) * 
                                   static_cast<intermediate_type>(rhs.raw_value());
                                   
        return Fixed(static_cast<int_type>(result));
    }

    // Integer multiplicationm without changing the result type
    template<typename T>
    Fixed small_multiply(const T other) const {
        static_assert(std::is_integral<T>::value, "T must be an integral type");
        return Fixed(value * other);
    }
    
    // Make the raw value accessible for operations
    int_type raw_value() const { return value; }
};

template <uint8_t out_bits,
          uint8_t lhs_bits, int8_t lhs_scale_shift,
          uint8_t rhs_bits, int8_t rhs_scale_shift>
Fixed<out_bits, lhs_scale_shift + rhs_scale_shift> fixed_mul( Fixed<lhs_bits, lhs_scale_shift> lhs,
    Fixed<rhs_bits, rhs_scale_shift> rhs) {
      return Fixed<out_bits, lhs_scale_shift + rhs_scale_shift>::multiply(lhs, rhs);
};

} // namespace Neural