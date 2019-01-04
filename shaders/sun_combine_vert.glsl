#version 120
attribute vec3 position;
attribute vec2 uv;

varying vec2 TexCoords;

void main()
{
    TexCoords = uv;
    gl_Position = vec4(position, 1.0);
}