#version 120

uniform sampler2D sampler;
varying vec2 frag_uv;

void main() {
    gl_FragColor = vec4(texture2D(sampler, frag_uv).r);
}