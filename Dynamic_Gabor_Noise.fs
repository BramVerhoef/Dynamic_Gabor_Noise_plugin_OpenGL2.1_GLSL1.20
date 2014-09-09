
#version 330

//  Created by Bram-Ernst Verhoef on 8/16/14.
//  Copyright (c) 2014 Bram-Ernst Verhoef. All rights reserved.
//
//  Based on "Procedural Noise using Sparse Gabor Convolution" by Lagae et al. (2009)


# define pi 3.14159265358979323846

/// ############################################################################

float borderSize = 40.0; // Size of the border transition between detection Gabor and noise
float nGaborSigmas = 3.0; // number of Gabor sigmas shown

// Uniform variables

uniform float gabor_noise_2d_r;
uniform float gabor_noise_2d_a;
uniform vec2 gabor_noise_2d_f;
uniform float gabor_noise_2d_lambda;
uniform float gabor_noise_2d_time;
uniform uint gabor_noise_gridSize;
uniform uint gabor_noise_impulses;
uniform float gabor_noise_contrast;

uniform float detection_Gabor_XLocation;
uniform float detection_Gabor_YLocation;
uniform float detection_Gabor_Sigma;
uniform float detection_Gabor_Orientation;
uniform float detection_Gabor_Frequency;
uniform float detection_Gabor_Offset;
uniform float detection_Gabor_Contrast;
uniform float detection_Gabor_Transparency;


// Uniform block

layout (std140) uniform ImpulseParam {
    vec4 impulseParam[2500]; // The GPU optimizes and ignores the non-active uniforms within this array. However, it is better so set this one high in case they are needed by the application (you can't set the array size with a variable (dynamically) for a uniform block (in contrast to a shader buffer object, which is not supported by openGL 4.1 = our current version)
};

float nArrayPosPerRow  = float(gabor_noise_impulses * gabor_noise_gridSize) * 4.0;
float nArrayPosPerCell = float(gabor_noise_impulses) * 4.0;


// -----------------------------------------------------------------------------

float gabor_noise_kernel_2d(const in float w, const in vec2 f, const in float phi, const in float a, const in vec2 x)
{
    float g = exp(-pi * (a * a) * dot(x, x));
    float h = sin((2.0 * pi * dot(f, x)) + phi);
    return w * g * h;
}

float gabor_noise_kernel_detect(const in float w, const in vec2 f, const in float phi, const in float a, const in vec2 x)
{
    float g = exp(-0.5 * (a * a) * dot(x, x));
    float h = sin((2.0 * pi * dot(f, x)) + phi);
    return w * g * h;
}

float detection_gabor_kernel(const in vec2 fragment, inout float detection_gabor_alpha)
{
    vec2 x             = vec2(detection_Gabor_XLocation - fragment.x, detection_Gabor_YLocation - fragment.y);
    float sigma        = 1.0 / detection_Gabor_Sigma;
    vec2 f_i           = detection_Gabor_Frequency * vec2(cos(detection_Gabor_Orientation), sin(detection_Gabor_Orientation));
    float kernel_value = gabor_noise_kernel_detect(detection_Gabor_Contrast, f_i, detection_Gabor_Offset, sigma, x);
    if (length(x) <= nGaborSigmas * detection_Gabor_Sigma + borderSize / 2.0) {
        if (length(x) > nGaborSigmas * detection_Gabor_Sigma - borderSize / 2.0) { // Use a Hanning window to taper the edges
            float borderDistance = length(x) - (nGaborSigmas * detection_Gabor_Sigma - borderSize / 2.0);
            detection_gabor_alpha = 0.5 * (1.0 + cos(pi * borderDistance / borderSize)) * detection_Gabor_Transparency;
        }
        else {
            detection_gabor_alpha = detection_Gabor_Transparency;
        }
        return 0.5 + 0.5 * kernel_value;
    }
    else {
        detection_gabor_alpha = 0.0;
        return 0.0;
    }
}

