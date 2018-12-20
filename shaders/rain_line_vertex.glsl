#version 120

attribute vec3 pos_quad;    //顶点相对于雨的头部中间的偏移量 
attribute float rain_line_head; //每条线小y端点在seq中的相对坐标
attribute float y_min;          //每个seq的中第一根雨丝的y值较小的端点的最小值
attribute vec3 seq_pos;         //每个seq的偏移，其中y坐标表示这个seq的随机偏移量
attribute float theta;

uniform mat4 mat_mvp;
uniform float t;
uniform float t_total;      //实际上t_total和t都是整数，传入的时候做了类型转换
uniform float d_max;

void main(){
    vec3 head_pos = vec3(seq_pos.x, seq_pos.y + y_min + rain_line_head, seq_pos.z);
    head_pos.y -=  t / t_total * d_max;
    head_pos.y = head_pos.y < y_min ? y_min + d_max - (y_min - head_pos.y) : head_pos.y;

    vec3 pos_vertex;
    pos_vertex.x = cos(theta) * pos_quad.x;
    pos_vertex.z = sin(theta) * pos_quad.x;
    pos_vertex.y = pos_quad.y;

    gl_Position = mat_mvp * vec4(head_pos + pos_vertex, 1.0);
}