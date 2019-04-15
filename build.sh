em++ h264decoder.cpp main_worked.cpp \
/home/zhangyi/Desktop/dist/lib/libavformat.a /home/zhangyi/Desktop/dist/lib/libavcodec.a /home/zhangyi/Desktop/dist/lib/libavutil.a \
-std=c++11 -o html/hello.html -I/home/zhangyi/Desktop/dist/include \
-s EXPORTED_FUNCTIONS='["_mainInit", "_mainPaint", "_putVideoStream", "_initVideoStream"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
-s USE_GLFW=3  -s TOTAL_MEMORY=268435456 -s ASYNCIFY=1
