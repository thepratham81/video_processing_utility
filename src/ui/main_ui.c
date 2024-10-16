#include <immintrin.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "SDL_events.h"
#include "SDL_stdinc.h"
#include "cstring.h"
#include "file_util.h"
#include "list.h"
#include "subprocess.h"
#include "ui/main_ui.h"
#include "vector.h"
#include "video.h"
#define BUFFER 100
#define TITLE "VPU"
#define disable_begin(ctx, condition) \
    if (condition)                    \
    nk_widget_disable_begin(ctx)
#define disable_end(ctx, condition) \
    if (condition)                  \
    nk_widget_disable_end(ctx)

struct _thread_args {
    List *list;
    int *flags;
    VideoOptions *video_opt;
};

char *_allowed_file[] = {".mp4", ".mkv", ".avi"};
SDL_Window *_window = NULL;
bool _is_task_running = false;
int _intrupt = 0;

void callback(VideoProgress *v, void *data) {
    size_t len = str_len(v->file_name);
    char *title = malloc(len + BUFFER);
    if (!title) {
        fprintf(stderr, "Error: Unable to allocalte Memory");
        return;
    }
    
    snprintf(title, len + BUFFER, "VPU: %s %.2f", v->file_name, v->progress);
    SDL_SetWindowTitle(_window, (v->is_finished)?TITLE:title);

}

void add_items(struct nk_context *ctx, List *list) {
    Node *node = list->head;
    Node *next_node;
    while (node) {
        next_node = node->next;

        nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2);

        // Label file name
        nk_layout_row_push(ctx, 0.90f);
        nk_label(ctx, node->data, NK_TEXT_CENTERED | NK_TEXT_LEFT);

        // Button Delete file from list
        nk_layout_row_push(ctx, 0.10f);
        if (nk_button_label(ctx, "D")) {
            list_pop(list, node);
        }
        node = next_node;
        nk_layout_row_end(ctx);
    }
}

void apply_filter(Video *video, int *flags, VideoOptions *video_opt) {
    if (flags[VIDEO_ROTATE])
        video_rotate(video, (float)video_opt->angle);
    if (flags[VIDEO_FIP_H])
        video_hflip(video);
    if (flags[VIDEO_FIP_V])
        video_vflip(video);
    if (flags[VIDEO_BRIGHTNESS])
        video_set_brightness(video, video_opt->brightness);
    if (flags[VIDEO_CONTARAST])
        video_set_contrast(video, video_opt->contrast);
}
static void process_video(char *file_name, int *flags, VideoOptions *video_opt) {
    Video v;
    char *file_name_without_ext = get_filename(file_name);
    char *extension = get_extension(file_name);
    String *output_file = String_from(file_name_without_ext);
    str_cat(&output_file, "_Processed");
    if (extension)
        str_cat(&output_file, extension);

    video_init(&v, file_name, output_file);

    apply_filter(&v, flags, video_opt);

    video_render_with_intrupt(&v, NULL, callback,&_intrupt);

    // cleanup:
    free(file_name_without_ext);
    free_string(output_file);
    free(extension);
    free_video(&v);
}

static void *_process_video_list_helper(void *data) {
    struct _thread_args *args = data;
    List *list = args->list;
    Node *node = list->head;
    while (node) {
        if (!_is_task_running) break;
        
        process_video(node->data, args->flags, args->video_opt);
        node = node->next;
    }
    _is_task_running = false;
    pthread_exit(NULL);
    return NULL;
}

void process_video_list(List *list, int *flags, VideoOptions *video_opt) {
    static struct _thread_args args;
    static pthread_t tid;
    args.list = list;
    args.flags = flags;
    args.video_opt = video_opt;
    _is_task_running = true;
    pthread_create(&tid, NULL, _process_video_list_helper, &args);
}

bool is_file_allowed(char *file_name) {
    char *temp = NULL;
    if (!is_file(file_name)) {
        return false;
    }

    temp = strrchr(file_name, '.');
    if (temp != NULL) {
        for (size_t i = 0; i < sizeof(_allowed_file) / sizeof(_allowed_file[0]); i++) {
            if (strcasecmp(_allowed_file[i], temp) == 0) {
                return true;
            }
        }
    }
    return false;
}

