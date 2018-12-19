#include "block_destroy.h"
#include "item.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "util.h"
#include <string.h>
#include "cube.h"
void __particle_collision_update(DestroyEmitter* emitter, int idx_particle, float dt);

void update_destroying_block(double d_w, double d_r, BlockDestroying *block_destroying, State *state){
    if(!block_destroying->dec)
        return;
    if(d_w > 0 || d_r > 0){
        State *s = state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if(hw == 0){
            block_destroying->dec = 0;
            return;
        }
        if(hx != block_destroying->x || hy != block_destroying->y || hz != block_destroying->z){
            block_destroying->start_stamp = glfwGetTime();
            block_destroying->duration = get_destroy_duration(hw);
            block_destroying->x = hx;
            block_destroying->y = hy;
            block_destroying->z = hz;
            block_destroying->w = hw;
            return;
        }
    }
    double d = glfwGetTime() - block_destroying->start_stamp;
    block_destroying->level_destruction = (int)(d / block_destroying->duration * DESTROY_TEXTURE_LEVEL);
    block_destroying->level_destruction = 
        block_destroying->level_destruction > DESTROY_TEXTURE_LEVEL - 1 ? DESTROY_TEXTURE_LEVEL - 1 : block_destroying->level_destruction;
    if(d >= block_destroying->duration){
        set_block(block_destroying->x, block_destroying->y, block_destroying->z, 0);
        record_block(block_destroying->x, block_destroying->y, block_destroying->z, 0);
        if(is_plant(get_block(block_destroying->x, block_destroying->y + 1, block_destroying->z))){
            set_block(block_destroying->x, block_destroying->y + 1, block_destroying->z, 0);
        }
        init_destroy_particle(&block_destroying->emitter, 
            block_destroying->x, block_destroying->y, block_destroying->z,
            block_destroying->w);
        //继续考察是否有方块被摧毁
        State *s = state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if(hw == 0){
            block_destroying->dec = 0;
            return;
        }
        block_destroying->start_stamp = glfwGetTime();
        block_destroying->duration = get_destroy_duration(hw);
        block_destroying->x = hx;
        block_destroying->y = hy;
        block_destroying->z = hz;
        block_destroying->w = hw;
    }
}

