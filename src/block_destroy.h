#ifndef BLOCK_DESTROY
#define BLOCK_DESTROY
#include "model.h"
#include <GL/glew.h>

#define PARTICLE_SEG_CNT 5
#define DESTROY_TEXTURE_SIZE 128
#define DESTROY_TEXTURE_LEVEL 4
#define DESTROY_MASK 150
#define PARTICLE_DURATION 3

typedef struct {
    float v[3];    //粒子速度,时间尺度为秒，注意glfwGetTime也是以秒为单位的
    float p[3];    //粒子位置
}DestroyParticle;


typedef struct {
    int active;
    double start_stamp;
    double last_update;
    int w;
    DestroyParticle particles[PARTICLE_SEG_CNT * PARTICLE_SEG_CNT * PARTICLE_SEG_CNT];
    Chunk *chunks[9];   //保存发射点附近的3*3的chunk地图,直接调用find_chunk开销太大
    GLuint program;
}DestroyEmitter;

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
    DestroyEmitter emitter;
} BlockDestroying;


extern void update_destroying_block(double d_w, double d_r, BlockDestroying *block_destroying, State *state);
extern void gen_destroy_texture();
extern void render_destroy_texture(BlockDestroying* block_destroying);

extern void init_destroy_particle(DestroyEmitter* emitter, int x, int y, int z, int w);
extern void update_destroy_particle(DestroyEmitter* emitter);
extern void render_destroy_particle(DestroyEmitter* emitter);
#endif