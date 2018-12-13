#ifndef MODEL_H
#define MODEL_H
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
#endif