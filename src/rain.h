#ifndef RAIN_H
#define RAIN_H
#include "model.h"
#define RAIN_SEQ_SEQ_CNT 1
#define RAIN_MAX_HEIGHT 200

typedef struct {
    int inited;
    int p, q;

    int cnt_instance_line;
    int len_seq_line;
    int t_line;
    int t_total_line;

    float t_splash;
    float t_animation_max_splash;
    float t_max_splash;
    int cnt_particle;
    float v_max_splash;
    int cnt_instance_splash;

    float t_step_interval;
    double last_update;

    GLuint vbo_seq_line, vbo_instance_line;
    GLuint vbo_splash, vbo_instance_splash;
    GLuint program_line;
    GLuint program_splash;
} Rain;

extern void update_rain(Rain* rain, State* state);
extern void render_rain(Rain* rain);

#endif