#ifndef RAIN_H
#define RAIN_H
#include "model.h"
#define RAIN_SEQ_SEQ_CNT 1
#define RAIN_MAX_HEIGHT 200

typedef struct {
    int p, q;
    GLuint vbo_instance_line;
    GLuint vbo_instance_splash;
    int valid;
}RainInstance;

typedef struct {
    int activated;
    int inited;
    int p, q;

    int cnt_seg_pb_line;
    float len_rain_line, interval_rain_line;
    int cnt_instance_line;
    int len_seq_line;   //记录模型大小
    int t_line;
    float v_line;

    int cnt_particle;   //同样用于记录模型的大小
    float v_max_splash;
    int splash_pb;
    int cnt_instance_splash;
    float t_splash;
    float t_animation_max_splash;
    float t_max_splash;

    float t_step_interval;
    double last_update;

    float control;

    GLuint vbo_seq_line;
    GLuint vbo_splash;
    GLuint program_line;
    GLuint program_splash;

    RainInstance instance[9];   //始终保证玩家所处的chunk和紧邻周围的chunk的绘制
} Rain;

extern void update_rain(Rain* rain, State* state);
extern void render_rain(Rain* rain);
extern void invalidate_rain(Rain* rain, int x, int y, int z, int w);
extern void control_rain(Rain* rain, int direction);
#endif