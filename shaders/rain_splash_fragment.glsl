#version 120
varying float valid;

void main(){
    if(valid > 0.9){
        discard;
    }
    gl_FragColor = vec4(0., 0.46, 0.74, 1.0);
}