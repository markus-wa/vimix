#include "defines.h"
#include "ImageShader.h"

ShadingProgram imageShadingProgram("shaders/texture-shader.vs", "shaders/texture-shader.fs");

ImageShader::ImageShader()
{
    program_ = &imageShadingProgram;
    reset();
}

void ImageShader::use()
{
    Shader::use();

    program_->setUniform("brightness", brightness);
    program_->setUniform("contrast", contrast);
}


void ImageShader::reset()
{
    Shader::reset();

    brightness = 0.f;
    contrast = 0.f;
}
