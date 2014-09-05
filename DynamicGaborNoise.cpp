/*
 *  DynamicGaborNoise.cpp
 *  DynamicGaborNoise
 *
 *  Created by Bram-Ernst Verhoef on 9/01/14.
 *  Copyright 2014 University of Chicago. All rights reserved.
 *
 */

#include "DynamicGaborNoise.h"

#include <algorithm>
#include <cmath>
#include <boost/math/special_functions/round.hpp>

#define BUFFER_OFFSET(offset) ((void *)(offset))
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

#define PROGRAM_NAME "Dynamic_Gabor_Noise"

const float gabor_noise_truncate = 0.01; // the value of the Gaussian at which the noise Gabor is truncated


const std::string DynamicGaborNoise::HORIZONTALRESOLUTION("horizontalResolution");
const std::string DynamicGaborNoise::VERTICALRESOLUTION("verticalResolution");
const std::string DynamicGaborNoise::VIEWINGDISTANCE("viewingDistance");
const std::string DynamicGaborNoise::HORIZONTALSCREENSIZE("horizontalScreenSize");
const std::string DynamicGaborNoise::TEXTURESIZE("textureSize");
const std::string DynamicGaborNoise::NOISE_NIMPULSES("noise_nImpulses");
const std::string DynamicGaborNoise::NOISE_SPATIALFREQUENCY("noise_spatialFrequency");
const std::string DynamicGaborNoise::NOISE_BANDWIDTH("noise_bandwidth");
const std::string DynamicGaborNoise::NOISE_TIMESPEEDUP("noise_timeSpeedUp");
const std::string DynamicGaborNoise::NOISE_TIMESPEEDUPSIGMA("noise_timeSpeedUpSigma");
const std::string DynamicGaborNoise::NOISE_CONTRAST("noise_contrast");
const std::string DynamicGaborNoise::AZIMUTH("azimuth");
const std::string DynamicGaborNoise::ELEVATION("elevation");
const std::string DynamicGaborNoise::SIGMA("sigma");
const std::string DynamicGaborNoise::ORIENTATION("orientation");
const std::string DynamicGaborNoise::SPATIALFREQUENCY("spatialFrequency");
const std::string DynamicGaborNoise::PHASEOFFSET("phaseOffset");
const std::string DynamicGaborNoise::CONTRAST("contrast");
const std::string DynamicGaborNoise::TRANSPARENCY("transparency");



void DynamicGaborNoise::describeComponent(ComponentInfo &info) {
    StandardDynamicStimulus::describeComponent(info);
    
    info.setSignature("stimulus/DynamicGaborNoise");
    info.setDisplayName("Dynamic Gabor Noise");
    info.setDescription("A Gabor surrounded by Gabor noise.");

    info.addParameter(HORIZONTALRESOLUTION,"1980");
    info.addParameter(VERTICALRESOLUTION,"1080");
    info.addParameter(VIEWINGDISTANCE,"300");
    info.addParameter(HORIZONTALSCREENSIZE,"477");
    info.addParameter(TEXTURESIZE,"800");
    info.addParameter(NOISE_NIMPULSES, "5");
    info.addParameter(NOISE_SPATIALFREQUENCY, "0.1");
    info.addParameter(NOISE_BANDWIDTH, "0.1");
    info.addParameter(NOISE_TIMESPEEDUP, "0.95");
    info.addParameter(NOISE_TIMESPEEDUPSIGMA, "5");
    info.addParameter(NOISE_CONTRAST, "1.0");
    info.addParameter(AZIMUTH, "1.0");
    info.addParameter(ELEVATION, "1.0");
    info.addParameter(SIGMA, "3.0");
    info.addParameter(ORIENTATION, "45.0");
    info.addParameter(SPATIALFREQUENCY, "0.11");
    info.addParameter(PHASEOFFSET, "0.0");
    info.addParameter(CONTRAST, "1.0");
    info.addParameter(TRANSPARENCY, "1.0");
}


