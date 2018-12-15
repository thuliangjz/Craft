#ifndef BLOCK_DESTROY
#define BLOCK_DESTROY
#include "model.h"
#include <GL/glew.h>
typedef struct {
    int dec;
    double duration;
    double start_stamp;
    int x;
    int y;
    int z;
    int w;
    int level_destruction;
    GLuint program;
} BlockDestroying;

extern void update_destroying_block(double d_w, double d_r, BlockDestroying *block_destroying, State *state);
extern void gen_destroy_texture();
extern void render_destroy_texture(BlockDestroying* block_destroying);
#endif