#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#include "tinyfiledialogs.h"
#include <stdlib.h>
#include <string.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>


#include <GL/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "icon.h"
#include "moviec.c"
#include "ui_font.h"
#include "icon_font.h"
#include "tinyfiledialogs.c"

#ifdef IMGUI_HAS_IMSTR
#define igBegin       igBegin_Str
#define igSliderFloat igSliderFloat_Str
#define igCheckbox    igCheckbox_Str
#define igColorEdit3  igColorEdit3_Str
#define igButton      igButton_Str
#endif

#define UNUSED(x) (void)(x)
#define igGetIO igGetIO_Nil

#define IM_COL32(R, G, B, A)                                                   \
    (((ImU32)(A) << 24) | ((ImU32)(B) << 16) | ((ImU32)(G) << 8) | ((ImU32)(R)))

#define THUMBNAIL_SIZE 400
#define WINDOW_WIDTH   700
#define WINDOW_HEIGHT  600

#define jh_divide_space_x2(out, n)                                             \
    do                                                                         \
    {                                                                          \
        ImVec2 available___size;                                               \
        igGetContentRegionAvail(&available___size);                            \
        out.x = (available___size.x) / (float)n;                               \
    } while (0)

#define jh_divide_space_x(out, n)                                              \
    do                                                                         \
    {                                                                          \
        ImVec2 available___size;                                               \
        igGetContentRegionAvail(&available___size);                            \
        float spacing = igGetStyle()->ItemSpacing.x;                           \
        out.x = (available___size.x - (n - 1) * spacing) / (float)n;           \
    } while (0)


GLFWwindow* window;
ImFont*     default_font;        
ImFont*     icon_font;        

#define MAX_BUFFER_SIZE (1<<12)
typedef struct 
{
    GLuint texture;
    char   input_file[MAX_BUFFER_SIZE];
    float  video_length;
    int    width;
    int    height;
    bool  output_dir_same_as_input;
    char output_dir[MAX_BUFFER_SIZE];
}Global_data;

Global_data global_data = {.output_dir_same_as_input=true};

#define arr_len(x) (sizeof(x)/sizeof(x[0]))
#define tool_tip_size 16
#define tooltip(x)                                                             \
    if (igIsItemHovered(ImGuiHoveredFlags_DelayShort))                         \
    {\
        igPushFont(default_font,16);\
        igSetTooltip(x);\
        igPopFont();\
    }

bool   load_texture_from_memory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height);

void split_path(const char *path, char **dir, char **filename);
ImVec2 fit_image(int square_size,ImVec2 input);

#if defined(_WIN32) || defined(_WIN64)

void get_video_folder(char *path) {
    SHGetFolderPathA(NULL, CSIDL_MYVIDEO, NULL, 0, path);
}

void get_executable_path(char* path) {
    GetModuleFileNameA(NULL, path, MAX_BUFFER_SIZE);
}

#else

void get_video_folder(char *path) {
    const char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(path, MAX_BUFFER_SIZE, "%s/Videos", home);
    } else {
        strcpy(path, "./Videos");
    }
}


void get_executable_path(char* path) {
    readlink("/proc/self/exe", path, MAX_BUFFER_SIZE);
}

#endif

char *str_dup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (!d) return 0;
    memcpy(d, s, len);
    return d;
}

void populate_thumbnail(const char* path)
{
    if (global_data.texture)
        glDeleteTextures(1, &global_data.texture);

    char ffmpeg_path[MAX_BUFFER_SIZE];
    char ffprobe_path[MAX_BUFFER_SIZE];

    char exe_dir[MAX_BUFFER_SIZE]; 
    get_executable_path(exe_dir);
    char*dir;
    char*file;
    split_path(exe_dir, &dir, &file);
    #ifdef _WIN32

        snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%s\\ffmpeg.exe",dir);
        snprintf(ffprobe_path, sizeof(ffprobe_path), "%s\\ffprobe.exe",dir);
    #else
        snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%s/ffmpeg",dir);
        snprintf(ffprobe_path, sizeof(ffprobe_path), "%s/ffprobe",dir);
    #endif
    free(dir);
    free(file);
    size_t outlen;
    const int ss = 300;
    unsigned char* data =
        video_get_thumbnail(ffmpeg_path,path,&outlen);

    if(!outlen) puts("Unable to load thumbnail");

    load_texture_from_memory(data, outlen, &global_data.texture, &global_data.width,&global_data.height);
    ImVec2 res      = fit_image(THUMBNAIL_SIZE, (ImVec2){global_data.width,global_data.height});
    global_data.width  = res.x;
    global_data.height = res.y;
    strncpy(global_data.input_file,path,sizeof(global_data.input_file)-1);
    global_data.input_file[sizeof(global_data.input_file)-1] = '\0';
    global_data.video_length = video_get_duration(ffprobe_path,global_data.input_file);
    free(data);
}

