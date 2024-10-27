#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "file_util.h"
#include "ui/main_ui.h"

SDL_Window   *_window = NULL;
bool _is_task_running = false;
bool _is_file_dialog_open = false;
int _intrupt = 0;
const char *_allowed_file[] = {".mp4", ".mkv", ".avi"};

void callback(VideoProgress *v, void *data) {
    size_t len = str_len(v->file_name);
    char *title = malloc(len + BUFFER);
    if (!title) {
        fprintf(stderr, "Error: Unable to allocalte Memory");
        return;
    }
   	if(len > 32){
        snprintf(title, len + BUFFER, "VPU: ..%s %.2f", v->file_name + len - 32, v->progress);
   	}else{
        snprintf(title, len + BUFFER, "VPU: %s %.2f", v->file_name, v->progress);
   	}

    SDL_SetWindowTitle(_window, (v->is_finished)?TITLE:title);
    (void)data;
}

void get_formated_time(char dest[],size_t len){
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    strftime(dest,len, "%Y-%m-%d_%H.%M.%S", local);
}

void add_items(struct nk_context *ctx,WinData *windata , List *list) {
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
        if(nk_button_image(ctx, windata->icon_delete)) {
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
        video_set_brightness(video, video_opt->brightness/100.0f);
    if (flags[VIDEO_CONTARAST])
        video_set_contrast(video, video_opt->contrast);
    if(flags[VIDEO_STERIO_TO_MONO]){
		video_sterio_to_mono(video);
    }
    if(flags[VIDEO_SCALE]){
        video_resize(video,video_opt->width,video_opt->height); 
    }
}

// Lot of thing can be go wrong here;
// I will fix it later.
static void process_video(const char *file_name, const char *output_dir, int *flags, VideoOptions *video_opt) {
    Video v;
    char formated_time[32]; // large enough to store formated time 

	char *base_name             = get_basename(file_name); 
    char *directory             = get_dirname(file_name);
    char *file_name_without_ext = get_filename(base_name);
    char *extension             = get_extension(base_name);

    String *output_file = String_init();
    char *output_file_name_full_path = NULL;
    get_formated_time(formated_time,sizeof(formated_time));
    str_cat(&output_file,file_name_without_ext,"_Processed_",formated_time,extension,NULL);
    if(output_dir){
		output_file_name_full_path =  join_path(output_dir,output_file,NULL); 
    }else{
		output_file_name_full_path =  join_path(directory,output_file,NULL); 
    }
   	video_init(&v, file_name, output_file_name_full_path);

    apply_filter(&v, flags, video_opt);
    video_render_with_intrupt(&v, NULL, callback,&_intrupt);

    // cleanup:
    free(base_name);
    free(directory);
    free(file_name_without_ext);
    free(output_file_name_full_path);
    free_string(output_file);
    free(extension);
    free_video(&v);
}

//thread function
static void *_process_video_list_helper(void *data) {
    struct _thread_args *args = data;
    List *list                = args->list;
    Node *node                = list->head;
    while (node) {
        if (!_is_task_running) break;
        
        process_video(node->data,args->output_folder, args->flags, args->video_opt);
        node = node->next;
    }
    _is_task_running = false;
    pthread_exit(NULL);
    return NULL;
}

// start thread to process video
void process_video_list(List *list,const char*output_folder,int *flags, VideoOptions *video_opt) {
    static struct _thread_args args;
    static pthread_t tid;
    args.list          = list;
    args.flags         = flags;
    args.video_opt     = video_opt;
    args.output_folder = output_folder;
    _is_task_running   = true;
    pthread_create(&tid, NULL, _process_video_list_helper, &args);
}

// check wether file name is allowed
// TODO: check the magic number
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
    _intrupt         = STOP_RENDRING;
    SDL_SetWindowTitle(_window, TITLE);
}

void *_btn_output_folder_clicked_helper(void *data){
	char *selected_folder = data;
    String *temp = select_folder_dialog(NULL);
    if(!temp) goto cleanup;
    size_t len   = str_len(temp);
    if(!temp || len >= PATH_MAX) goto cleanup;
    strcpy(selected_folder,temp); 

cleanup:
    free_string(temp);
    _is_file_dialog_open = false;
    return NULL;
}

void btn_output_folder_clicked(char *selected_folder){
 	pthread_t tid;
    _is_file_dialog_open = true;
    pthread_create(&tid, NULL,_btn_output_folder_clicked_helper ,selected_folder);

}

void *_btn_add_file_clicked_helper(void *data){
    List *list = data;
    char video_folder[2048];
    get_video_folder(video_folder);
    char **result = open_file_dialog(video_folder, "Video File |*.mkv;*.mp4;*.avi|All Files|*.*");
    if(!result){
        _is_file_dialog_open = false;
        return NULL;
    } 
    for(size_t i = 0 ; i < vector_length(result);i++){
        if(!is_file(result[i])) continue;
        list_append(list, str_duplicate(result[i]));
        free_string(result[i]);
    }
    free_vector(result);
    _is_file_dialog_open = false;
    return NULL;
}

