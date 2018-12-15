#version 120

attribute vec4 position;
attribute vec2 uv_texture;
attribute vec2 uv_weight;

uniform mat4 matrix_mvp;

varying vec2 frag_uv_texture;
varying vec2 frag_uv_weight;
void main() {
    frag_uv_texture = uv_texture;
    frag_uv_weight = uv_weight;
    gl_Position = matrix_mvp * position;
}