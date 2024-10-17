/* nuklear - 1.32.0 - public domain */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear_sdl_renderer.h"

#include "./resources.c"
#include "ui/main_ui.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600
#define TITLE "VPU"

int main(int argc, char *argv[]) {

    /* Platform */
    WinData windata;
    SDL_Window *win;
    SDL_Renderer *renderer;
    int running = 1;
    int flags = 0;
    float font_scale = 2;

    /* GUI */
    struct nk_context *ctx;
    struct nk_colorf bg;

    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);

    win = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                           SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (win == NULL) {
        SDL_Log("Error SDL_CreateWindow %s", SDL_GetError());
        exit(-1);
    }

    flags |= SDL_RENDERER_ACCELERATED;
    flags |= SDL_RENDERER_PRESENTVSYNC;

#if 0
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#endif
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR /* Available since 2.0.8 */
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 22)
    SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
#endif

#if SDL_VERSION_ATLEAST(2, 0, 8)
    /* This hint tells SDL to respect borderless window as a normal window.
    ** For example, the window will sit right on top of the taskbar instead
    ** of obscuring it. */
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 12)
    /* This hint tells SDL to allow the user to resize a borderless windoow.
    ** It also enables aero-snap on Windows apparently. */
    SDL_SetHint("SDL_BORDERLESS_RESIZABLE_STYLE", "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 9)
    SDL_SetHint("SDL_MOUSE_DOUBLE_CLICK_RADIUS", "4");
#endif


    renderer = SDL_CreateRenderer(win, -1, flags);

    if (renderer == NULL) {
        SDL_Log("Error SDL_CreateRenderer %s", SDL_GetError());
        exit(-1);
    }

    /* scale the renderer output for High-DPI displays */
    {
        int render_w, render_h;
        int window_w, window_h;
        float scale_x, scale_y;
        SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
        SDL_GetWindowSize(win, &window_w, &window_h);
        scale_x = (float)(render_w) / (float)(window_w);
        scale_y = (float)(render_h) / (float)(window_h);
        SDL_RenderSetScale(renderer, scale_x, scale_y);
        font_scale = scale_y;
    }

    /* GUI */
    ctx = nk_sdl_init(win, renderer);

    ctx->style.window.padding = (struct nk_vec2){.x = 20, .y = 10};
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas *atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font *font;
        /* set up the font atlas and add desired font; note that font sizes are
         * multiplied by font_scale to produce better results at higher DPIs */
        nk_sdl_font_stash_begin(&atlas);
        // font = nk_font_atlas_add_default(atlas, 20 * font_scale, &config);
        // font = nk_font_atlas_add_from_file(atlas,"./font.ttf", 20*
        // font_scale,&config);
        font = nk_font_atlas_add_from_memory(
            atlas, font_data, sizeof(font_data) / sizeof(font_data[0]),
            25.0 * font_scale, &config);
        nk_sdl_font_stash_end();
        /* this hack makes the font appear to be scaled down to the desired
         * size and is only necessary when font_scale > 1 */
        font->handle.height /= font_scale;
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        nk_style_set_font(ctx, &font->handle);
    }

    // populating windata
    windata.ctx = ctx;
    windata.renderer = renderer;
    windata.w = WINDOW_WIDTH;
    windata.h = WINDOW_WIDTH;
    windata.should_close = 0;
    windata.handle_event = nk_sdl_handle_event;
    windata.window = win;

    while (running) {
        /* Input */

        /* GUI */
        main_ui(&windata);
        if (windata.should_close) {
            goto cleanup;
        }
        SDL_RenderClear(renderer);

        nk_sdl_render(NK_ANTI_ALIASING_ON);

        SDL_RenderPresent(renderer);
    }

cleanup:
    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