void btn_stop_clicked(){
    _is_task_running = false;
    _intrupt = STOP_RENDRING;
    SDL_SetWindowTitle(_window, TITLE);
}
void main_ui(WinData *windata) {

    static List list = {NULL, NULL, SDL_free};
    static VideoOptions video_options = {.angle = 90};
    static char output_dir[255];

    SDL_Event evt;
    struct nk_context *ctx = windata->ctx;
    _window = windata->window;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT) {
            windata->should_close = 1;
        } else if (evt.type == SDL_DROPFILE) { // File is droped in window
            if (is_file_allowed(evt.drop.file)) {
                list_append(&list, evt.drop.file);
            }
        }

        windata->handle_event(&evt);
    }

    nk_input_end(ctx);

    if (nk_begin(ctx, "VPS", nk_rect(0, 0, windata->w, windata->h),
                 NK_WINDOW_NO_SCROLLBAR)) {

        // Input files
        nk_layout_row_dynamic(ctx, 450, 2);
        if (nk_group_begin(ctx, "Files", 0)) {
            // Group file
            nk_layout_row_dynamic(ctx, 380, 1);
            if (nk_group_begin(ctx, "Darg & Drop File Here",
                               NK_WINDOW_BORDER | NK_WINDOW_TITLE)) {
                // Add files
                add_items(ctx, &list);
                nk_group_end(ctx);
            }

            // Button Add Files
            nk_layout_row_dynamic(ctx, 0, 2);
            if (nk_button_label(ctx, "Add File(s)")) {
            }

            // Button Remove all files
            // Disable button if list is empty
            disable_begin(ctx, !list.head);
            if (nk_button_label(ctx, "Remove All")) {
                while (list.head) {
                    list_pop(&list, NULL);
                }
            }
            // Disable button end
            disable_end(ctx, !list.head);

            nk_group_end(ctx);
        }

        // Group options
        if (nk_group_begin(ctx, "Options", 0)) {
            // All avilable options
            nk_layout_row_dynamic(ctx, 0, 1);
            nk_checkbox_label(ctx, "Merge All Videos", &video_options.check[VIDEO_MERGE_VIDEOS]);

            // disable all other options when merge video option is selected
            disable_begin(ctx, video_options.check[VIDEO_MERGE_VIDEOS]);

            nk_layout_row_dynamic(ctx, 0, 2);
            nk_checkbox_label(ctx, "Rotate x° clockwise", &video_options.check[VIDEO_ROTATE]);
            if (video_options.check[VIDEO_ROTATE]) {
                nk_property_int(ctx, "Rotate x°", 1, &video_options.angle, 360, 1, 1);
            }

            nk_layout_row_dynamic(ctx, 0, 1);
            nk_checkbox_label(ctx, "Flip horizontally", &video_options.check[VIDEO_FIP_H]);
            nk_checkbox_label(ctx, "Flip vertically", &video_options.check[VIDEO_FIP_V]);
            nk_checkbox_label(ctx, "Change aspect ratio", &video_options.check[VIDEO_ASPECT_RATIO]);

            nk_checkbox_label(ctx, "Scale Video", &video_options.check[VIDEO_SCALE]);
            if (video_options.check[VIDEO_SCALE]) {
                nk_layout_row_dynamic(ctx, 0, 2);
                nk_property_int(ctx, "Width", 1, &video_options.width, 360, 1, 30);
                nk_property_int(ctx, "Height", 1, &video_options.height, 360, 1, 30);
            }

            nk_layout_row_dynamic(ctx, 0, 2);
            nk_checkbox_label(ctx, "Change Brightness", &video_options.check[VIDEO_BRIGHTNESS]);
            if (video_options.check[VIDEO_BRIGHTNESS]) {
                nk_property_float(ctx, "Brightness", 1, &video_options.brightness, 100, 1, 1);
            }

            // Disable video ends
            disable_end(ctx, video_options.check[VIDEO_MERGE_VIDEOS]);

            nk_group_end(ctx);
        }

        // Output Folder Selection
        nk_layout_row_dynamic(ctx, 0, 1);
        nk_label(ctx, "Output Folder:", NK_TEXT_CENTERED | NK_TEXT_LEFT);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2);
        {

            nk_layout_row_push(ctx, 0.80f);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, output_dir, 255, NULL);

            nk_layout_row_push(ctx, 0.20f);
            if (nk_button_label(ctx, "Browse")) {
            }

            nk_layout_row_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 0, 1);
        if (_is_task_running) {
            if (nk_button_label(ctx, "Stop All")) btn_stop_clicked();
            
        } else {
            // Process Video Button
            // Disable button if file list is empty
            disable_begin(ctx, !list.head || _is_task_running); //_is_task_running -> global variable;

            if (nk_button_label(ctx, "Process Video")) {
                _is_task_running = true;
                _intrupt = 0;
                process_video_list(&list, video_options.check, &video_options);
            }
            disable_end(ctx, list.head || !_is_task_running);
        }
    }

    nk_end(ctx);
}
