#include "file_util.h"
#define IMPLEMENT_VECTOR
#include <math.h>
#include <stdio.h>
#include "vector.h"
#define IMPLEMENT_STRING
#include "cstring.h"

#include "video.h"
#include "subprocess.h"

#ifdef _WIN32
    #define fileno(stream) _fileno(stream)
    #define lseek(fd, offset, whence) _lseek(fd, offset, whence)
#endif

void default_callback(VideoProgress *v, void *data){
    (void)data;
    printf("%s %f",v->file_name,v->progress);
}

void video_rotate(Video *video, float angle) {
    char temp[100] = {0};
    if (angle == 90) {
        vector_append(video->filters, String_from("transpose=1"));
    } else if (angle == 180) {
        vector_append(video->filters, String_from("transpose=2"));
    } else if (angle == 270) {
        vector_append(video->filters, String_from("transpose=3"));
    } else {
        float degree = fmod(angle, 360.0f) * (3.14159265358979323846 / 180.0f);
        if (degree != 0) {
            snprintf(temp, 100-1, "rotate=%.6f", degree);
            vector_append(video->filters, String_from(temp));
        }
    }
}

void video_hflip(Video *video) {
    vector_append(video->filters, String_from("hflip"));
}

void video_vflip(Video *video) {
    vector_append(video->filters, String_from("vflip"));
}

void video_remove_audio(Video *video) {
    vector_append(video->extra_option, String_from("-an"));
}

void video_change_framerate(Video *video, size_t framerate) {
    char temp[100] = {0};
    snprintf(temp, 100, "-r %ld", framerate);
    vector_append(video->extra_option, String_from(temp));
}
void video_change_bitrate(Video *video, size_t bitrate) {
    char temp[100] = {0};
    snprintf(temp, 100, "-b:v %ld", bitrate);
    vector_append(video->extra_option, String_from(temp));
}

// void video_set_volume(Video *video , size_t volume){
//     char temp[100] = {0};
//     snprintf(temp, 100, "volume=%.6f", (float)(volume%100)/(100.0f));
//     vector_append(video->extra_option,String_from(temp));
// }

void set_aspect_ratio(Video *video, char *ratio) {
    char temp[100] = {0};
    snprintf(temp, 100, "setdar=%s", ratio);
    vector_append(video->filters, String_from(temp));
}

void video_set_brightness(Video *video, float value) {
    char temp[100] = {0};
    snprintf(temp, 100, "eq=brightness=%.6f", value);
    vector_append(video->filters, String_from(temp));
}
void video_set_contrast(Video *video, float value) {
    char temp[100] = {0};
    snprintf(temp, 100, "eq=contrast=%.6f", value);
    vector_append(video->filters, String_from(temp));
}

void video_set_saturation(Video *video, float value) {
    char temp[100] = {0};
    snprintf(temp, 100, "q=saturation=%.6f", value);
    vector_append(video->filters, String_from(temp));
}
void video_resize(Video *video, int width, int height) {
    char temp[100] = {0};
    snprintf(temp, 100, "scale=%d:%d", width, height);
    vector_append(video->filters, String_from(temp));
}

// add -vf [options] to command line argument
static void _apply_filter(Video *video, void *cmd) {
    char ***command = cmd;
    if (vector_length(video->filters)) {
        char *joined = str_join((const char **)video->filters, ",", vector_length(video->filters), NULL, NULL);
        vector_append(joined,'\0');
        vector_append(*command, String_from("-vf"));
        vector_append(*command, String_from(joined));
        free_string(joined);
    }
}

void video_merge_file(const char **file){
    return;
}

// generate command to execute ffmpeg -i <input file> -vf [options] -progress pipe:2 <output file>
void video_generate_command(Video *video) {
    String **command = Vector(*command);

    if (!command) {
        return;
    }

    vector_append(command, String_from("/usr/bin/ffmpeg"));
    vector_append(command, String_from("-i"));
    vector_append(command, String_from(video->input_file));

    _apply_filter(video, &command);
    
    vector_append(command,String_from("-progress"));
    vector_append(command,String_from("pipe:2"));
    vector_append(command, String_from(video->output_file));
    vector_append(command,NULL);
    video->command = command;
}

void video_init(Video *v , char *input_file, char *output_file) {
    v->extra_option = Vector(char *);
    v->output_file  = String_from(output_file);
    v->input_file   = String_from(input_file);
    v->filters      = Vector(char *);
    v->command      = NULL;
}

//get the duration of a video
static float video_duration(const char *file_name) {
    //TODO: get more info about video
    const char *command[] = {
    "/usr/bin/ffprobe",
    "-v",
    "error",
    "-show_entries",
    "format=duration",
    "-of",
    "default=noprint_wrappers=1:nokey=1",
    file_name,
    NULL
    };

    struct subprocess_s subprocess;
    int result = subprocess_create(command, 0, &subprocess);
    float video_length;
    if(result!=0){
        fprintf(stderr,"Error: Unable to create subprocess");
        return -1.0;
    }
    FILE* p_stdout = subprocess_stdout(&subprocess);
    if(fscanf(p_stdout, "%f",&video_length)==1){
        return video_length;
    }
    return -1;
}

//parse ffmpeg output to get progress in percent
static void _get_progress(const char *line, float video_duration_sec, float *progress) {
    long time_processed;
    if (sscanf(line, "out_time_ms=%ld", &time_processed) == 1) {
        double seconds_processed = time_processed / 1000000.0;
        *progress = (seconds_processed /video_duration_sec) * 100.0;
    }
}

void _video_render_helper(Video *video,void *data, void (*callback)(VideoProgress *,void *),int *intrupt){
    if(is_file(video->output_file)){
        return;
    }

    float video_length = video_duration(video->input_file);
    if(video_length < 0.0f){
        return;
    }
    struct subprocess_s subprocess;
    VideoProgress vp = {0};
    video_generate_command(video);
    int result = subprocess_create((const char * const *)video->command, 0, &subprocess);
    if(result!=0){
        fprintf(stderr,"Error: Unable to create subprocess");
        return;
    }
    
    if(!callback) callback = default_callback;
    FILE* p_stderr = subprocess_stderr(&subprocess);
    char process_stdout[1024];
    int len = sizeof(process_stdout);

    vp.file_name = video->input_file;
    while(subprocess_alive(&subprocess)){
            fgets(process_stdout,len, p_stderr);
            _get_progress(process_stdout,video_length , &vp.progress);
            if(intrupt && *intrupt == STOP_RENDRING){
                subprocess_destroy(&subprocess);
                break;
            }
            callback(&vp,data);
    }
    vp.progress = 100;
    vp.is_finished = true;
    callback(&vp,data);
}

//start subprocess of generated command
void video_render(Video *video,void *data, void (*callback)(VideoProgress *,void *)) {
    _video_render_helper(video,data,callback,NULL);
}

void video_render_with_intrupt(Video *video,void *data, void (*callback)(VideoProgress *,void *),int *intrupt) {
    _video_render_helper(video,data,callback,intrupt);
}
void free_video(Video *video) {
    for (size_t i = 0; i < vector_length(video->filters); i++) {
        free_string(video->filters[i]);
    }
    if (video->command) {

        for (size_t i = 0; i < vector_length(video->command); i++) {
            free_string(video->command[i]);
        }
    }

    free_string(video->filters);
}
