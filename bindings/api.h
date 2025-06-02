#ifndef ADMETE_API_H
#define ADMETE_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Simple hello world function
int init();

int encode_features(char* fen, char* buffer, unsigned int buffer_size, char* move_after, int quiece);

#ifdef __cplusplus
}
#endif

#endif // ADMETE_API_H