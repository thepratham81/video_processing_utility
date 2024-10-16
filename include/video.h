#ifndef _VIDEO_H_
#include <stdbool.h>
typedef struct{ 
    char *input_file;
    char *output_file;
    char **filters;
    char **extra_option;
    char **command;
} Video;

typedef struct{
    float progress;
    bool is_finished;
    String *file_name;
}VideoProgress;

typedef enum {STOP_RENDRING=2} RENDRING_INTRUPTS;
#define QUOTE  "\""
void video_rotate(Video *video, float angle);
void video_hflip(Video *video);

void video_vflip(Video *video);

void set_aspect_ratio(Video *video, char *ratio);

void video_set_brightness(Video *video, float value);
void video_set_contrast(Video *video, float value);

void video_set_saturation(Video *video, float value);
void video_resize(Video *video, int width, int height);

void free_video(Video *video);

void video_generate_command(Video *video);

void video_init(Video *v , char *input_file, char *output_file);
void video_render_with_intrupt(Video *video,void *data, void (*callback)(VideoProgress *,void *),int *intrupt);
void video_render(Video *video,void *data, void (*callback)(VideoProgress *,void *));
#endif // _VIDEO_H_

