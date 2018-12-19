#ifndef RAIN_H
#define RAIN_H
#include "model.h"
#define RAIN_SEQ_SEQ_CNT 1
#define RAIN_MAX_HEIGHT 200

typedef struct {
    int inited;
    int p, q;
    int cnt_instance;
    int len_seq;
    double last_update;
    int t;
    int t_total;
    GLuint vbo_seq, vbo_instance;
    GLuint program_rain_line;
} Rain;

extern void update_rain(Rain* rain, State* state);
extern void render_rain(Rain* rain);

#endif