void gen_destroy_texture(){
    char *data = (char*)calloc(DESTROY_TEXTURE_LEVEL * DESTROY_TEXTURE_SIZE 
                                * DESTROY_TEXTURE_SIZE, sizeof(char));
    char ***table = (char***)calloc(DESTROY_TEXTURE_LEVEL, sizeof(char**));
    int sz = 2;
    for(int i = 0; i < DESTROY_TEXTURE_LEVEL; ++i){
        table[i] = (char**)calloc(sz, sizeof(char*));
        for(int j = 0; j < sz; ++j){
            table[i][j] = (char*)calloc(sz, sizeof(char));
        }
        sz *= 2;
    }
    sz = 2;
    for(int i = 0; i < DESTROY_TEXTURE_LEVEL; ++i){
        for(int j = 0; j < sz; ++j){
            for(int k = 0; k < sz; ++k){
                table[i][j][k] = (255 - DESTROY_MASK) +  (char)(rand() % DESTROY_MASK);
            }
        }
        sz *= 2;
    }

    for(int i = 0 ; i < DESTROY_TEXTURE_SIZE; ++i){
        sz = 2;
        for(int level = 0; level < DESTROY_TEXTURE_LEVEL; ++level){
            for(int t = 0; t < DESTROY_TEXTURE_SIZE; ++t){
                int j = level * DESTROY_TEXTURE_SIZE + t;
                int level_i = i * sz / DESTROY_TEXTURE_SIZE;
                int level_j = t * sz / DESTROY_TEXTURE_SIZE;
                data[i * DESTROY_TEXTURE_SIZE * DESTROY_TEXTURE_LEVEL + j] = table[level][level_i][level_j];
                //data[i * DESTROY_TEXTURE_SIZE * DESTROY_TEXTURE_LEVEL + j] = 255;
            }
            sz *= 2;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, DESTROY_TEXTURE_SIZE * DESTROY_TEXTURE_LEVEL, 
        DESTROY_TEXTURE_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    sz = 2;
    for(int i = 0; i < DESTROY_TEXTURE_LEVEL; ++i){
        for(int j = 0; j < sz; ++j){
            free(table[i][j]);
        }
        sz *= 2;
        free(table[i]);
    }
    free(table);
    free(data);
}

void render_destroy_texture(BlockDestroying* block_destroying){
    if(!block_destroying->dec)
        return;
    //准备vbo, 包括position(3), uv_texture(2), uv_weight(2)
    float mat_mvp[16] = {0};
    get_mvp_matrix(mat_mvp);
    float ao[6][4] = {0};
    float light[6][4] = {0};
    int sz_vbo = 7 * 36, stride = 7;
    float *buffer = calloc(sz_vbo, sizeof(float));
    float *buffer_cube = calloc(360, sizeof(float));
    memset(buffer, 0, sz_vbo * sizeof(float));
    memset(buffer_cube, 0, 360 * sizeof(float));
    make_cube(buffer_cube, ao, light, 
        1, 1, 1, 1, 1, 1,   //6个面都可见 
        block_destroying->x, block_destroying->y, block_destroying->z, 
        0.5f, block_destroying->w);
    float displace = 1.f / DESTROY_TEXTURE_LEVEL;
    for(int i = 0; i < 6; ++i){
        //遍历6个面
        for(int j = 0; j < 6; ++j){
            //每个面上有6个点
            int idx = i * 6 + j;
            int bs = idx * 7, bcs = idx * 10;
            buffer[bs] = buffer_cube[bcs];
            buffer[bs + 1] = buffer_cube[bcs + 1];
            buffer[bs + 2] = buffer_cube[bcs + 2];
            buffer[bs + 3] = buffer_cube[bcs + 6];
            buffer[bs + 4] = buffer_cube[bcs + 7];
            switch(j){
            case 0:
            case 3:
                buffer[bs + 5] = displace * block_destroying->level_destruction;
                buffer[bs + 6] = 0.f;
                break;
            case 1:
            case 5:
                buffer[bs + 5] = (block_destroying->level_destruction + 1) * displace;
                buffer[bs + 6] = 1.f;
                break;
            case 2:
                buffer[bs + 5] = displace * block_destroying->level_destruction;
                buffer[bs + 6] = 1.f;
                break;
            case 4:
                buffer[bs + 5] = (block_destroying->level_destruction + 1) * displace;
                buffer[bs + 6] = 0.f;
            }
        }
    }
    glUseProgram(block_destroying->program);
    //生成vbo
    GLuint gl_buffer = gen_buffer(sz_vbo * sizeof(float), buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
    GLuint position, uv_texture, uv_weight;
    position = glGetAttribLocation(block_destroying->program, "position");
    uv_texture = glGetAttribLocation(block_destroying->program, "uv_texture");
    uv_weight = glGetAttribLocation(block_destroying->program, "uv_weight");
    glEnableVertexAttribArray(position);
    glEnableVertexAttribArray(uv_texture);
    glEnableVertexAttribArray(uv_weight);
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * stride, (void*)0);
    glVertexAttribPointer(uv_texture, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * stride, (void*)(3 * sizeof(GL_FLOAT)));
    glVertexAttribPointer(uv_weight, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * stride, (void*)(5 * sizeof(GL_FLOAT)));

    glUniform1i(glGetUniformLocation(block_destroying->program, "sampler_texture"), 0);
    glUniform1i(glGetUniformLocation(block_destroying->program, "sampler_weight"), 4);
    glUniformMatrix4fv(glGetUniformLocation(block_destroying->program, "matrix_mvp"), 1, GL_FALSE, mat_mvp);
    
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(uv_texture);
    glDisableVertexAttribArray(uv_weight);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    del_buffer(gl_buffer);
    free(buffer);
    free(buffer_cube);    
}

void init_destroy_particle(DestroyEmitter* emitter, int x, int y, int z, int w){
    emitter->active = 1;
    int idx = 0;
    float rand_max = 1.f;
    const int rand_interval = 10;
    //游戏中的方块都是以整数点作为中心，1作为边长
    const float step = 1.f / PARTICLE_SEG_CNT;
    float corner_center[3] = {x - 0.5 + 0.5 * step, y - 0.5 + 0.5 * step, z - 0.5 + 0.5 * step};
    float center[3] = {x, y, z};
    float d = 2;
    for(int i = 0; i < PARTICLE_SEG_CNT; ++i){
        for(int j = 0; j < PARTICLE_SEG_CNT; ++j){
            for(int k = 0; k < PARTICLE_SEG_CNT; ++k){
                emitter->particles[idx].p[0] = corner_center[0] + step * i;
                emitter->particles[idx].p[1] = corner_center[1] + step * j;
                emitter->particles[idx].p[2] = corner_center[2] + step * k;
                for(int l = 0; l < 3; ++l){
                    emitter->particles[idx].v[l] = (emitter->particles[idx].p[l] - center[l]) * d + 
                        ((rand() % rand_interval) - rand_interval / 2) / (float)rand_interval * rand_max;
                }
                ++idx;
            }
        }
    }
    int p = chunked(x);
    int q = chunked(z);
    for(int i = -1; i < 2; ++i){
        for(int j = -1; j < 2; ++j){
            emitter->chunks[(i + 1) * 3 + j + 1] = find_chunk(p + i, q + j);
        }
    }
    emitter->start_stamp = glfwGetTime();
    emitter->last_update = emitter->start_stamp;
    emitter->w = w;
}

void linear_v_dec(float *v, float a, float dt){
    float vt = *v;
    if(fabsf(vt) < 1e-5)
        return;
    int sgn = vt > 0 ? 1 : -1;
    vt += -sgn * a * dt;
    int sgn1 = vt > 0 ? 1 : -1;
    vt = sgn1 == sgn ? vt : 0;
    *v = vt;
}

void update_destroy_particle(DestroyEmitter* emitter){
    if(!emitter->active)
        return;
    const int particle_count =  PARTICLE_SEG_CNT * PARTICLE_SEG_CNT * PARTICLE_SEG_CNT;
    double t_current = glfwGetTime();
    if(t_current - emitter->start_stamp > PARTICLE_DURATION){
        emitter->active = 0;
        return;
    }
    if(t_current - emitter->last_update < 0.01) //控制刷新速率
        return;
    float dt = t_current - emitter->last_update;
    for(int i = 0; i < particle_count; ++i){
        __particle_collision_update(emitter, i, dt);
        emitter->particles[i].v[1] -= (5.f + emitter->particles[i].v[1] * 0.5) * dt;
        linear_v_dec(&emitter->particles[i].v[0], 0.5, dt);
        linear_v_dec(&emitter->particles[i].v[2], 0.5, dt);
    }
//    for(int i = 0; i < particle_count; ++i){
//        for(int j = 0; j < 3; ++j){
//            emitter->particles[i].p[j] += emitter->particles[i].v[j] * dt;
//        }
//    }
    emitter->last_update = glfwGetTime();
}

void __particle_collision_update(DestroyEmitter* emitter, int idx_particle, float dt){
    //精简版find_chunk
    int p = chunked(emitter->particles[idx_particle].p[0]);
    int q = chunked(emitter->particles[idx_particle].p[2]);
    Chunk* chunk = 0;
    for(int i = 0; i < 9; ++i){
        if(emitter->chunks[i] == 0)
            continue;
        if(emitter->chunks[i]->p == p && emitter->chunks[i]->q == q){
            chunk = emitter->chunks[i];
            break;
        }
    }
    if(!chunk){
        printf("error\n");
        return;
    }
    Map *map = &chunk->map;
    //计算增量位移
    const float max_step = 1.f / PARTICLE_SEG_CNT * 0.9;  //最大的移动距离
    DestroyParticle *particle = &emitter->particles[idx_particle];
    int cnt_step = 0;
    for(int i = 0 ; i < 3; ++i){
        int cnt_tmp = ceilf(fabsf(particle->v[i]) * dt / max_step);
        cnt_step = cnt_step > cnt_tmp ? cnt_step : cnt_tmp;
    }
    float step[3] = {0};
    for(int i = 0; i < 3; ++i){
        step[i] = particle->v[i] * dt / cnt_step;
    }
    //增量更新并进行碰撞检测
    static const float vertex_pos_rel[8][3] = {
        {-1, -1, -1},
        {-1, -1, 1},
        {-1, 1, -1},
        {-1, 1, 1},
        {1, -1, -1},
        {1, -1, 1},
        {1, 1, -1},
        {1, 1, 1}
    };
    static const float n = (1.f / PARTICLE_SEG_CNT) * 0.5f;
    for(int i = 0 ; i < cnt_step; ++i){
        for(int j = 0; j < 3; ++j){
            if(fabsf(step[j]) < 1e-5f)
                continue;
            particle->p[j] += step[j];
            for(int k = 0; k < 8; ++k){
                float pos_vertex[3];
                for(int l = 0; l < 3; ++l){
                    pos_vertex[l] = particle->p[l] + vertex_pos_rel[k][l] * n;
                }
                int center[3] = {roundf(pos_vertex[0]), roundf(pos_vertex[1]), roundf(pos_vertex[2])};
                if(!is_obstacle(map_get(map, center[0], center[1], center[2])))
                    continue;
                //发生碰撞
                particle->p[j] = step[j] > 0 ? center[j] - 0.5 - n : center[j] + 0.5 + n;
                step[j] = 0;
                particle->v[j] = 0;
                break;
            }
        }
    }
}

void render_destroy_particle(DestroyEmitter* emitter){
    //假设已经使用了绘制的program
    if(!emitter->active)
        return;
    float mat_mvp[16] = {0};
    get_mvp_matrix(mat_mvp);
    const int count_particle = PARTICLE_SEG_CNT * PARTICLE_SEG_CNT * PARTICLE_SEG_CNT;
    const int step1 = 5, step2 = 10;
    float ao[6][4] = {0};
    float light[6][4] = {0};

    float *b1 = calloc(count_particle * 36 * step1, sizeof(float));
    float *b2 = calloc(count_particle * 36 * step2, sizeof(float));
    int p1 = 0, p2 = 0;
    for(int i = 0; i < count_particle; ++i){
        make_cube(b2 + p2, ao, light, 1, 1, 1, 1, 1, 1, 
            emitter->particles[i].p[0], 
            emitter->particles[i].p[1],
            emitter->particles[i].p[2], 
            1.f / PARTICLE_SEG_CNT * 0.5, emitter->w);
        for(int j = 0; j < 36; ++j){
            b1[p1] = b2[p2];
            b1[p1 + 1] = b2[p2 + 1];
            b1[p1 + 2] = b2[p2 + 2];
            b1[p1 + 3] = b2[p2 + 6];
            b1[p1 + 4] = b2[p2 + 7];
            p1 += step1;
            p2 += step2;
        }
    }
    glUseProgram(emitter->program);
    GLuint b_g = gen_buffer(count_particle * 36 * step1 * sizeof(float), b1);
    glBindBuffer(GL_ARRAY_BUFFER, b_g);
    GLuint pos, uv;
    pos = glGetAttribLocation(emitter->program, "position");
    uv = glGetAttribLocation(emitter->program, "uv");
    glEnableVertexAttribArray(pos);
    glEnableVertexAttribArray(uv);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * step1, (void*)0);
    glVertexAttribPointer(uv, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * step1, (void*)(3 * sizeof(GL_FLOAT)));
    glUniform1i(glGetUniformLocation(emitter->program, "sampler"), 0);
    glUniformMatrix4fv(glGetUniformLocation(emitter->program, "mat_mvp"), 1, GL_FALSE, mat_mvp);
    glDrawArrays(GL_TRIANGLES, 0, count_particle * 36);

    glDisableVertexAttribArray(pos);
    glDisableVertexAttribArray(uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    del_buffer(b_g);
    free(b1);
    free(b2);
}