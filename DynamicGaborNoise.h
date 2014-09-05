/*
 *  DynamicGaborNoise.cpp
 *  DynamicGaborNoise
 *
 *  Created by Bram-Ernst Verhoef on 8/29/14.
 *  Copyright 2014 University of Chicago. All rights reserved.
 *
 */

#ifndef DynamicGaborNoisePlugin_H_
#define DynamicGaborNoisePlugin_H_


using namespace mw;


class DynamicGaborNoise : public StandardDynamicStimulus {

public:
    
    // GENERAL PARAMETERS
    
    static const std::string HORIZONTALRESOLUTION; // in pixels
    static const std::string VERTICALRESOLUTION;   // in pixels
    static const std::string VIEWINGDISTANCE;      // in mm
    static const std::string HORIZONTALSCREENSIZE; // in mm
    static const std::string TEXTURESIZE;          // in pixels
    
    // GABOR NOISE PARAMETERS
    
    static const std::string NOISE_NIMPULSES;
    static const std::string NOISE_SPATIALFREQUENCY;
    static const std::string NOISE_BANDWIDTH;
    static const std::string NOISE_TIMESPEEDUP;
    static const std::string NOISE_TIMESPEEDUPSIGMA;
    static const std::string NOISE_CONTRAST;
    
    // DETECTION GABOR PARAMETERS
    
    static const std::string AZIMUTH;
    static const std::string ELEVATION;
    static const std::string SIGMA;
    static const std::string ORIENTATION;
    static const std::string SPATIALFREQUENCY;
    static const std::string PHASEOFFSET;
    static const std::string CONTRAST;
    static const std::string TRANSPARENCY;
    
    static void describeComponent(ComponentInfo &info);

    explicit DynamicGaborNoise(const ParameterValueMap &parameters);
    ~DynamicGaborNoise() { }
    
    void load(shared_ptr<StimulusDisplay> display) MW_OVERRIDE;
    //void unload(shared_ptr<StimulusDisplay> display)MW_OVERRIDE;
    void drawFrame(shared_ptr<StimulusDisplay> display) MW_OVERRIDE;
    Datum getCurrentAnnounceDrawData() MW_OVERRIDE;
   
protected:
    void stopPlaying() MW_OVERRIDE;
    
private:
    
    void validateParameters() const;
    void init();
    void load_shaders();
    GLchar* read_shader_source_from_file(const char* filename);
    void shader_source_from_file(GLuint shader, const char* filename);
    void shader_info_log(GLuint shader);
    void compile_shader(GLuint shader);
    void program_info_log(GLuint program);
    void link_program(GLuint program);
    void gabor_noise_begin();
    uint getSeed();
    void gabor_noise_end();

    //void computeDotSizeToPixels(shared_ptr<StimulusDisplay> display);

    
    
    shared_ptr<Variable> horizontalResolution;
    shared_ptr<Variable> verticalResolution;
    shared_ptr<Variable> viewingDistance;
    shared_ptr<Variable> horizontalScreenSize;
    shared_ptr<Variable> textureSize;
    shared_ptr<Variable> noise_nImpulses;
    shared_ptr<Variable> noise_spatialFrequency;
    shared_ptr<Variable> noise_bandWidth;
    shared_ptr<Variable> noise_timeSpeedUp;
    shared_ptr<Variable> noise_timeSpeedUpSigma;
    shared_ptr<Variable> noise_contrast;
    shared_ptr<Variable> azimuth;
    shared_ptr<Variable> elevation;
    shared_ptr<Variable> sigma;
    shared_ptr<Variable> orientation;
    shared_ptr<Variable> spatialFrequency;
    shared_ptr<Variable> phaseOffset;
    shared_ptr<Variable> contrast;
    shared_ptr<Variable> transparency;
    GLfloat gabor_noise_frequency;
    GLfloat gabor_noise_bandWidth;
    GLfloat detection_Gabor_XLocation;
    GLfloat detection_Gabor_YLocation;
    GLfloat detection_Gabor_Frequency;
    GLfloat detection_Gabor_Sigma;
    GLfloat detection_Gabor_Orientation;
    GLfloat detection_Gabor_Offset;
    GLuint  gabor_noise_program;
    GLuint  transparencyLocation;
    GLuint  contrastLocation;
    GLuint  uniformTimeLocation;

    
    MWTime previousTime, currentTime;
    
};


#endif 























