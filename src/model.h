#ifndef MODEL_H
#define MODEL_H

#include "map.h"
#include "sign.h"
#include <GL/glew.h>
typedef struct {
    Map map;
    Map lights;
    SignList signs;
    int p;
    int q;
    int faces;
    int sign_faces;
    int dirty;
    int miny;
    int maxy;
    GLuint buffer;
    GLuint sign_buffer;
} Chunk;

typedef struct {
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float t;
} State;

extern int hit_test(
    int previous, float x, float y, float z, float rx, float ry,
    int *bx, int *by, int *bz);
extern void set_block(int x, int y, int z, int w);
extern void record_block(int x, int y, int z, int w);
extern int get_block(int x, int y, int z);
extern void get_mvp_matrix(float *mat);
extern int chunked(float x);
extern Chunk *find_chunk(int p, int q);
extern int highest_block(float x, float z);
#endif