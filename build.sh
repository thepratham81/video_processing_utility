set -xe

g++ -include ./src/common.h -I./cimgui/imgui/ -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o cimgui.cpp.o -c ./cimgui/cimgui.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui.cpp.o -c ./cimgui/imgui/imgui.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui_draw.cpp.o -c ./cimgui/imgui/imgui_draw.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui_demo.cpp.o -c ./cimgui/imgui/imgui_demo.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui_widgets.cpp.o -c ./cimgui/imgui/imgui_widgets.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui_tables.cpp.o -c ./cimgui/imgui/imgui_tables.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11  -o imgui_impl_opengl3.cpp.o -c ./cimgui/imgui/backends/imgui_impl_opengl3.cpp

g++ -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -std=gnu++11 -o imgui_impl_glfw.cpp.o -c ./cimgui/imgui/backends/imgui_impl_glfw.cpp

ar qc libcimgui.a cimgui.cpp.o imgui.cpp.o imgui_draw.cpp.o imgui_demo.cpp.o imgui_widgets.cpp.o imgui_tables.cpp.o imgui_impl_opengl3.cpp.o imgui_impl_glfw.cpp.o

ranlib libcimgui.a

gcc -DCIMGUI_USE_GLFW -DCIMGUI_USE_OPENGL3 -include ./src/common.h -I./cimgui/imgui -I./cimgui -I./cimgui/imgui/examples/libs/gl3w -o main.o -c ./src/main.c

g++ main.o -o vpt libcimgui.a -lGL libglfw3.a -lm -ldl 