DynamicGaborNoise::DynamicGaborNoise(const ParameterValueMap &parameters) :
StandardDynamicStimulus(parameters),
    horizontalResolution(parameters[HORIZONTALRESOLUTION]),
    verticalResolution(registerVariable(parameters[VERTICALRESOLUTION])),
    viewingDistance(registerVariable(parameters[VIEWINGDISTANCE])),
    horizontalScreenSize(parameters[HORIZONTALSCREENSIZE]),
    textureSize(registerVariable(parameters[TEXTURESIZE])),
    noise_nImpulses(parameters[NOISE_NIMPULSES]),
    noise_spatialFrequency(registerVariable(parameters[NOISE_SPATIALFREQUENCY])),
    noise_bandWidth(registerVariable(parameters[NOISE_BANDWIDTH])),
    noise_timeSpeedUp(parameters[NOISE_TIMESPEEDUP]),
    noise_timeSpeedUpSigma(registerVariable(parameters[NOISE_TIMESPEEDUPSIGMA])),
    noise_contrast(registerVariable(parameters[NOISE_CONTRAST])),
    azimuth(parameters[AZIMUTH]),
    elevation(registerVariable(parameters[ELEVATION])),
    sigma(registerVariable(parameters[SIGMA])),
    orientation(parameters[ORIENTATION]),
    spatialFrequency(registerVariable(parameters[SPATIALFREQUENCY])),
    phaseOffset(registerVariable(parameters[PHASEOFFSET])),
    contrast(registerVariable(parameters[CONTRAST])),
    transparency(registerVariable(parameters[TRANSPARENCY])),
    previousTime(-1),
    currentTime(-1)
{
    
    double halfScreenVisualDeg = 180.0 * std::atan((horizontalScreenSize->getValue().getFloat() / 2.0) / viewingDistance->getValue().getFloat()) / M_PI;
    double pixelsPerDeg = (horizontalResolution->getValue().getFloat() / 2.0) / halfScreenVisualDeg;
    gabor_noise_frequency = noise_spatialFrequency->getValue().getFloat() / pixelsPerDeg;
    gabor_noise_bandWidth = noise_bandWidth->getValue().getFloat() / pixelsPerDeg;
    detection_Gabor_XLocation = textureSize->getValue().getFloat() / 2.0 + azimuth->getValue().getFloat() * pixelsPerDeg;
    detection_Gabor_YLocation = textureSize->getValue().getFloat() / 2.0 + elevation->getValue().getFloat() * pixelsPerDeg;
    detection_Gabor_Frequency = spatialFrequency->getValue().getFloat() / pixelsPerDeg;
    detection_Gabor_Sigma = sigma->getValue().getFloat() * pixelsPerDeg;
    detection_Gabor_Orientation = (orientation->getValue().getFloat() / 180.0) * M_PI + M_PI / 2.0;
    detection_Gabor_Offset = (phaseOffset->getValue().getFloat() / 180.0) * M_PI;
    
    validateParameters();
}


void DynamicGaborNoise::load(shared_ptr<StimulusDisplay> display) {
    if (loaded)
        return;

    init();
    
    loaded = true;
}

/*
void DynamicGaborNoise::unload(shared_ptr<StimulusDisplay> display) {
    if (!loaded)
        return;
    
    for (int i = 0; i < display->getNContexts(); i++) {
        OpenGLContextLock ctxLock = display->setCurrent(i);
        }
    
       
    loaded = false;
}
*/



// #############################################################################

GLchar* DynamicGaborNoise::read_shader_source_from_file(const char* filename)
{
    FILE* fp = std::fopen(filename, "rb");
    assert(fp != NULL);
    std::fseek(fp, 0, SEEK_END);
    std::size_t shader_source_size = std::ftell(fp);
    std::rewind(fp);
    GLchar* shader_source = new GLchar[shader_source_size + 1];
    shader_source[shader_source_size] = 0;
    std::fread(shader_source, 1, shader_source_size, fp);
    std::fclose(fp);
    return shader_source;
}

void DynamicGaborNoise::shader_source_from_file(GLuint shader, const char* filename)
{
    const GLchar* shader_source = read_shader_source_from_file(filename);
    glShaderSource(shader, 1, &shader_source, NULL);
    delete [] shader_source;
}

void DynamicGaborNoise::shader_info_log(GLuint shader)
{
    GLint info_log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0) {
        char* info_log = new char[info_log_length];
        glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
        std::cout << info_log;
        delete [] info_log;
    }
}

