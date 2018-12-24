#version 120

void main() {
    // vec3 lighting = vec3(1.0);

    // FragColor = vec4(lighting, 1.0f);
    // // Check whether fragment output is higher than threshold, if so output as brightness color
    // float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    // if(brightness > 0.99)
    //     BrightColor = vec4(FragColor.rgb, 1.0);
    gl_FragColor = vec4(1.0);
}