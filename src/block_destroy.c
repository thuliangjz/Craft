#include "block_destroy.h"
#include "item.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"
#define DESTROY_TEXTURE_SIZE 128
#define DESTROY_TEXTURE_LEVEL 4

#define DESTROY_MASK 255
void update_destroying_block(double d_w, double d_r, BlockDestroying *block_destroying, State *state){
    if(!block_destroying->dec)
        return;
    if(d_w > 5 || d_r > 5){
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
            return;
        }
    }
    double d = glfwGetTime() - block_destroying->start_stamp;
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
                table[i][j][k] = (char)(rand() % DESTROY_MASK);
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

void render_destroy_texture_test(BlockDestroying *block_destorying){
    float buffer[] = {
        -1, 1, 0, 0, 1,
        -1, -1, 0, 0, 0,
        1, -1, 0, 1, 0,
        1, 1, 0, 1, 1, 
        -1, 1, 0, 0, 1,
        1, -1, 0, 1, 0,
    }; 
    glUseProgram(block_destorying->program);
    glUniform1i(block_destorying->sampler, 4);
    GLuint gl_buffer = gen_buffer(sizeof(buffer), buffer);
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
    glEnableVertexAttribArray(block_destorying->position);
    glEnableVertexAttribArray(block_destorying->uv);
    glVertexAttribPointer(block_destorying->position, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 5, 0);
    glVertexAttribPointer(block_destorying->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 5, sizeof(GL_FLOAT) * 3);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(block_destorying->position);
    glDisableVertexAttribArray(block_destorying->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    del_buffer(gl_buffer);
}