void DynamicGaborNoise::compile_shader(GLuint shader)
{
    glCompileShader(shader);
    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        shader_info_log(shader);
        std::cerr << PROGRAM_NAME << ": " << "error: " << "\n";
        std::exit(EXIT_FAILURE);
    }
}

void DynamicGaborNoise::program_info_log(GLuint program)
{
    GLint info_log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0) {
        char* info_log = new char[info_log_length];
        glGetProgramInfoLog(program, info_log_length, NULL, info_log);
        std::cout << info_log;
        delete [] info_log;
    }
}

void DynamicGaborNoise::link_program(GLuint program)
{
    glLinkProgram(program);
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        program_info_log(program);
        std::cerr << PROGRAM_NAME << ": " << "error: " << "\n";
        std::exit(EXIT_FAILURE);
    }
}


void DynamicGaborNoise::load_shaders()
{
    static GLuint vertex_shader;
    static GLuint fragment_shader;
    
    if (glIsProgram(gabor_noise_program) == GL_FALSE) {
        gabor_noise_program = glCreateProgram();
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glAttachShader(gabor_noise_program, vertex_shader);
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glAttachShader(gabor_noise_program, fragment_shader);
    }
    
    const GLchar* vertex_shader_source = read_shader_source_from_file("Dynamic_Gabor_Noise.vs");
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL); 
    delete [] vertex_shader_source;
    compile_shader(vertex_shader);
    
    
    const GLchar* fragment_shader_source = read_shader_source_from_file("Dynamic_Gabor_Noise.fs");
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    delete [] fragment_shader_source;
    compile_shader(fragment_shader);
    
    link_program(gabor_noise_program);
    program_info_log(gabor_noise_program);
    
    
    glValidateProgram(gabor_noise_program);
    GLint validate_status = GL_FALSE;
    glGetProgramiv(gabor_noise_program, GL_VALIDATE_STATUS, &validate_status);
    if (validate_status == GL_FALSE) {
        program_info_log(gabor_noise_program);
    }
    
}

// #############################################################################


uint DynamicGaborNoise::getSeed()
{
  return unsigned (time(0));
    
};


class pseudo_random_number_generator {
private:
    GLuint seed_;
public:
    void seed(GLuint s) { seed_ = s; }
    GLuint changeSeed () {seed_ *= 3039177861u; return seed_; }
    GLfloat uniform_0_1 () {return GLfloat(changeSeed()) / GLfloat(UINT_MAX); }
    GLfloat uniform (GLfloat min, GLfloat max) { return min + (uniform_0_1() * (max - min)); }
    GLfloat gaussian_rv (GLfloat mean, GLfloat variance) {GLfloat x_1 = uniform_0_1(); GLfloat x_2 = uniform_0_1(); GLfloat z = sqrt(-2.0 * log(x_1)) * cos(2.0 * M_PI * x_2); return mean + (sqrt(variance) * z); } // Box-Muller transformation
};


