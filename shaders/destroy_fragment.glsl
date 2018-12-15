#version 120

uniform sampler2D sampler_texture;
uniform sampler2D sampler_weight;

varying vec2 frag_uv_texture;
varying vec2 frag_uv_weight;

void main() {
    vec3 color = texture2D(sampler_texture, frag_uv_texture).xyz;
    if(color == vec3(1.0, 0.0, 1.0)) {
        discard;
    }
    gl_FragColor = vec4(vec3(texture2D(sampler_weight, frag_uv_weight).r) * color, 1.0);
}