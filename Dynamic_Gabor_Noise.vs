
#version 330


uniform float gabor_noise_texture_size;

layout(location=0) in vec4 position;
out vec2 x_tex;

mat4 scale(vec3 v) { return mat4(vec4(v.x, 0.0, 0.0, 0.0), vec4(0.0, v.y, 0.0, 0.0), vec4(0.0, 0.0, v.z, 0.0), vec4(0.0, 0.0, 0.0, 1.0)); }

void main()
{
    gl_Position = position;
    x_tex = vec2((gabor_noise_texture_size-1) * (position + vec4(1.0))/2.0); 
    
}

