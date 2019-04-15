#ifdef NOEMSCRIPTEN
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <easywsclient.hpp>
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#else
#include <emscripten/emscripten.h>

#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>
#endif

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "framecontainer.h"
#include <chrono>

const int VIDEO_PIXEL_WIDTH = 1920;
const int VIDEO_PIXEL_HEIGHT = 1080;

struct SYUVInfo
{
    SYUVInfo(int pixWid = VIDEO_PIXEL_WIDTH, int pixHei = VIDEO_PIXEL_HEIGHT);
    bool update(int pixWid, int pixHei);
    bool setViewSize(int pixWid, int pixHei);

    unsigned m_wid;
    float m_semiWid;
    unsigned m_hei;
    float m_semiHei;
    unsigned m_len;

    unsigned m_yWid;
    unsigned m_yHei;
    unsigned m_uWid;
    unsigned m_uHei;

    unsigned m_yLen;
    unsigned m_uLen;
    float m_yuvRadio;

    int m_viewWid;
    int m_viewHei;
    float m_renderScale;
};
SYUVInfo::SYUVInfo(int pixWid, int pixHei)
{
    update(pixWid, pixHei);
    setViewSize(pixWid, pixHei);
}

bool SYUVInfo::update(int pixWid, int pixHei)
{
    m_wid = pixWid;
    m_hei = pixHei;
    m_semiWid = m_wid >> 1;
    m_semiHei = m_hei >> 1;

    m_yWid = pixWid;
    m_yHei = pixHei;
    m_uWid = (pixWid + 1) / 2;
    m_uHei = (pixHei + 1) / 2;

    m_yLen = m_yWid*m_yHei;
    m_uLen = m_uWid*m_uHei;
    m_len = m_yLen + m_uLen * 2;

    m_yuvRadio = m_semiHei / m_semiWid;

    return true;
}

bool SYUVInfo::setViewSize(int pixWid, int pixHei)
{
    m_viewWid = pixWid;
    m_viewHei = pixHei;

    //adapt to render pixels
    m_renderScale = std::min(float(m_viewWid) / m_wid, float(m_viewHei) / m_hei);
    return true;
}

//*******
//
static FrameContainer *m_frames = nullptr;

//
static SYUVInfo* m_yuvInfo = nullptr;


extern "C"  {
int initVideoStream()
{
    m_yuvInfo = new SYUVInfo;
    m_frames = new FrameContainer(m_yuvInfo->m_len, 8);
    return 0;
}

int putVideoStream(uint8_t* src, int sz)
{
    bool stat = m_frames->produce(src, sz);
    if (!stat)
    {
        printf("skip#########\n");
    }
    return 1;
}

}
//***********


static GLuint m_textureY = 0, m_textureU = 0, m_textureV = 0;
static GLuint texUniY = 0, texUniU = 0, texUniV = 0;
GLFWwindow* window;

void bindVideoBuf(const uint8_t *yuvBuf)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    char * ptr = (char *)yuvBuf;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuvInfo->m_yWid, m_yuvInfo->m_yHei
        , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ptr);
    glUniform1i(texUniY, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    ptr += m_yuvInfo->m_yLen;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuvInfo->m_uWid, m_yuvInfo->m_uHei
        , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ptr);
    glUniform1i(texUniU, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    ptr += m_yuvInfo->m_uLen;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_yuvInfo->m_uWid, m_yuvInfo->m_uHei
        , 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, ptr);
    glUniform1i(texUniV, 2);
}

void initTexture()
{
    glGenTextures(1, &m_textureY);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_textureU);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_textureV);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    glEnable(GL_TEXTURE_2D);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource =
"attribute vec3 aPos;\n"
"attribute vec2 aTextureCoord;\n"
"varying vec2 vTextureCoord;\n"
"void main()\n"
"{\n"
"   vTextureCoord = aTextureCoord;\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
const char *fragmentShaderSource =
"precision mediump float;\n"
"varying vec2 vTextureCoord;\n"
"uniform sampler2D tex_y;\n"
"uniform sampler2D tex_u;\n"
"uniform sampler2D tex_v;\n"
"void main()\n"
"{\n"
"   vec3 yuv;\n"
"   vec3 rgb;\n"
"   yuv.x = texture2D(tex_y, vTextureCoord).r;\n"
"   yuv.y = texture2D(tex_u, vTextureCoord).r - 0.5;\n"
"   yuv.z = texture2D(tex_v, vTextureCoord).r - 0.5;\n"
"   \n"
"   rgb = mat3(1.0, 1.0, 1.0,\n"
"       0.0, -0.395, 2.032,\n"
"       1.140, -0.581, 0.0) * yuv;\n"
"   \n"
"   gl_FragColor = vec4(rgb, 1.0);\n"
//"   gl_FragColor = texture(tex_y, vTextureCoord);\n"
"}\n\0";

extern "C"
int mainInit()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "WASM OpenGL", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

#ifdef NOEMSCRIPTEN
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
#endif // !EMSCRIPTEN


    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n %s \n", infoLog);
    }
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n %s \n", infoLog);
    }
    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n %s \n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glUseProgram(shaderProgram);
    texUniY = glGetUniformLocation(shaderProgram, "tex_y");
    texUniU = glGetUniformLocation(shaderProgram, "tex_u");
    texUniV = glGetUniformLocation(shaderProgram, "tex_v");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    GLfloat vertices[] = {
        -1.f, -1.f, 0.0f, 0.f, 1.f, // left-bottom  
        1.f, -1.f, 0.0f, 1.f, 1.f,// right-bottom
        1.f,  1.f, 0.0f, 1.f, 0.f,  // right-top
        -1.f, -1.f, 0.0f, 0.f, 1.f, // left-bottom
        1.f,  1.f, 0.0f, 1.f, 0.f,  // right-top    
        -1.f,  1.f, 0.0f, 0.f, 0.f // left-top 
    };

    GLuint VBO;
    //glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    //glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    initTexture();

    return 0;
}

extern "C"
int mainPaint()
{
    // render
    // ------
    glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw our first triangle
    //glUseProgram(shaderProgram);
    //glBindTexture(GL_TEXTURE_2D, m_textureY);
    //printf("gldraw...%d\n", yuvBufUpdate);
    int yuvBufUpdate = 0;
    auto buf = m_frames->consume(&yuvBufUpdate);
    if (buf)
    {
        if (yuvBufUpdate)
        {
            //printf("yuvBufUpdate bind...\n");
            bindVideoBuf(buf);
        }
        m_frames->consumeFinished();
    }

    //glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // glBindVertexArray(0); // no need to unbind it every time

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();

    //emscripten_sleep(40);
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

#ifdef NOEMSCRIPTEN
int main()
{
    initVideoStream();
    mainInit();

    using easywsclient::WebSocket;
    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        printf("WSAStartup Failed.\n");
        return 1;
    }
    WebSocket *ws = WebSocket::from_url("ws://10.64.29.1:12345");
    if (!ws) return 1;

    bool dblHit = true;
    while (!glfwWindowShouldClose(window))
    {
        dblHit = !dblHit;

        if (dblHit)
        {
            ws->poll();
            ws->send("hello");
            ws->dispatch([ws](const std::string & message) {
                //printf(">>> %d\n", message.size());
                putVideoStream((uint8_t*)message.data(), message.size());
                //pdec->putVideoStream((uint8_t*)message.data(), message.size());
            });
        }

        processInput(window);
        mainPaint();
        Sleep(20);
    }

    return 1;
}
#endif
