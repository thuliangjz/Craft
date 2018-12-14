#version 120

attribute vec4 position;
attribute vec2 uv;

varying vec2 frag_uv;
void main() {
    frag_uv = uv;
    gl_Position = position;
}