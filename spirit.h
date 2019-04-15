#pragma once
#include <glad/glad.h>
#include <glm/matrix.hpp>
#include <string>
using namespace std;

struct SShaderDesc
{
    string vs;
    string fs;
    int sid;
};

struct STextureInfo
{
    void *pixel;
    int width, height;
    GLint pixfmt;
};

struct SRenderContext
{

};

struct SSpirit
{
    //restore shader
    int sid;
    int textureNum;
    STextureInfo *texInfo;
    GLuint texture0;

    glm::mat4 model;
};