#include "types.hpp"
#include <string>

std::string print_score(const score_t score);
namespace Printing {
std::string piece_name(const PieceType);
std::string side_name(const CastlingSide p);
}