void btn_add_file_clicked(List *list){
    pthread_t tid;
    _is_file_dialog_open = true;
    pthread_create(&tid, NULL, _btn_add_file_clicked_helper, list);

}

void btn_process_video_clicked(List *list,const char*output_folder,VideoOptions *video_options){
    _is_task_running = true;
    _intrupt         = 0;
    process_video_list(list, output_folder ,video_options->check, video_options);
}

void main_ui(WinData *windata) {
    static List list                  = {NULL, NULL, free};
    static VideoOptions video_options = {.angle = 90,.check[VIDEO_OUTPUT_DIR] = 1};
    static char output_dir[PATH_MAX];

    SDL_Event evt;
    struct nk_context *ctx = windata->ctx;
    _window = windata->window;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt)) {
        if (evt.type == SDL_QUIT) {
            windata->should_close = 1;
        } else if (evt.type == SDL_DROPFILE) { // File is droped in window
            if (is_file_allowed(evt.drop.file)) {
                list_append(&list, str_duplicate(evt.drop.file));
                SDL_free(evt.drop.file);
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
                add_items(ctx,windata, &list);
                nk_group_end(ctx);
            }

            // Button Add Files
            nk_layout_row_dynamic(ctx, 0, 2);
            disable_begin(ctx,_is_file_dialog_open );
            if (nk_button_label(ctx, "Add File(s)")) {
                btn_add_file_clicked(&list);
            }
            disable_end(ctx,_is_file_dialog_open );

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
            disable_begin(ctx,1);
            nk_checkbox_label(ctx, "Merge All Videos (Not implemented yet)",
            					&video_options.check[VIDEO_MERGE_VIDEOS]);
            disable_end(ctx,1);

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
            nk_checkbox_label(ctx, "Sterio to mono", &video_options.check[VIDEO_STERIO_TO_MONO]);
            //nk_checkbox_label(ctx, "Change aspect ratio", &video_options.check[VIDEO_ASPECT_RATIO]);

            nk_checkbox_label(ctx, "Scale Video", &video_options.check[VIDEO_SCALE]);
            if (video_options.check[VIDEO_SCALE]) {
                nk_layout_row_dynamic(ctx, 0, 2);
                nk_property_int(ctx, "Width", 1, &video_options.width, 3600, 1, 1);
                nk_property_int(ctx, "Height", 1, &video_options.height, 3600, 1, 1);
            }

            nk_layout_row_dynamic(ctx, 0, 2);
            nk_checkbox_label(ctx, "Change Brightness", &video_options.check[VIDEO_BRIGHTNESS]);
            if (video_options.check[VIDEO_BRIGHTNESS]) {
                nk_property_float(ctx, "Brightness", -100, &video_options.brightness, 100,1, 1);
            }

            // Disable video ends
            disable_end(ctx, video_options.check[VIDEO_MERGE_VIDEOS]);

            nk_group_end(ctx);
        }

        // Output Folder Selection
        nk_layout_row_dynamic(ctx, 0, 1);
        nk_checkbox_label(ctx,"Output directory same as input directory",&video_options.check[VIDEO_OUTPUT_DIR]);

        nk_label(ctx, "Output Folder:", NK_TEXT_CENTERED | NK_TEXT_LEFT);

        disable_begin(ctx,video_options.check[VIDEO_OUTPUT_DIR]);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2);
        {

            nk_layout_row_push(ctx, 0.80f);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, output_dir,sizeof(output_dir), NULL);

            nk_layout_row_push(ctx, 0.20f);
            if (nk_button_label(ctx, "Browse")) {
                btn_output_folder_clicked(output_dir);
            }

            nk_layout_row_end(ctx);
        }
        
        disable_end(ctx,video_options.check[VIDEO_OUTPUT_DIR]);

        nk_layout_row_dynamic(ctx, 0, 1);

        if (_is_task_running) {
            if (nk_button_label(ctx, "Stop All")) btn_stop_clicked();
            
        } else {
            // Process Video Button
            // Disable button if file list is empty
            disable_begin(ctx, !list.head || _is_task_running); //_is_task_running -> global variable;

            if (nk_button_label(ctx, "Process Video")) {
                const char *temp = NULL;
                if(!video_options.check[VIDEO_OUTPUT_DIR] && strlen(output_dir)) temp = output_dir;
                btn_process_video_clicked(&list,temp, &video_options);
            }
            disable_end(ctx, list.head || !_is_task_running);
        }
    }

    nk_end(ctx);
}
