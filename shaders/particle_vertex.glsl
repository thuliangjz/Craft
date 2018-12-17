#version 120

attribute vec4 position;
attribute vec2 uv;

uniform mat4 mat_mvp;

varying vec2 frag_uv;

void main(){
    gl_Position = mat_mvp * position;
    frag_uv = uv;
}