void   drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count > 1)
    {

        printf("only one video file is allowed\n");
        return;
    }
     
#define is_video(x) true
    if (!is_video(paths[0]))
    {
        puts("Only video file is allowed");
        return;
    }
    populate_thumbnail(paths[0]);
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

bool jh_chk_button(const char* label, bool* status,ImVec2 size)
{
    if(*status)
    {
        igPushStyleColor_Vec4(ImGuiCol_Button,
                              *igGetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    bool clicked = igButton(label,size);
    // bool clicked = igSmallButton(label);
    if(*status)
    {
      igPopStyleColor(1);
    }
    if(clicked)
    {
        *status = !(*status);
    }
    return *status;
}

bool jh_radio_button(const char* label, int* v, int id, ImVec2 size)
{
    bool is_selected = (*v == id);
    
    if (is_selected)
    {
        const ImVec4* pressed_color = igGetStyleColorVec4(ImGuiCol_ButtonActive);
        igPushStyleColor_Vec4(ImGuiCol_Button, *pressed_color);
    }
    
    bool clicked = igButton(label, size);
    
    if (is_selected)
    {
        igPopStyleColor(1);
    }
    
    if (clicked)
    {
        if(is_selected)
        {
            *v = -1;
            return false;
        }
        *v = id;
        return true;
    }
    
    return false;
}

int gcd(int a, int b) {
    return (b == 0) ? a : gcd(b, a % b);
}

ImVec2 get_aspect_ratio(ImVec2 dimension)
{
    int hcf = gcd(dimension.x,dimension.y);
    return (ImVec2){dimension.x/hcf,dimension.y/hcf};
}

ImVec2 fit_image(int square_size, ImVec2 input)
{
    ImVec2 res   = get_aspect_ratio(input);
    float ratio  = res.x/res.y;
    
    float width  = square_size;
    float height = width/ratio;
    
    if(height > square_size)
    {
        height   = square_size;
        width    = height * ratio; 
    }
    
    return (ImVec2){width, height};
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(
        glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    window = glfwCreateWindow((int)(WINDOW_WIDTH * main_scale), (int)(WINDOW_HEIGHT* main_scale),
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

    default_font = ImFontAtlas_AddFontFromMemoryTTF(ioptr->Fonts, font_ttf, font_ttf_len, 0, NULL, NULL);
    ioptr->FontDefault = default_font;

    icon_font = ImFontAtlas_AddFontFromMemoryTTF(ioptr->Fonts, icon_ttf, icon_ttf_len, 0, NULL, NULL);

    bool quit = false;
    UNUSED(quit);
}

typedef struct
{
    bool  fliph;
    bool  flipv;
    bool  strip_audio;
    bool  stereo_to_mono;
    int   volume;
    float angle;
} SingleClickMenu;

typedef struct 
{
    bool is_rendring; 
    bool stop_rendering;
    float progress;
    void(*callback)(VideoProgress*,void*);
    SingleClickMenu* options;
}RenderWidgetData;

void render_callback(VideoProgress* vp,void* arg)
{
    RenderWidgetData* user_data = arg;
    user_data->is_rendring = true;
    float total_processed_sec =  (vp->total_processed/1000000.0f);
    user_data->progress = total_processed_sec/global_data.video_length;

    if(vp->is_finished)
    {

        user_data->progress = 1.0f;
        user_data->is_rendring = false;
    }
}

void split_path(const char *path, char **dir, char **filename) {
    // Find the last occurrence of '/' or '\'
    const char *last_slash = strrchr(path, '/');
    const char *last_backslash = strrchr(path, '\\');

    // Use whichever comes last (for cross-platform compatibility)
    const char *separator = (last_slash > last_backslash) ? last_slash : last_backslash;

    if (separator == NULL) {
        // No separator found, entire path is filename
        *dir = str_dup(".");
        *filename = str_dup(path);
    } else {
        // Split at the separator
        size_t dir_len = separator - path;
        *dir = malloc(dir_len + 1);
        strncpy(*dir, path, dir_len);
        (*dir)[dir_len] = '\0';

        *filename = str_dup(separator + 1);
    }
}
const char* get_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return ""; 
    }
    return dot;
}

void *jh_render_video(void *arg)
{

    RenderWidgetData* user_data = arg;

    user_data->progress = 0;
    SingleClickMenu opt= *user_data->options;
    Video v;
    video_init(&v,global_data.input_file);
    if(opt.fliph) video_fliph(&v);
        
    video_scale_volume(&v,10);
    if(opt.flipv) video_flipv(&v);
    if(opt.strip_audio) video_remove_audio(&v);
    if(opt.angle!=0.0f) video_rotate(&v,opt.angle);
    if(opt.volume!=100) video_scale_volume(&v, opt.volume);
    if(opt.stereo_to_mono) video_stereo_to_mono(&v);
    char output[MAX_BUFFER_SIZE]={0};
    char time_stamp[100] = {0};
    const char* ext = get_extension(global_data.input_file);

    time_t t;
    struct tm *tm_info;

    t = time(NULL);
    tm_info = localtime(&t);

    strftime(time_stamp,sizeof(time_stamp), "%Y-%m-%d_%H.%M.%S", tm_info);
    
    if(global_data.output_dir_same_as_input)
    {
        snprintf(output, sizeof(output),"%s_%s%s",global_data.input_file,time_stamp,ext);
    }
    else
    {
        char* input_dir = NULL;
        char* file_name = NULL;
        split_path(global_data.input_file,&input_dir,&file_name);
        snprintf(output, sizeof(output), "%s/%s_%s%s",global_data.output_dir,file_name,time_stamp,ext);
        free(input_dir);
        free(file_name);
    }
    char ffmpeg_path[MAX_BUFFER_SIZE];
    char exe_dir[MAX_BUFFER_SIZE]; 
    get_executable_path(exe_dir);
    char*dir;
    char*file;
    split_path(exe_dir, &dir, &file);

    #ifdef _WIN32

        snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%s\\ffmpeg.exe",dir);
    #else
        snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%s/ffmpeg",dir);
    #endif

    free(dir);
    free(file);
    video_render(ffmpeg_path,&v,output,user_data->callback,user_data,&user_data->stop_rendering);
    user_data->is_rendring = false;
    user_data->progress = 0;
    return NULL;
}
void show_render(SingleClickMenu* single_click_menu,ImVec2 size)
{

    static RenderWidgetData user_data = {.callback = render_callback,
                                         .stop_rendering = false,
                                         .progress = 0.0f,
                                         .is_rendring = false,
                                        };
    size.x -= igGetStyle()->WindowPadding.x;
    size.x -= igGetStyle()->ItemSpacing.x;
    if(user_data.is_rendring)
    {

        size.x -= igGetStyle()->WindowPadding.x;
        float button_size = 0.2*size.x; // 80:20
        size.x -= button_size;
        char tmp[100];
        snprintf(tmp, sizeof(tmp), "%f",user_data.progress*100);
        igProgressBar(user_data.progress,(ImVec2){size.x},tmp);
        igSameLine(0.0f, -1.0f);
        if(igButton("Cancle",(ImVec2){button_size}))
        {
            user_data.stop_rendering = true;
        }

        return;
    }
    
    igBeginDisabled(!global_data.texture);
    if(igButton("Render",(ImVec2){size.x}))
    {
        static pthread_t t;

        user_data.options = single_click_menu,
        pthread_create(&t, NULL, jh_render_video, (void*)&user_data);
    }
    igEndDisabled();


}
void show_single_click_menu(SingleClickMenu* m)
{
    #define big_icon_size 70
    #define small_icon_size 35

    enum RotateOption 
    {
        ROTATE_NONE = -1,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270,
        ROTATE_CUSTOM
    };

    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.45f, 0.45f, 0.45f, 1.0f}); 
    ImVec2 size = {0};
    igBeginGroup();

    static int select_radio_rotate = -1;
    static int custom_angle = 42;
    igPushFont(icon_font,small_icon_size);
    jh_divide_space_x(size,4);

    if(jh_radio_button(icon_rotate_90, &select_radio_rotate,ROTATE_90,size)) m->angle = 90;
    tooltip("Rotate the video by 90°");
    igSameLine(0.0f, -1.0f);

    if(jh_radio_button(icon_rotate_180, &select_radio_rotate,ROTATE_180,size)) m->angle = 190;
    tooltip("Rotate the video by 180°");
    igSameLine(0.0f, -1.0f);

    if(jh_radio_button(icon_rotate_270, &select_radio_rotate,ROTATE_270,size)) m->angle = 270;
    tooltip("Rotate the video by 270°");
    igSameLine(0.0f, -1.0f);

    if(jh_radio_button(icon_rotate_custom, &select_radio_rotate,ROTATE_CUSTOM,size))m->angle = custom_angle;
    tooltip("Rotate by custom angle");

    switch(select_radio_rotate)
    {
        case ROTATE_90:
        {
          m->angle = 90;
          break;
        }

        case ROTATE_180:
        {
          m->angle = 180;
          break;
        }

        case ROTATE_270:
        {
          m->angle = 270;
          break;
        }
        case ROTATE_NONE:
        {
            m->angle = 0;
            break;
        }
    }
    igPopFont();

    if (select_radio_rotate == ROTATE_CUSTOM)
    {
        const int min_val = -360;
        const int max_val = 360;
        igSetNextItemWidth(150.0f);
        if (igInputInt("Angle", &custom_angle, 1, 5, 0))
        {
            if (custom_angle < min_val)
                custom_angle = min_val;
            if (custom_angle > max_val)
                custom_angle = max_val;
        }
        m->angle = custom_angle;
    }


    igPushFont(icon_font,big_icon_size);
    jh_divide_space_x(size, 2);
    jh_chk_button(icon_fliph, &m->fliph,size);igSameLine(0.0f, -1.0f);
    tooltip("Flip video horizontally");

    jh_chk_button(icon_flipv, &m->flipv,size);//igSameLine(0.0f, -1.0f);
    tooltip("Flip video Vertically");
    igPopFont();

    igPushFont(icon_font,small_icon_size);
    jh_divide_space_x(size,4);

    static int group_volume = -1 ;
    jh_radio_button(icon_50_up,  &group_volume,0 ,size);igSameLine(0.0f, -1.0f);
    tooltip("Increase Volume by 50%%");

    jh_radio_button(icon_50_down,&group_volume,1 ,size);igSameLine(0.0f, -1.0f);
    tooltip("Decrease Volume by 50%%");

    jh_radio_button(icon_25_up,  &group_volume,2 ,size);igSameLine(0.0f, -1.0f);
    tooltip("Increase Volume by 25%%");

    jh_radio_button(icon_25_down,&group_volume,3 ,size);//igSameLine(0.0f, -1.0f);
    tooltip("Decrease Volume by 25%%");
    igPopFont();
    
    switch(group_volume)
    {
        case 0:
            m->volume = 150.0f;break;
        case 1:
            m->volume = 50.0f;break;
        case 2:
            m->volume = 125.0f;break;
        case 3:
            m->volume = 75.0f;break;
        default:
            m->volume = 100.0f;
    }



    igPushFont(icon_font,big_icon_size);

    jh_divide_space_x(size,2);

    jh_chk_button(icon_stero_to_mono, &m->stereo_to_mono,size);igSameLine(0.0f, -1.0f);
    tooltip("Stereo to mono");

    jh_chk_button(icon_mute, &m->strip_audio,size);
    tooltip("Remove audio from video");

    igPopFont();

    // igCheckbox("Strip Audio", &m->strip_audio);
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
    igPopStyleColor(1); // The parameter is how many colors to pop
}

