#version 120
varying vec2 TexCoords;

uniform sampler2D scene;
// uniform sampler2D bloomBlur;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture2D(scene, TexCoords).rgb;      
    // vec3 bloomColor = texture2D(bloomBlur, TexCoords).rgb;
    // hdrColor += bloomColor; // additive blending
    // vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // result = pow(result, vec3(1.0 / gamma));
    gl_FragColor = vec4(hdrColor, 1.0f);
}