#include "Shader.h"
#include "Resource.h"
#include "Log.h"
#include "Visitor.h"
#include "RenderingManager.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "GlmToolkit.h"
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// Globals
ShadingProgram *ShadingProgram::currentProgram_ = nullptr;
ShadingProgram simpleShadingProgram("shaders/simple.vs", "shaders/simple.fs");

// Blending presets for matching with Shader::BlendMode
GLenum blending_equation[6] = { GL_FUNC_ADD, GL_FUNC_ADD, GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD, GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD};
GLenum blending_source_function[6] = { GL_SRC_ALPHA,GL_SRC_ALPHA,GL_SRC_ALPHA,GL_SRC_ALPHA,GL_SRC_ALPHA,GL_SRC_ALPHA};
GLenum blending_destination_function[6] = {GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE, GL_DST_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA};



ShadingProgram::ShadingProgram(const std::string& vertex_file, const std::string& fragment_file) : vertex_id_(0), fragment_id_(0), id_(0)
{
    vertex_file_ = vertex_file;
    fragment_file_ = fragment_file;
}

void ShadingProgram::init()
{
    vertex_code_ = Resource::getText(vertex_file_);
    fragment_code_ = Resource::getText(fragment_file_);
	compile();
	link();
}

bool ShadingProgram::initialized()
{
    return (id_ != 0);
}

void ShadingProgram::compile()
{
    const char* vcode = vertex_code_.c_str();
    vertex_id_ = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_id_, 1, &vcode, NULL);
    glCompileShader(vertex_id_);

    const char* fcode = fragment_code_.c_str();
    fragment_id_ = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_id_, 1, &fcode, NULL);
    glCompileShader(fragment_id_);

    checkCompileErr();
}

void ShadingProgram::link()
{
    id_ = glCreateProgram();
    glAttachShader(id_, vertex_id_);
    glAttachShader(id_, fragment_id_);
    glLinkProgram(id_);
    checkLinkingErr();
    glUseProgram(id_);
    glUniform1i(glGetUniformLocation(id_, "iChannel0"), 0);
    glUniform1i(glGetUniformLocation(id_, "iChannel1"), 1);
    glUseProgram(0);
    glDeleteShader(vertex_id_);
    glDeleteShader(fragment_id_);
}

void ShadingProgram::use()
{
    if (currentProgram_ == nullptr || currentProgram_ != this)
    {
        currentProgram_ = this;
        glUseProgram(id_);
    }
}

void ShadingProgram::enduse()
{
    glUseProgram(0);
    currentProgram_ = nullptr ;
}

template<>
void ShadingProgram::setUniform<int>(const std::string& name, int val) {
	glUniform1i(glGetUniformLocation(id_, name.c_str()), val);
}

template<>
void ShadingProgram::setUniform<bool>(const std::string& name, bool val) {
	glUniform1i(glGetUniformLocation(id_, name.c_str()), val);
}

template<>
void ShadingProgram::setUniform<float>(const std::string& name, float val) {
	glUniform1f(glGetUniformLocation(id_, name.c_str()), val);
}

template<>
void ShadingProgram::setUniform<float>(const std::string& name, float val1, float val2) {
    glUniform2f(glGetUniformLocation(id_, name.c_str()), val1, val2);
}

template<>
void ShadingProgram::setUniform<float>(const std::string& name, float val1, float val2, float val3) {
    glUniform3f(glGetUniformLocation(id_, name.c_str()), val1, val2, val3);
}

template<>
void ShadingProgram::setUniform<glm::vec4>(const std::string& name, glm::vec4 val) {
    glm::vec4 v(val);
    glUniform4fv(glGetUniformLocation(id_, name.c_str()), 1, glm::value_ptr(v));
}

template<>
void ShadingProgram::setUniform<glm::vec3>(const std::string& name, glm::vec3 val) {
    glm::vec3 v(val);
    glUniform4fv(glGetUniformLocation(id_, name.c_str()), 1, glm::value_ptr(v));
}

template<>
void ShadingProgram::setUniform<glm::mat4>(const std::string& name, glm::mat4 val) {
    glm::mat4 m(val);
	glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}


// template<>
// void ShadingProgram::setUniform<float*>(const std::string& name, float* val) {
// 	glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, val);
// }

void ShadingProgram::checkCompileErr()
{
    int success;
    char infoLog[1024];
    glGetShaderiv(vertex_id_, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_id_, 1024, NULL, infoLog);
        Log::Warning("Error compiling Vertex ShadingProgram:\n%s", infoLog);
    }
    glGetShaderiv(fragment_id_, GL_COMPILE_STATUS, &success);
	if (!success) {
        glGetShaderInfoLog(fragment_id_, 1024, NULL, infoLog);
        Log::Warning("Error compiling Fragment ShadingProgram:\n%s", infoLog);
	}
}

void ShadingProgram::checkLinkingErr()
{
	int success;
	char infoLog[1024];
	glGetProgramiv(id_, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(id_, 1024, NULL, infoLog);
        Log::Warning("Error linking ShadingProgram:\n%s", infoLog);
	}
}


bool Shader::force_blending_opacity = false;

Shader::Shader() : blending(BLEND_OPACITY)
{
    // create unique id
    id_ = GlmToolkit::uniqueId();

    program_ = &simpleShadingProgram;
    reset();
}


void Shader::operator = (const Shader &S )
{
    color = S.color;
    blending = S.blending;
}

void Shader::accept(Visitor& v) {
    v.visit(*this);
}

void Shader::use()
{
    // initialization on first use
    if (!program_->initialized())
        program_->init();

    // Use program and set uniforms
    program_->use();

    // set uniforms
    program_->setUniform("projection", projection);
    program_->setUniform("modelview", modelview);
    program_->setUniform("color", color);

    iResolution = glm::vec3( Rendering::manager().currentAttrib().viewport, 0.f);
    program_->setUniform("iResolution", iResolution);

    // Blending Function
    if (force_blending_opacity) {
        glEnable(GL_BLEND);
        glBlendEquation(blending_equation[BLEND_OPACITY]);
        glBlendFunc(blending_source_function[BLEND_OPACITY], blending_destination_function[BLEND_OPACITY]);

    }
    else if ( blending != BLEND_CUSTOM ) {
        glEnable(GL_BLEND);
        glBlendEquation(blending_equation[blending]);
        glBlendFunc(blending_source_function[blending], blending_destination_function[blending]);

        // TODO different blending for alpha and color
        //        glBlendEquationSeparate(blending_equation[blending], GL_FUNC_ADD);
        //        glBlendFuncSeparate(blending_source_function[blending], blending_destination_function[blending], GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    }
    else
        glDisable(GL_BLEND);
}


void Shader::reset()
{
    projection = glm::identity<glm::mat4>();
    modelview  = glm::identity<glm::mat4>();
    iResolution = glm::vec3(1280.f, 720.f, 0.f);
    color = glm::vec4(1.f, 1.f, 1.f, 1.f);
}


