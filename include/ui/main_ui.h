#ifndef MAIN_UI_H
#define MAIN_UI_H

#include "SDL_video.h"
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#ifndef NK_NUKLEAR_H_
#include <nuklear.h>
#include <SDL.h>
#endif //NK_NUKLEAR_H_

typedef struct {
    struct nk_context *ctx;
    SDL_Renderer *renderer;
    SDL_Window *window;
    int (*handle_event)(SDL_Event *evt);
    int should_close;
    int w;
    int h;
}WinData;

typedef enum {
    VIDEO_ROTATE,
    VIDEO_FIP_H,
    VIDEO_FIP_V,
    VIDEO_ASPECT_RATIO,
    VIDEO_BRIGHTNESS,
    VIDEO_CONTARAST,
    VIDEO_SCALE,
    VIDEO_MERGE_VIDEOS
} Filter;

typedef struct{
    int check[10];
    int width;
    int height;
    int volume;
    char *aspect_ratio;
    float brightness;
    float saturation;
    float contrast;
    int angle;
}VideoOptions;

void main_ui(WinData *windata);

#endif // MAIN_UI_H