//this funtion is generated by claude
void jh_thumbnail(GLuint texture, ImVec2 size, int width, int height, 
                   float rotation_degrees,bool vflip,bool hflip)
{
    if (igInvisibleButton("##thumbnail", size, 0))
    {
        printf("Invisible button was clicked!\n");
    }

    ImDrawList* draw_list = igGetWindowDrawList();
    ImVec2 button_min, button_max;
    igGetItemRectMin(&button_min);
    igGetItemRectMax(&button_max);
    ImDrawList_AddRectFilled(draw_list, button_min, button_max, IM_COL32(40, 40, 40, 255), 0, 0);
    
    // Calculate centered image position
    float dx = (size.x - width) / 2.0f;
    float dy = (size.y - height) / 2.0f;
    
    ImVec2 image_min = {button_min.x + dx, button_min.y + dy};
    ImVec2 image_max = {image_min.x + width, image_min.y + height};
    
    // Handle UV coordinates for flipping
    ImVec2 uv_min = {hflip ? 1.0f : 0.0f, vflip ? 1.0f : 0.0f};
    ImVec2 uv_max = {hflip ? 0.0f : 1.0f, vflip ? 0.0f : 1.0f};
    
    ImTextureRef tex_ref = {._TexData = NULL, ._TexID = (ImTextureID)texture};
    
    // If no rotation, use simple AddImage
    if (rotation_degrees == 0.0f)
    {
        ImDrawList_AddImage(draw_list, tex_ref, image_min, image_max,
                           uv_min, uv_max, 0xFFFFFFFF);
    }
    else
    {
        // Calculate rotation
        float angle_rad = rotation_degrees * (3.14159265f / 180.0f);
        float cos_a = cosf(angle_rad);
        float sin_a = sinf(angle_rad);
        
        // Calculate center of the image
        ImVec2 center = {
            image_min.x + width * 0.5f,
            image_min.y + height * 0.5f
        };
        
        // Calculate half dimensions
        float half_w = width * 0.5f;
        float half_h = height * 0.5f;
        
        // Calculate rotated corners around center
        ImVec2 p1, p2, p3, p4;
        
        // Top-left corner
        float x1 = -half_w, y1 = -half_h;
        p1.x     = center.x + (x1 * cos_a - y1 * sin_a);
        p1.y     = center.y + (x1 * sin_a + y1 * cos_a);
        
        // Top-right corner
        float x2 = half_w, y2  = -half_h;
        p2.x     = center.x + (x2 * cos_a - y2 * sin_a);
        p2.y     = center.y + (x2 * sin_a + y2 * cos_a);
        
        // Bottom-right corner
        float x3 = half_w, y3  = half_h;
        p3.x     = center.x + (x3 * cos_a - y3 * sin_a);
        p3.y     = center.y + (x3 * sin_a + y3 * cos_a);
        
        // Bottom-left corner
        float x4 = -half_w, y4 = half_h;
        p4.x     = center.x + (x4 * cos_a - y4 * sin_a);
        p4.y     = center.y + (x4 * sin_a + y4 * cos_a);
        
        // UV coordinates for the quad (accounting for flips)
        ImVec2 uv1 = {hflip ? 1.0f : 0.0f, vflip ? 1.0f : 0.0f};  // top-left
        ImVec2 uv2 = {hflip ? 0.0f : 1.0f, vflip ? 1.0f : 0.0f};  // top-right
        ImVec2 uv3 = {hflip ? 0.0f : 1.0f, vflip ? 0.0f : 1.0f};  // bottom-right
        ImVec2 uv4 = {hflip ? 1.0f : 0.0f, vflip ? 0.0f : 1.0f};  // bottom-left
        
        ImDrawList_AddImageQuad(draw_list, tex_ref, p1, p2, p3, p4,
                               uv1, uv2, uv3, uv4, 0xFFFFFFFF);
    }
}
void show_output_section(ImVec2 size)
{

    igBeginGroup();
    igCheckbox("Output dir same as input dir", &global_data.output_dir_same_as_input);
    size.x -= 2*igGetStyle()->WindowPadding.x;
    size.x -= igGetStyle()->ItemSpacing.x;
    float button_size = 0.2*size.x; // 80:20
    size.x -= button_size;

    if (!global_data.output_dir_same_as_input)
    {

        igInputTextEx("##", "Output file", global_data.output_dir, sizeof(global_data.output_dir), size,
                      0, NULL, NULL);
        igSameLine(0.0f, -1.0f);

        if (igButton("Browse", (ImVec2){button_size}))
        {
            char const* selected_folder = tinyfd_selectFolderDialog(
            "Output Directory","");
            if(selected_folder)
            {
                strncpy(global_data.output_dir,selected_folder,sizeof(global_data.output_dir)-1);
                global_data.output_dir[sizeof(global_data.output_dir)-1] = '\0';
            }
        }
    }
    igEndGroup();
}
void show_preview_window(int angle,bool fliph,bool flipv)
{


    igBeginGroup();
    if (global_data.texture)
    {
        jh_thumbnail(global_data.texture,
                       (ImVec2){THUMBNAIL_SIZE, THUMBNAIL_SIZE}, global_data.width,
                       global_data.height, angle, flipv, fliph);
        if(igButton("Remove Video",(ImVec2){0}))
        {
            glDeleteTextures(1,&global_data.texture);
            global_data.texture = 0;
        }
    }
    else
    {
        if (igButton("Drag and Drop\n  or click",
                     (ImVec2){THUMBNAIL_SIZE, THUMBNAIL_SIZE}))
        {

            char const* supported_file[] = {"*.mp4", "*.avi", "*.mkv", "*.mov",
                                             "*.wmv", "*.flv", "*.webm"};
            const int total_pattern = arr_len(supported_file);
            char const* selected_file = tinyfd_openFileDialog(
                "Select a video file", 
                "",            
                total_pattern, 
                supported_file, 
                "Video Files", 
                0              
            );
            if(selected_file)
            {
                populate_thumbnail(selected_file);
            }

        }
    }

    #if 0
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
    #endif
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
    style->WindowRounding    = 1.0f;
    style->FrameRounding     = 12.0f;
    style->ScrollbarRounding = 3.0f;
    style->GrabRounding      = 3.0f;
    style->ImageBorderSize   = 4;
    style->TabRounding       = 12.0f;
    style->FramePadding.y    = 10;
    style->FramePadding.x    = 10;

    colors[ImGuiCol_WindowBg]      = (ImVec4){0.1f, 0.1f, 0.15f, 1.0f};
    colors[ImGuiCol_Header]        = (ImVec4){0.2f, 0.2f, 0.3f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = (ImVec4){0.3f, 0.3f, 0.4f, 1.0f};
    colors[ImGuiCol_HeaderActive]  = (ImVec4){0.4f, 0.4f, 0.5f, 1.0f};
    colors[ImGuiCol_Button]        = (ImVec4){0,0,0,1};
    colors[ImGuiCol_ButtonHovered] = (ImVec4){0.3f, 0.4f, 0.6f, 1.0f};
    colors[ImGuiCol_ButtonActive]  = (ImVec4){0.4f, 0.5f, 0.7f, 1.0f};
    colors[ImGuiCol_TabSelected]   = (ImVec4){1, 0.5f, 0.7f, 1.0f};

    SingleClickMenu m = {0,.volume = -1};
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
            // igShowStyleEditor(NULL);
            igBegin("main_window", NULL,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoTitleBar);
            {

                igBeginTabBar("main_tab", 0);
                {
                    if (igBeginTabItem("Single Click", NULL, 0))
                    {
                        show_preview_window(m.angle,m.fliph,m.flipv);
                        igSameLine(0.0f, -1.0f);
                        show_single_click_menu(&m);

                        static bool input;
                        static char data[2048];

                        igDummy((ImVec2){0, 20});

                        show_output_section((ImVec2){viewport->Size.x,0});
                        show_render(&m,viewport->Size);
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