void DynamicGaborNoise::gabor_noise_begin()
{
    glUseProgram(gabor_noise_program);
    
    // Compute some parameter values for the Gabors
    
    GLfloat gabor_noise_2d_r;
    GLfloat gabor_noise_2d_f[2];
    GLfloat gabor_noise_2d_a;
    GLfloat gabor_noise_2d_lambda;
    float gabor_noise_orientation_theta = M_PI / 4.0;
    unsigned gabor_noise_impulses = noise_nImpulses->getValue().getInteger();
    
    
    gabor_noise_2d_f[0]   = gabor_noise_frequency * std::cos(gabor_noise_orientation_theta);
    gabor_noise_2d_f[1]   = gabor_noise_frequency * std::sin(gabor_noise_orientation_theta);
    gabor_noise_2d_a      = gabor_noise_bandWidth;
    gabor_noise_2d_r      = std::sqrt(-log(gabor_noise_truncate) / M_PI) / gabor_noise_2d_a;
    gabor_noise_2d_lambda = noise_nImpulses->getValue().getFloat() / (M_PI * (gabor_noise_2d_r * gabor_noise_2d_r));
    
    // Compute the random variables that will be stored in the uniform block
    
    enum uniformBlocks { Gabor_X_Indices, Gabor_Y_Indices, Gabor_Orientations, Gabor_PhaseJitter, NumUniformBlocks};
    GLuint gabor_noise_gridSize = ceil(textureSize->getValue().getInteger() / gabor_noise_2d_r) + 2;
    int nArrayElements = gabor_noise_gridSize * gabor_noise_gridSize * gabor_noise_impulses * NumUniformBlocks;
    int nImpulses = gabor_noise_gridSize * gabor_noise_gridSize * gabor_noise_impulses;
    GLfloat gabor_impulseParams[nArrayElements];
    
    pseudo_random_number_generator prng;
    uint gabor_noise_seed = getSeed();
    prng.seed(gabor_noise_seed);
    int iter = 0;
    for (int imp = 0; imp < nImpulses; imp++) {
        for (int pn = 0; pn < NumUniformBlocks; pn++) {
            switch (pn) {
                case Gabor_X_Indices:
                    gabor_impulseParams[iter] = prng.uniform_0_1();
                    break;
                case Gabor_Y_Indices:
                    gabor_impulseParams[iter] = prng.uniform_0_1();
                    break;
                case Gabor_Orientations:
                    gabor_impulseParams[iter] = prng.uniform(0.0, 2.0 * M_PI);
                    break;
                case Gabor_PhaseJitter:
                    gabor_impulseParams[iter] = prng.gaussian_rv(0.0, noise_timeSpeedUpSigma->getValue().getFloat());
                    break;
            }
            iter++;
        }
    }
    
    // Set up the uniform buffer and variables
    
    GLuint uniformBuffer, blockBinding = 0;
    GLint blockSize;
    glGenBuffers(1, &uniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
    GLuint blockIndex = glGetUniformBlockIndex(gabor_noise_program, "ImpulseParam");
    glGetActiveUniformBlockiv(gabor_noise_program,blockIndex,GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
    glUniformBlockBinding(gabor_noise_program, blockIndex, blockBinding);
    glBufferData(GL_UNIFORM_BUFFER, blockSize, gabor_impulseParams, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, blockBinding, uniformBuffer);
    
    uniformTimeLocation = glGetUniformLocation(gabor_noise_program, "gabor_noise_2d_time");
    glUniform1f(glGetUniformLocation(gabor_noise_program, "gabor_noise_2d_r"), gabor_noise_2d_r);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "gabor_noise_2d_a"), gabor_noise_2d_a);
    glUniform2f(glGetUniformLocation(gabor_noise_program, "gabor_noise_2d_f"), gabor_noise_2d_f[0], gabor_noise_2d_f[1]);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "gabor_noise_2d_lambda"), gabor_noise_2d_lambda);
    glUniform1ui(glGetUniformLocation(gabor_noise_program, "gabor_noise_gridSize"), gabor_noise_gridSize);
    glUniform1ui(glGetUniformLocation(gabor_noise_program, "gabor_noise_impulses"), gabor_noise_impulses);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "gabor_noise_contrast"), noise_contrast->getValue().getFloat());
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_XLocation"), detection_Gabor_XLocation);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_YLocation"), detection_Gabor_YLocation);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_Sigma"), detection_Gabor_Sigma);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_Orientation"), detection_Gabor_Orientation);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_Frequency"), detection_Gabor_Frequency);
    glUniform1f(glGetUniformLocation(gabor_noise_program, "detection_Gabor_Offset"), detection_Gabor_Offset);
    transparencyLocation = glGetUniformLocation(gabor_noise_program, "detection_Gabor_Transparency");
    contrastLocation = glGetUniformLocation(gabor_noise_program, "detection_Gabor_Contrast");
    glUniform1f(transparencyLocation, transparency->getValue().getFloat());
    glUniform1f(contrastLocation, 0.0);
    
}


