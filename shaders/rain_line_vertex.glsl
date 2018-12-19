#version 120

attribute float rain_line_y;    //用于区分一条线两个端点
attribute float rain_line_head; //每条线小y端点在seq中的相对坐标
attribute float y_min;          //每个seq的中第一根雨丝的y值较小的端点的最小值
attribute vec3 seq_pos;         //每个seq的偏移，其中y坐标表示这个seq的随机偏移量

uniform mat4 mat_mvp;
uniform float t;
uniform float t_total;      //实际上t_total和t都是整数，传入的时候做了类型转换
uniform float d_max;

void main(){
    //计算当前的line的head的初始位置
    vec3 head_pos = vec3(seq_pos.x, seq_pos.y + y_min + rain_line_head, seq_pos.z);
    //计算t时刻head的位置
    head_pos.y -=  t / t_total * d_max;
    head_pos.y = head_pos.y < y_min ? y_min + d_max - (y_min - head_pos.y) : head_pos.y;
    //端点逻辑位置
    vec3 line_pos = head_pos;
    line_pos.y = head_pos.y + rain_line_y;
    gl_Position = mat_mvp * vec4(line_pos, 1.0);
}