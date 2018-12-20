#version 120
attribute vec3 pos_quad;
attribute vec3 v;
attribute vec3 translation;
attribute float t_init;
attribute float angle;

uniform float t;    //保证位于0~t_max之间
uniform float t_animation_max;
uniform float t_max;
uniform float g;
uniform mat4 mat_mvp;

varying float valid;

void main(){
    float t_current = t_init + t;
    t_current = t_current > t_max ? t_current - t_max : t_current;
    //如果不在动画播放时间则指示fragment shader丢弃
    valid = t_current > t_animation_max ? 1.0 : 0.0;

    float s = sin(angle), c = cos(angle);
    vec3 v_rotated = v;
    v_rotated.x = v.x * c - v.z * s;
    v_rotated.z = v.x * s + v.z * c;
    vec3 pos;
    pos.x = v_rotated.x * t_current + translation.x;
    pos.z = v_rotated.z * t_current + translation.z;
    pos.y = v_rotated.y * t_current - 0.5 * g * t_current * t_current + translation.y;

    pos.x += pos_quad.x * c - pos_quad.z * s;
    pos.y += pos_quad.y;
    pos.z += pos_quad.x * s + pos_quad.z * c;

    gl_Position = mat_mvp * vec4(pos, 1.0);
}