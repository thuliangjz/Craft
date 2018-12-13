#include "block_destroy.h"
#include "item.h"
#include <GLFW/glfw3.h>
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
