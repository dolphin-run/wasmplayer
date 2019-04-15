TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt flat

unix:{
INCLUDEPATH +=  /home/zhangyi/emsdk/emscripten/1.38.22/system/include \
            /home/zhangyi/Desktop/dist/include
}

win32:{

DEFINES += NOEMSCRIPTEN

INCLUDEPATH +=  D:/ffmpeg/include  glwin
LIBS += -LD:/ffmpeg/lib -lavformat -lavcodec -lavutil -llib/glfw3

HEADERS +=  spirit.h easywsclient.hpp glwin/glad/glad.h
SOURCES +=  spirit.cpp easywsclient.cpp glwin/glad/glad.c
}


HEADERS +=  h264decoder.h framecontainer.h
SOURCES +=  main_worked.cpp h264decoder.cpp framecontainer.cpp
