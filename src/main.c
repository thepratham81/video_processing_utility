#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <GL/gl.h>

#ifdef IMGUI_HAS_IMSTR
#define igBegin igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox igCheckbox_Str
#define igColorEdit3 igColorEdit3_Str
#define igButton igButton_Str
#endif

#define igGetIO igGetIO_Nil

GLFWwindow* window;

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    for (int i = 0; i < count; i++)
    {
        printf("Dropped file: %s\n", paths[i]);
    }
}

bool toggle_button(const char* label, int *v, int id)
{
    bool is_selected = (*v == id);
    
    if (is_selected) {
        const ImVec4* pressed_color = igGetStyleColorVec4(ImGuiCol_ButtonActive);
        igPushStyleColor_Vec4(ImGuiCol_Button, *pressed_color);
    }
    
    bool clicked = igSmallButton(label);
    
    if (is_selected) {
        igPopStyleColor(1);
    }
    
    if (clicked) {
        *v = id;
    }
    
    return is_selected;
}
int main(int argc, char* argv[])
{

    if (!glfwInit())
        return -1;

    // Decide GL+GLSL versions
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
#else
    // GL 3.2 + GLSL 130
    const char* glsl_version = "#version 130";
#endif

    // just an extra window hint for resize
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(
        glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale),
                              "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);

    if (!window)
    {
        printf("Failed to create window! Terminating!\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // enable vsync
    glfwSwapInterval(1);

    // check opengl version sdl uses
    printf("opengl version: %s\n", (char*)glGetString(GL_VERSION));

    // setup imgui
    igCreateContext(NULL);

    // set docking
    ImGuiIO* ioptr = igGetIO();
    ioptr->ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable
    // Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    ioptr->ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    ioptr->ConfigFlags |=
        ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform
                                          // Windows
#endif

    // Setup scaling
    ImGuiStyle* style = igGetStyle();
    ImGuiStyle_ScaleAllSizes(
        style, main_scale); // Bake a fixed style scale. (until we have a
                            // solution for dynamic style scaling, changing this
                            // requires resetting Style + calling this again)
    style->FontScaleDpi =
        main_scale; // Set initial font scale. (using
                    // io.ConfigDpiScaleFonts=true makes this unnecessary. We
                    // leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
    ioptr->ConfigDpiScaleFonts =
        true; // [Experimental] Automatically overwrite style.FontScaleDpi in
              // Begin() when Monitor DPI changes. This will scale fonts but
              // _NOT_ scale sizes/padding for now.
    ioptr->ConfigDpiScaleViewports =
        true; // [Experimental] Scale Dear ImGui and Platform Windows when
              // Monitor DPI changes.
#endif
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    igStyleColorsDark(NULL);
    ImFont *defalut =  ImFontAtlas_AddFontFromFileTTF(ioptr->Fonts, "./font.ttf", 16.0f, NULL, NULL);
    ImFont *icon    =  ImFontAtlas_AddFontFromFileTTF(ioptr->Fonts, "/home/user/programming/guis/vpu_python/asset/icon.ttf",
                                   16.0f, NULL, NULL);
    #define icon(x) igPushFont(icon,16);igPopFont()
    ImVec4 clearColor;
    clearColor.x = 0.45f;
    clearColor.y = 0.55f;
    clearColor.z = 0.60f;
    clearColor.w = 1.00f;

    // main event loop
    bool quit = false;
    glfwSetDropCallback(window, drop_callback);
   
    bool rotate = false;
    bool fliph  = false;
    bool flipv  = false;
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();

        // start imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        igNewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;

            igBegin("Tools", NULL, 0);
            igCheckbox("Rotate", &rotate);
            if(rotate)
            {
                static int e = 0;

                igPushFont(icon,40);
                
                toggle_button("\ue901",&e,0);igSameLine(0.0f, -1.0f);
                toggle_button("\ue902",&e,1);igSameLine(0.0f, -1.0f);
                toggle_button("\ue903",&e,2);
                igPopFont();


                
            }
            igCheckbox("Flip Horizontally", &fliph);
            igCheckbox("Flip Vertically", &flipv);


        }
        igEnd();


        // render
        igRender();
        glfwMakeContextCurrent(window);
        glViewport(0, 0, (int)ioptr->DisplaySize.x, (int)ioptr->DisplaySize.y);
        glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
#ifdef IMGUI_HAS_DOCK
        if (ioptr->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_window = glfwGetCurrentContext();
            igUpdatePlatformWindows();
            igRenderPlatformWindowsDefault(NULL, NULL);
            glfwMakeContextCurrent(backup_current_window);
        }
#endif
        glfwSwapBuffers(window);
    }

    // clean up
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
