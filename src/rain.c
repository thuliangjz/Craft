#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "config.h"
#include "rain.h"
#include "item.h"
#include "util.h"

float *__instance_buffer_rain_line(Chunk* chunk, int cnt_seg_pb, 
    float len_rain_line, float interval_rain_line,
    int *cnt_instance){
        int p = chunk->p , q = chunk->q;
    Map* map = &chunk->map;
    int ox = p * CHUNK_SIZE, oz = q * CHUNK_SIZE;
    int *y_max = calloc(CHUNK_SIZE * CHUNK_SIZE, sizeof(int));
    memset(y_max, 0, CHUNK_SIZE * CHUNK_SIZE * sizeof(int));
    MAP_FOR_EACH(map, ex, ey, ez, ew){
        int x = ex - ox;
        int z = ez - oz;
        if(x < 0 || z < 0 || x >= CHUNK_SIZE || z >= CHUNK_SIZE)    //map中可能保存了来自其他chunk中的元素,如果不做处理可能导致free异常
            continue;
        if(is_obstacle(ew))
            y_max[x * CHUNK_SIZE + z] = MAX(y_max[x * CHUNK_SIZE + z], ey);
    }END_MAP_FOR_EACH;
    int sz_data = cnt_seg_pb * cnt_seg_pb * CHUNK_SIZE * CHUNK_SIZE;
    *cnt_instance = sz_data;
    sz_data *= 4;   //每个instance包含4个float
    float *data = calloc(sz_data, sizeof(float));
    float *data_head = data;
    float delta_instance = 1.f / cnt_seg_pb;
    const int rand_step = 128;
    const float rand_step_f = rand_step;
    for(int x = 0; x < CHUNK_SIZE; ++x){
        for(int z = 0; z < CHUNK_SIZE; ++z){
            float y_min = y_max[x * CHUNK_SIZE + z] + 0.5 - len_rain_line;
            float v_x = ox + x, v_z = oz + z;
            for(int seq_i = 0; seq_i < cnt_seg_pb; ++seq_i){
                for(int seq_j = 0; seq_j < cnt_seg_pb; ++seq_j){
                    *(data++) = v_x + ((rand() % rand_step) / rand_step_f - 0.5) * delta_instance;
                    *(data++) = (rand() % rand_step) / rand_step_f * interval_rain_line;
                    *(data++) = v_z + ((rand() % rand_step) / rand_step_f - 0.5) * delta_instance;
                    *(data++) = y_min;

                    v_z += delta_instance;
                }
                v_x += delta_instance;
            }
        }
    }
    free(y_max);
    return data_head;
}

float *seq_buffer_rain_line(float len_rain_line, float interval_rain_line, int* len_seq){
    int sz_data = ceilf(RAIN_MAX_HEIGHT / interval_rain_line);
    *len_seq = sz_data;
    float *data = calloc(sz_data * 4, sizeof(float));
    float *data_head = data;
    float y = 0;
    //一条线的两个顶点包含两个属性：头的位置和相对位置,第一个对应rain_line_head, 第二个对应rain_line_y
    for(int i = 0; i < sz_data; ++i){
        *(data++) = y;
        *(data++) = 0;
        *(data++) = y;
        *(data++) = len_rain_line;
        y += interval_rain_line;
    }
    return data_head;
}

void __draw_rain_line(GLuint program, 
    GLuint vbo_seq, GLuint vbo_instance,
    int len_seq, int cnt_instance, 
    int t, int t_total){
    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_seq);
    GLuint rain_line_y = glGetAttribLocation(program, "rain_line_y");
    GLuint rain_line_head = glGetAttribLocation(program, "rain_line_head");
    glEnableVertexAttribArray(rain_line_y);
    glEnableVertexAttribArray(rain_line_head);
    glVertexAttribPointer(rain_line_y, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(float) * 1));
    glVertexAttribPointer(rain_line_head, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(0));

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    GLuint y_min = glGetAttribLocation(program, "y_min"),
        seq_pos = glGetAttribLocation(program, "seq_pos");
    glEnableVertexAttribArray(y_min);
    glEnableVertexAttribArray(seq_pos);
    glVertexAttribPointer(seq_pos, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(y_min, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(seq_pos, 1);
    glVertexAttribDivisor(y_min, 1);

    glUniform1f(glGetUniformLocation(program, "t"), t);
    glUniform1f(glGetUniformLocation(program, "t_total"), t_total);
    glUniform1f(glGetUniformLocation(program, "d_max"), (float)RAIN_MAX_HEIGHT);
    float mat_mvp[16];
    get_mvp_matrix(mat_mvp);
    glUniformMatrix4fv(glGetUniformLocation(program, "mat_mvp"), 1, GL_FALSE, mat_mvp);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glLineWidth(2.f);
    glDrawArraysInstanced(GL_LINES, 0, len_seq * 2, cnt_instance);

    glVertexAttribDivisor(rain_line_head, 0);
    glVertexAttribDivisor(y_min, 0);
    glVertexAttribDivisor(seq_pos, 0);
    glDisableVertexAttribArray(rain_line_head);
    glDisableVertexAttribArray(rain_line_y);
    glDisableVertexAttribArray(y_min);
    glDisableVertexAttribArray(seq_pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void render_rain(Rain* rain){
    __draw_rain_line(rain->program_rain_line, rain->vbo_seq, rain->vbo_instance,
        rain->len_seq, rain->cnt_instance, rain->t, rain->t_total);
}

void __update_rain_time(Rain* rain, double t_step_interval){
    double t_current = glfwGetTime();
    double interval = t_current - rain->last_update;
    if(interval > t_step_interval){
        ++rain->t;
        rain->t = rain->t >= rain->t_total ? 0 : rain->t;
        rain->last_update = t_current;
    }
}

void update_rain(Rain* rain, State* state){
    int p = chunked(state->x);
    int q = chunked(state->z);
    const float len_rain_line = 0.9f, interval_rain_line = 10.f;
    const float v_rain = 5.f, t_step_interval = 0.01;
    const int cnt_seg_pb = 1;

    if(rain->inited && p == rain->p && q == rain->q){
        __update_rain_time(rain, t_step_interval);
        return;
    }
    if(rain->inited){
        del_buffer(rain->vbo_instance);
    }
    if(!rain->inited){
        rain->inited = 1;
        rain->t_total = ceilf(RAIN_MAX_HEIGHT / (v_rain * t_step_interval));
        rain->t = 0;
        float *data_seq = seq_buffer_rain_line(len_rain_line, interval_rain_line, &rain->len_seq);
        rain->vbo_seq = gen_buffer(rain->len_seq * sizeof(float) * 4, data_seq);
        free(data_seq);
    }
    //生成vbo_instance
    Chunk *chunk = find_chunk(p, q);
    float *data_instance = __instance_buffer_rain_line(chunk, cnt_seg_pb, len_rain_line, interval_rain_line, &rain->cnt_instance);
    rain->vbo_instance = gen_buffer(rain->cnt_instance * 4 * sizeof(float), data_instance);
    free(data_instance);
    rain->p = p; rain->q = q;
}