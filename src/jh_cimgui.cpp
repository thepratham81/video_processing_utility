#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
#define IMGUI_IMPL_API extern "C" 
#define IMGUI_IMPL_OPENGL_LOADER_GL3W
// #define IMGUI_USER_CONFIG "./external/cimgui/cimconfig.h"
#define IMGUI_DEFINE_MATH_OPERATORS

#include "./external/cimgui/cimconfig.h"
#include "./external/cimgui/imgui/imgui.h"

#include "./external/cimgui/imgui/imgui.cpp"
#include "./external/cimgui/imgui/imgui_demo.cpp"
#include "./external/cimgui/imgui/imgui_draw.cpp"
#include "./external/cimgui/imgui/imgui_tables.cpp"
#include "./external/cimgui/imgui/imgui_widgets.cpp"

#include "./external/cimgui/imgui/backends/imgui_impl_opengl3.cpp"
#include "./external/cimgui/imgui/backends/imgui_impl_glfw.cpp"

#include "./external/cimgui/cimgui.cpp"
