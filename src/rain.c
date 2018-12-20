#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "config.h"
#include "rain.h"
#include "item.h"
#include "util.h"

void __update_rain_time(Rain* rain);
void __init_rain(Rain* rain);
GLuint gen_seq_buffer(Rain* rain);
GLuint gen_splash_buffer(Rain* rain);
GLuint gen_instance_buffer_line(Rain* rain, Chunk* chunk);
GLuint gen_instance_buffer_splash(Rain* rain, Chunk* chunk);
int __get_t_totoal_line(Rain* rain);
void __gen_instances(Rain* rain, int p, int q, int force);
int __is_neighbor(int p1, int q1, int p, int q);
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

float *__seq_buffer_rain_line(float len_rain_line, float interval_rain_line, int* len_seq){
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

float *__splash_buffer(int particle_cnt, float v_max){
    int sz_data = particle_cnt * 36;
    float *data = calloc(sz_data, sizeof(float));
    
    float phi = 2 * PI / (float)particle_cnt;
    float sin_phi = sinf(phi), cos_phi = cosf(phi);
    const float step_theta = 32.f, step_v = 64.f;
    const float n = 0.01f;
    const float quad_vert[6][3] = {
        {-1, 1, 0},
        {1, 1, 0},
        {1, -1, 0},
        {-1, -1, 0},
        {-1, 1, 0},
        {1, -1, 0}
    };
    float *data_orig = data;
    for(int i = 0; i < particle_cnt; ++i){
        float theta = (rand() % (int)step_theta) / step_theta * PI / 2;
        float v = (rand() % (int)step_v) / step_v * v_max;
        for(int j = 0; j < 6; ++j){
            *(data++) = cosf(i * phi) * cosf(theta) * v;
            *(data++) = sinf(theta) * v;
            *(data++) = sinf(i * phi) * cosf(theta) * v;
            *(data++) = n * quad_vert[j][0];
            *(data++) = n * quad_vert[j][1];
            *(data++) = n * quad_vert[j][2];
        }
    }
    return data_orig;
}

float *__instance_buffer_rain_splash(Chunk* chunk, int splash_pb, float t_max, float t_step, int* cnt_instance){
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
    int sz_data = CHUNK_SIZE * CHUNK_SIZE * splash_pb;
    if(cnt_instance){
        *cnt_instance = sz_data;
    }
    sz_data *= 5;
    float *data = calloc(sz_data, sizeof(float));
    const int step_translation = 70, step_angle = 50;
    int cnt_time_step = ceilf(t_max / t_step);
    float *data_orig = data;
    for(int x = 0; x < CHUNK_SIZE; ++x){
        for(int z = 0; z < CHUNK_SIZE; ++z){
            float vx = ox + x, vz = oz + z;
            float y = y_max[CHUNK_SIZE * x + z] + 0.5;
            for(int i = 0; i < splash_pb; ++i){
                *(data++) = vx + (rand() % step_translation) / (float)step_translation - 0.5f;
                *(data++) = y;
                *(data++) = vz + (rand() % step_translation) / (float)step_translation - 0.5f;
                *(data++) = (rand() % cnt_time_step) * t_step;
                *(data++) = (rand() % step_angle) / (float) step_angle * 2 * PI;
            }
        }
    }
    free(y_max);
    return data_orig;
}

void __draw_rain_splash(GLuint program, 
    GLuint vbo_splash, GLuint vbo_instance,
    int cnt_particle, int cnt_instance,
    float t, float t_max, float t_animation_max){
    glUseProgram(program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_splash);
    GLuint v = glGetAttribLocation(program, "v");
    GLuint pos_quad = glGetAttribLocation(program, "pos_quad");
    glEnableVertexAttribArray(v);
    glEnableVertexAttribArray(pos_quad);
    glVertexAttribPointer(v, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glVertexAttribPointer(pos_quad, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, vbo_instance);
    GLuint translation = glGetAttribLocation(program, "translation");
    GLuint t_init = glGetAttribLocation(program, "t_init");
    GLuint angle = glGetAttribLocation(program, "angle");
    glEnableVertexAttribArray(translation);
    glEnableVertexAttribArray(t_init);
    glEnableVertexAttribArray(angle);
    glVertexAttribPointer(translation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(t_init, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(angle, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(translation, 1);
    glVertexAttribDivisor(t_init, 1);
    glVertexAttribDivisor(angle, 1);

    glUniform1f(glGetUniformLocation(program, "t"), t);
    glUniform1f(glGetUniformLocation(program, "t_max"), t_max);
    glUniform1f(glGetUniformLocation(program, "g"), 8.f);
    glUniform1f(glGetUniformLocation(program, "t_animation_max"), t_animation_max);
    float mat_mvp[16];
    get_mvp_matrix(mat_mvp);
    glUniformMatrix4fv(glGetUniformLocation(program, "mat_mvp"), 1, GL_FALSE, mat_mvp);

    glDrawArraysInstanced(GL_TRIANGLES, 0, cnt_particle * 6, cnt_instance);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(v);
    glVertexAttribDivisor(translation, 0);
    glVertexAttribDivisor(t_init, 0);
    glVertexAttribDivisor(angle, 0);
    glDisableVertexAttribArray(translation);
    glDisableVertexAttribArray(t_init);
    glDisableVertexAttribArray(angle);
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

void __update_rain_time(Rain* rain){
    double t_current = glfwGetTime();
    double interval_line = t_current - rain->last_update;
    if(interval_line > rain->t_step_interval){
        ++rain->t_line;
        rain->t_line = rain->t_line >= __get_t_totoal_line(rain) ? 0 : rain->t_line;

        rain->t_splash += rain->t_step_interval;
        rain->t_splash = rain->t_splash > rain->t_max_splash ? 
            rain->t_splash - rain->t_max_splash : rain->t_splash;
        
        rain->last_update = t_current;
    }
}

int __is_neighbor(int p1, int q1, int p, int q){
    return abs(p - p1) <= 1 && abs(q - q1) <= 1;
}

void __gen_instances(Rain* rain, int p, int q, int force){
    if(force){
        int idx_instance = 0;
        for(int dp = -1; dp < 2; ++dp){
            for(int dq = -1; dq < 2; ++dq){
                Chunk* chunk = find_chunk(p + dp, q + dq);
                if(!chunk){
                    rain->instance[idx_instance].valid = 0;
                    continue;
                }
                rain->instance[idx_instance].p = p + dp;
                rain->instance[idx_instance].q = q + dq;
                rain->instance[idx_instance].vbo_instance_line = gen_instance_buffer_line(rain, chunk);
                rain->instance[idx_instance].vbo_instance_splash = gen_instance_buffer_splash(rain, chunk); 
                rain->instance[idx_instance].valid = 1;
                ++idx_instance;
            }
        }
    }
    else{
        if(rain->p == p && rain->q == q)
            return;
        RainInstance copy[9];
        int flags[9] = {0};
        for(int i = 0; i < 9; ++i){
            if(!__is_neighbor(rain->instance[i].p, rain->instance[i].q, p, q) && 
                rain->instance[i].valid){
                del_buffer(rain->instance[i].vbo_instance_line);
                del_buffer(rain->instance[i].vbo_instance_splash);
                rain->instance[i].valid = 0;
            }
            else{
                int cpy_idx = (rain->instance[i].p - p + 1) * 3 + (rain->instance[i].q - q + 1);
                copy[cpy_idx] = rain->instance[i];
                flags[cpy_idx] = 1;
            }
        }
        for(int i = 0; i < 9; ++i){
            if(!flags[i]){
                copy[i].p = i / 3 - 1 + p;
                copy[i].q = i % 3 - 1 + q;
                Chunk* chunk = find_chunk(copy[i].p, copy[i].q);
                if(chunk){
                    copy[i].valid = 1;
                    copy[i].vbo_instance_line = gen_instance_buffer_line(rain, chunk);
                    copy[i].vbo_instance_splash = gen_instance_buffer_splash(rain, chunk);
                }
                else{
                    copy[i].valid = 0;
                }
            }
            rain->instance[i] = copy[i];
        }
    }
    rain->p = p;
    rain->q = q;
}

void __init_rain(Rain* rain){
    rain->inited = 1;
    rain->activated = 1;
    
    rain->t_step_interval = 0.01f;
    rain->last_update = glfwGetTime();
    
    rain->cnt_seg_pb_line = 1;
    rain->len_rain_line = 0.9f;
    rain->interval_rain_line = 30.f;
    rain->t_line = 0;
    rain->v_line = 8.f;
    
    rain->cnt_particle = 20;
    rain->v_max_splash = 3.f;
    rain->splash_pb = 3;
    rain->t_splash = 0.f;
    rain->t_max_splash = 5.f;
    rain->t_animation_max_splash = 0.5f;

    rain->vbo_seq_line = gen_seq_buffer(rain);
    rain->vbo_splash = gen_splash_buffer(rain);
}

GLuint gen_seq_buffer(Rain* rain){
    float *data = __seq_buffer_rain_line(rain->len_rain_line, rain->interval_rain_line, &rain->len_seq_line);
    GLuint vbo = gen_buffer(rain->len_seq_line * 4 * sizeof(float), data);
    free(data);
    return vbo;
}

GLuint gen_splash_buffer(Rain* rain){
    float *data = __splash_buffer(rain->cnt_particle, rain->v_max_splash);
    GLuint vbo = gen_buffer(rain->cnt_particle * 36 * sizeof(float), data);
    free(data);
    return vbo;
}

GLuint gen_instance_buffer_line(Rain* rain, Chunk* chunk){
    float *data = __instance_buffer_rain_line(
        chunk, 
        rain->cnt_seg_pb_line,
        rain->len_rain_line,
        rain->interval_rain_line, 
        &rain->cnt_instance_line);
    GLuint vbo = gen_buffer(rain->cnt_instance_line * 4 * sizeof(float), data);
    free(data);
    return vbo;
}

GLuint gen_instance_buffer_splash(Rain* rain, Chunk* chunk){
    float *data = __instance_buffer_rain_splash(
        chunk, 
        rain->splash_pb,
        rain->t_max_splash,
        rain->t_step_interval,
        &rain->cnt_instance_splash
    );
    GLuint vbo = gen_buffer(rain->cnt_instance_splash * 5 * sizeof(float), data);
    free(data);
    return vbo;
}

int __get_t_totoal_line(Rain* rain){
    return ceilf(RAIN_MAX_HEIGHT / (rain->v_line * rain->t_step_interval));
}

void update_rain(Rain* rain, State* state){
    int p = chunked(state->x);
    int q = chunked(state->z);
    if(!rain->inited){
        __init_rain(rain);
        __gen_instances(rain, p, q, 1);
        return;
    }
    __gen_instances(rain, p, q, 0);
    if(rain->activated){
        __update_rain_time(rain);
    }
}

void render_rain(Rain* rain){
    if(!rain->activated)
        return;
    for(int i = 0; i < 9; ++i){
        if(rain->instance[i].valid){
            __draw_rain_line(rain->program_line, rain->vbo_seq_line, rain->instance[i].vbo_instance_line,
                rain->len_seq_line, rain->cnt_instance_line, rain->t_line, __get_t_totoal_line(rain));
            __draw_rain_splash(rain->program_splash, rain->vbo_splash, rain->instance[i].vbo_instance_splash,
                rain->cnt_particle, rain->cnt_instance_splash, rain->t_splash, rain->t_max_splash, rain->t_animation_max_splash);
        }
    }
}