// -----------------------------------------------------------------------------

struct gabor_noise_2d
{
    float r_;
    float a_;
    vec2  f_;
    float lambda_;
};

void gabor_noise_2d_constructor(out gabor_noise_2d this_, const in float r, const in float a, const in vec2 f, const in float lambda)

{
    this_.r_ = r;
    this_.a_ = a;
    this_.f_ = f;
    this_.lambda_ = lambda;
}


float gabor_noise_2d_cell(const in gabor_noise_2d this_, const in ivec2 c, const in vec2 x_c, const in float t)
{
    uint currentCellInArray = uint((nArrayPosPerRow * (float(c.y) + 1.0) + (nArrayPosPerCell * (float(c.x) + 1.0))) / 4.0);// +1.0 because c.y and c.x can be -1
    uint n = gabor_noise_impulses;
    float sum = 0.0;
    for (uint i = 0u; i < n; ++i) {
        float indexf = currentCellInArray + i;
        uint index    = uint(indexf);
        
        vec2 x_i_c = vec2(impulseParam[index][0], impulseParam[index][1]);
        vec2 x_k_i = this_.r_ * (x_c - x_i_c);
        if (dot(x_k_i, x_k_i) < (this_.r_ * this_.r_)) {
            float w_i = gabor_noise_contrast;
            float f_r = length(this_.f_);
            float f_t = impulseParam[index][2];
            vec2 f_i  = f_r * vec2(cos(f_t), sin(f_t));
            float phi_i = t * impulseParam[index][3];
            float a_i   = this_.a_;
            sum += gabor_noise_kernel_2d(w_i, f_i, phi_i, a_i, x_k_i);
        }
    }
    return sum;
}


float gabor_noise_2d_grid(const in gabor_noise_2d this_, const in vec2 x_g, const in float t)
{
    vec2 int_x_g = floor(x_g);
    ivec2 c = ivec2(int_x_g);
    vec2 x_c = x_g - int_x_g;
    float sum = 0.0;
    ivec2 i;
    for (i[1] = -1; i[1] <= +1; ++i[1]) {
        for (i[0] = -1; i[0] <= +1; ++i[0]) {
            ivec2 c_i = c + i;
            vec2 x_c_i = x_c - i;
            sum += gabor_noise_2d_cell(this_, c_i, x_c_i, t);/
        }
    }
    return sum / sqrt(this_.lambda_);
}

float gabor_noise_2d_noise(const in gabor_noise_2d this_, const in vec2 x, const in float t)
{
    vec2 x_g = x / this_.r_;
    return gabor_noise_2d_grid(this_, x_g, t);
}

float gabor_noise_2d_variance(const in gabor_noise_2d this_)
{
    return 1.0 / (4.0 * (this_.a_ * this_.a_));
}

/// ############################################################################

in vec2 x_tex;
out vec4 fragColor;

float detection_gabor_alpha;

void main()
{
    gabor_noise_2d gabor_noise_2d_;
    gabor_noise_2d_constructor(gabor_noise_2d_, gabor_noise_2d_r, gabor_noise_2d_a, gabor_noise_2d_f, gabor_noise_2d_lambda);
    float noise = gabor_noise_2d_noise(gabor_noise_2d_, x_tex.xy, gabor_noise_2d_time);
    float noise_scale = 0.5 / (3.0 * sqrt(gabor_noise_2d_variance(gabor_noise_2d_)));
    float noise_bias = 0.5;
    float noise_intensity = noise_bias + (noise_scale * noise);
    float detection_gabor_intensity = detection_gabor_kernel(x_tex, detection_gabor_alpha);
    fragColor = (1.0 - detection_gabor_alpha) * vec4(vec3(noise_intensity), 1.0) + detection_gabor_alpha * vec4(vec3(detection_gabor_intensity), 1.0);
}

/// ############################################################################
