#ifndef BLOCK_DESTROY
#define BLOCK_DESTROY
#include "model.h"
typedef struct {
    int dec;
    double duration;
    double start_stamp;
    int x;
    int y;
    int z;
} BlockDestroying;

extern void update_destroying_block(double d_w, double d_r, BlockDestroying *block_destroying, State *state);
#endif