void DynamicGaborNoise::init()
{
    
    glClearColor(0.5,0.5,0.5,1);
    
    
    // set up the vertices:
    // already in clip space
    
    const GLfloat vertices[] = {
        1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    GLuint VAO, bufferObject;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Build the vertex buffer
    glGenBuffers(1, &bufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) , vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    load_shaders();
    gabor_noise_begin();
    
}




void DynamicGaborNoise::validateParameters() const {
    if (contrast->getValue().getFloat() <= 0.0f || contrast->getValue().getFloat() >= 1.0f) {
        throw SimpleException("contrast must be within [0,1]");
    }
    
    // make one for the maximum number of impulses

}


/*void DynamicGaborNoise::computeDotSizeToPixels(shared_ptr<StimulusDisplay> display) {
    dotSizeToPixels.clear();
    
    GLdouble xMin, xMax, yMin, yMax;
    GLint width, height;

    display->getDisplayBounds(xMin, xMax, yMin, yMax);
    
    for (int i = 0; i < display->getNContexts(); i++) {
        OpenGLContextLock ctxLock = display->setCurrent(i);
        display->getCurrentViewportSize(width, height);
        dotSizeToPixels.push_back(GLdouble(width) / (xMax - xMin));
    }
}*/


void DynamicGaborNoise::drawFrame(shared_ptr<StimulusDisplay> display) {
    
    /*if (display->getCurrentContextIndex() == 0) {
        currentTime = double(getElapsedTime()) / 1000000.0; // in seconds
        if ((previousTime != -1) && (previousTime != currentTime)) {
            updateDots();
        }
        previousTime = currentTime;
    }*/
    
    currentTime = double(getElapsedTime()) / 1000000.0; // in seconds
    double gabor_noise_2d_time = noise_timeSpeedUp->getValue().getFloat() * (currentTime/60.0);
    glUniform1f(uniformTimeLocation, gabor_noise_2d_time);
    
    //if (frame == detection_Gabor_onsetFrame) {
        glUniform1f(transparencyLocation, transparency->getValue().getFloat());
        glUniform1f(contrastLocation, contrast->getValue().getFloat());
    //}
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    /* swap front and back buffers*/
    // Add 10 render frames (glFinish) without swapping the buffer to "warm up" (= allow the gpu to detect and implement any necessary optimizations) the gpu
    
    /*if (frame <= extraFrames){
        glFinish();
    }
    else if (frame > extraFrames) {
        if (frame == extraFrames + 1){
            startTime = glfwGetTime();
        }
        glfwSwapBuffers(window);
    }*/
}


Datum DynamicGaborNoise::getCurrentAnnounceDrawData() {
    boost::mutex::scoped_lock locker(stim_lock);

    Datum announceData = StandardDynamicStimulus::getCurrentAnnounceDrawData();

    announceData.addElement(STIM_TYPE, "Dynamic_Gabor_Noise");
    announceData.addElement(HORIZONTALRESOLUTION, horizontalResolution->getValue().getInteger());
    announceData.addElement(VERTICALRESOLUTION, verticalResolution->getValue().getInteger());
    announceData.addElement(VIEWINGDISTANCE, viewingDistance->getValue().getInteger());
    announceData.addElement(TEXTURESIZE, textureSize->getValue().getInteger());
    announceData.addElement(NOISE_NIMPULSES, noise_nImpulses->getValue().getInteger());
    announceData.addElement(NOISE_SPATIALFREQUENCY, noise_bandWidth->getValue().getFloat());
    announceData.addElement(NOISE_BANDWIDTH, noise_bandWidth->getValue().getFloat());
    announceData.addElement(NOISE_TIMESPEEDUP, noise_timeSpeedUp->getValue().getFloat());
    announceData.addElement(NOISE_TIMESPEEDUPSIGMA, noise_timeSpeedUpSigma->getValue().getFloat());
    announceData.addElement(AZIMUTH, azimuth->getValue().getFloat());
    announceData.addElement(ELEVATION, elevation->getValue().getFloat());
    announceData.addElement(SIGMA, sigma->getValue().getFloat());
    announceData.addElement(ORIENTATION, orientation->getValue().getFloat());
    announceData.addElement(SPATIALFREQUENCY, spatialFrequency->getValue().getFloat());
    announceData.addElement(PHASEOFFSET, phaseOffset->getValue().getFloat());
    announceData.addElement(CONTRAST, contrast->getValue().getFloat());
    announceData.addElement(TRANSPARENCY, transparency->getValue().getFloat());
    
    return announceData;
}


void DynamicGaborNoise::stopPlaying() {
    StandardDynamicStimulus::stopPlaying();
    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    gabor_noise_end();
    previousTime = -1;
}

void DynamicGaborNoise::gabor_noise_end()
{
    glUseProgram(0);
}





















