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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "icon.h"
#include "moviec.c"
#ifdef IMGUI_HAS_IMSTR
#define igBegin igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox igCheckbox_Str
#define igColorEdit3 igColorEdit3_Str
#define igButton igButton_Str
#endif
#define UNUSED(x) (void)(x)
#define igGetIO igGetIO_Nil
#define IM_COL32(R, G, B, A)                                                   \
    (((ImU32)(A) << 24) | ((ImU32)(B) << 16) | ((ImU32)(G) << 8) | ((ImU32)(R)))
GLFWwindow* window;

#define tooltip(x)                                                             \
    if (igIsItemHovered(ImGuiHoveredFlags_DelayShort))                         \
    igSetTooltip(x)

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count > 1)
    {

        printf("only one video file is allowed\n");
        return;
    }
     
#define is_video(x) true
    if (!is_video(paths[i]))
    {
        puts("Only video file is allowed");
    }
}
//https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-opengl-users
bool load_texture_from_memory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

bool load_texture_from_file(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = malloc(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = load_texture_from_memory(file_data, file_size, out_texture, out_width, out_height);
    free(file_data);
    return ret;
}
// FIXME:it should diselct when pressed twice
bool toggle_button(const char* label, int* v, int id)
{
    bool is_selected = (*v == id);

    if (is_selected)
    {
        const ImVec4* pressed_color =
            igGetStyleColorVec4(ImGuiCol_ButtonActive);
        igPushStyleColor_Vec4(ImGuiCol_Button, *pressed_color);
    }

    bool clicked = igSmallButton(label);

    if (is_selected)
    {
        igPopStyleColor(1);
    }

    if (clicked)
    {
        *v = id;
    }

    return is_selected;
}

int gcd(int a, int b) {
    return (b == 0) ? a : gcd(b, a % b);
}

ImVec2 get_aspect_ratio(ImVec2 dimension)
{
    int hcf = gcd(dimension.x,dimension.y);
    return (ImVec2){dimension.x/hcf,dimension.y/hcf};
}

ImVec2 fit_image(int square_size,ImVec2 input)
{
    ImVec2 res = get_aspect_ratio(input);
    float ratio = res.x/res.y;

    //fix the width
    float width  = square_size;
    float height = input.x/ratio;

    if(height > square_size)
    {
        //fix the height
        height =  square_size;
        width = square_size*ratio;
    }

    return (ImVec2){width,height};
}

void init()
{

    if (!glfwInit())
        return;

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(
        glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    window = glfwCreateWindow((int)(700 * main_scale), (int)(600 * main_scale),
                              "VPU", NULL, NULL);
    if (!window)
    {
        printf("Failed to create window! Terminating!\n");
        glfwTerminate();
        return;
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
    // ioptr->ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    // ioptr->ConfigFlags |=
    // ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform
    // Windows
#endif
    // ioptr->MouseDrawCursor = true;
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
    ImFont* default_font = ImFontAtlas_AddFontFromFileTTF(
        ioptr->Fonts, "./font.ttf", 0, NULL, NULL);
    ioptr->FontDefault = default_font;

    bool quit = false;
    UNUSED(quit);
}

typedef struct
{
    bool rotate;
    bool fliph;
    bool filpv;
    bool strip_audio;
    bool stereo_to_mono;
} SingleClickMenu;

void show_single_click_menu(SingleClickMenu* m)
{
    igBeginGroup();

    igCheckbox("Rotate", &m->rotate);
    tooltip("Rotate the video");
    if (m->rotate)
    {
        static int e = 0;

        // igPushFont(icon_font,40);

        toggle_button(icon_rotate_90, &e, 0);
        tooltip("Rotate the video by 90degrees");
        igSameLine(0.0f, -1.0f);

        toggle_button(icon_rotate_180, &e, 1);
        tooltip("Rotate the video by 180degrees");
        igSameLine(0.0f, -1.0f);

        toggle_button(icon_rotate_270, &e, 2);
        igSameLine(0.0f, -1.0f);

        toggle_button(icon_rotate_custom, &e, 3);

        // igPopFont();

        if (e == 3)
        {
            static int value = 42;
            const int min_val = -360;
            const int max_val = 360;
            igSetNextItemWidth(150.0f);
            if (igInputInt("Angle", &value, 1, 5, 0))
            {
                if (value < min_val)
                    value = min_val;
                if (value > max_val)
                    value = max_val;
            }
        }
    }

    igCheckbox("Flip Horizontally", &m->fliph);
    igCheckbox("Flip Vertically", &m->filpv);
    igCheckbox("Stereo to mono", &m->stereo_to_mono);

    igCheckbox("Strip Audio", &m->strip_audio);
    tooltip("Remove audio from the video");
    {
        static bool aspect_ratio = false;
        igCheckbox("Aspect Ratio", &aspect_ratio);
        if (aspect_ratio)
        {
            igText("Work in progress");
        }
    }

    igEndGroup();
}

void show_preview_window()
{
    #define MAX_IMAGE_DIMENSION 300

    static bool is_initilise = false;
    static GLuint texture;
    static int width, height;

    if (!is_initilise)
    {
        size_t outlen;
        const int ss = 300;
        unsigned char* data = video_get_thumbnail("/home/user/Videos/input.mp4",&outlen);
        if(!outlen) puts("Unable to load thumbnail");
        load_texture_from_memory(data, outlen, &texture, &width, &height);
        ImVec2 res = fit_image(MAX_IMAGE_DIMENSION, (ImVec2){width,height});
        width = res.x;
        height = res.y;
        is_initilise = true;
        free(data);
    }

    igBeginGroup();
    ImTextureRef tex_ref = {._TexData = NULL,
                            ._TexID = (ImTextureID)(intptr_t)texture};
    igImage(tex_ref, (ImVec2){width, height}, (ImVec2){0, 0},
            (ImVec2){1, 1});
    if (igBeginTable("video_info", 2,
                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg,
                     (ImVec2){400, 0}, 0.0f))
    {

        igTableSetupColumn(NULL, ImGuiTableColumnFlags_WidthFixed, 100.0f, 0);

        igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
        igTableSetColumnIndex(0);
        igText("Location");
        igTableSetColumnIndex(1);
        igText("~/video/test.mp4");

        igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
        igTableSetColumnIndex(0);
        igText("width");
        igTableSetColumnIndex(1);
        igText("900");

        igEndTable();
    }

    igEndGroup();
}
int main(int argc, char* argv[])
{
    init();


    glfwSetDropCallback(window, drop_callback);

    ImVec4 clearColor;
    clearColor.x = 0.11f;
    clearColor.y = 0.11f;
    clearColor.z = 0.11f;
    clearColor.w = 1.00f;

    ImGuiIO* ioptr = igGetIO();

    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;
    style->WindowRounding = 1.0f;
    style->FrameRounding = 3.0f;
    style->ScrollbarRounding = 3.0f;
    style->GrabRounding = 3.0f;
    style->ImageBorderSize = 4;
    style->TabRounding = 0.0f;
    colors[ImGuiCol_WindowBg] = (ImVec4){0.1f, 0.1f, 0.15f, 1.0f};
    colors[ImGuiCol_Header] = (ImVec4){0.2f, 0.2f, 0.3f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = (ImVec4){0.3f, 0.3f, 0.4f, 1.0f};
    colors[ImGuiCol_HeaderActive] = (ImVec4){0.4f, 0.4f, 0.5f, 1.0f};
    colors[ImGuiCol_Button] = (ImVec4){0.2f, 0.3f, 0.5f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = (ImVec4){0.3f, 0.4f, 0.6f, 1.0f};
    colors[ImGuiCol_ButtonActive] = (ImVec4){0.4f, 0.5f, 0.7f, 1.0f};
    ImFont* icon_font = ImFontAtlas_AddFontFromFileTTF(
        ioptr->Fonts, "/home/user/programming/guis/vpu_python/asset/icon.ttf",
        16.0f, NULL, NULL);
    SingleClickMenu m = {0};
    while (!glfwWindowShouldClose(window))
    {

        glfwPollEvents();
        // start imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        {

            ImGuiViewport* viewport = igGetMainViewport();
            igSetNextWindowPos(viewport->Pos, ImGuiCond_Always, (ImVec2){0, 0});
            igSetNextWindowSize(viewport->Size, ImGuiCond_Always);

            igBegin("main_window", NULL,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoTitleBar);
            {

                igBeginTabBar("main_tab", 0);
                {
                    if (igBeginTabItem("Single Click", NULL, 0))
                    {
                        show_preview_window();
                        igSameLine(0.0f, -1.0f);
                        show_single_click_menu(&m);

                        static bool input;
                        static char data[2048];

                        igDummy((ImVec2){0, 20});

                        igBeginGroup();
                        igCheckbox("Output dir same as input dir", &input);

                        if (!input)
                        {

                            igInputTextEx("##",
                                          "Output file",
                                          data,
                                          sizeof(data),
                                          (ImVec2){viewport->Size.x - 110, 0},
                                          0,
                                          NULL,
                                          NULL
                                          );
                            igSameLine(0.0f, -1.0f);

                            if (igButton("Browse", (ImVec2){0}))
                            {
                            }
                        }
                        igEndGroup();
                        igEndTabItem();
                    }

                    if (igBeginTabItem("Advance", NULL, 0))
                    {

                        igEndTabItem();
                    }

                    if (igBeginTabItem("Remix", NULL, 0))
                    {

                        igEndTabItem();
                    }
                    igEndTabBar();
                }
                igEnd();
            }
        }
        // ImVec2 pos;
        // igGetWindowPos(&pos);
        // pos.x += 43;
        // pos.y += 34;
        // igSetNextWindowPos(pos, 0, (ImVec2){0, 0});
        static bool hidden = true;
        if (!hidden && igBegin("Preview", NULL, 0))
        {

            igBeginTabBar("maadsf", 0);
            {
                if (igBeginTabItem("asf", NULL, 0))
                {

                    igText("s900");
                    igEndTabItem();
                }
                igEndTabBar();
            }

            igEnd();
        }

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
