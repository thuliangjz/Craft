#include "block_destroy.h"
#include "item.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#include <string.h>
#include "cube.h"
#define DESTROY_TEXTURE_SIZE 128
#define DESTROY_TEXTURE_LEVEL 4

#define DESTROY_MASK 150
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
    glEnableVertexAttribArray(uv_weight);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    del_buffer(gl_buffer);
    free(buffer);
    free(buffer_cube);    
}