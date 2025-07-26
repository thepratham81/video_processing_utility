#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IMPLEMENT_VECTOR
#include "vector.h"

#define MAX_BUF_SIZE 1 << 8
#define video_add_filter(dst, x)                                               \
    do                                                                         \
    {                                                                          \
        char* tm___p;                                                          \
        if (vector_length(dst))                                                \
            vector_append(dst, ',');                                           \
        tm___p = x;                                                            \
        while (*tm___p)                                                        \
            vector_append(dst, *tm___p++);                                     \
    } while (0)

#define video_add_etc(video, x)                                                \
    do                                                                         \
    {                                                                          \
        char* tm___p;                                                          \
        size_t lo___c;                                                         \
        tm___p = x;                                                            \
        lo___c = vector_length(video->me__m);                                  \
        while (*tm___p)                                                        \
            vector_append(video->me__m, *tm___p++);                            \
        vector_append(video->me__m, '0');                                      \
        vector_append(video->et__c, lo___c);                                     \
    } while (0)
typedef struct
{
    char* file_name;
    char* v__f;
    char* a__f;
    int* et__c;
    char* me__m; /* arena */
} Video;

int video_init(Video* video, char* input)
{
    video->file_name = calloc(strlen(input) + 1, sizeof(*input));

    if (!video->file_name)
        return -1;
    strcpy(video->file_name, input);

    video->v__f = Vector(*video->v__f);
    video->a__f = Vector(*video->a__f);
    video->et__c = Vector(*video->et__c);
    video->me__m = Vector(*video->me__m);
    return 0;
}

void free_video(Video* video)
{
    free(video->file_name);
    free_vector(video->v__f);
    free_vector(video->a__f);
    free_vector(video->et__c);
    free_vector(video->me__m);
}

void video_rotate(Video* video, int angle)
{
    switch (angle)
    {
    case 90: {
        video_add_filter(video->v__f, "transpose=1");
    }
    break;

    case 180: {
        video_add_filter(video->v__f, "transpose=2");
    }
    break;

    case 270: {
        video_add_filter(video->v__f, "transpose=3");
    }
    break;

    default: {
        float degree;
        degree = (angle % 360) * (3.14159265358979323846 / 180.0f);
        if (degree)
        {
            char tmp[MAX_BUF_SIZE];
            snprintf(tmp, sizeof(tmp), "rotate=%.6f", degree);
            video_add_filter(video->v__f, tmp);
        }
    }
    }
}

void video_fliph(Video* video)
{
    video_add_filter(video->v__f, "hflip");
}

void video_flipv(Video* video)
{
    video_add_filter(video->v__f, "vflip");
}

void video_set_aspect_ratio(Video* video, int x, int y)
{

    char tmp[MAX_BUF_SIZE];
    snprintf(tmp, sizeof(tmp), "setdar=%d/%d", x, y);
    video_add_filter(video->v__f, tmp);
}

void video_stereo_to_mono(Video* video)
{
    video_add_etc(video, "-ac");
    video_add_etc(video, "1");
}

void video_scale_volume(Video* video, int volume)
{
    char tmp[MAX_BUF_SIZE];
    snprintf(tmp, 100, "volume=%.6f", (float)(volume % 100) / (100.0f));
    video_add_etc(video, "-filter:a");
    video_add_etc(video, tmp);
}

void video_change_bitrate(Video *video, int bitrate)
{
    char tmp[MAX_BUF_SIZE];
    snprintf(tmp,sizeof(tmp),"-b:v %d", bitrate);
    video_add_etc(video, tmp);
}

void video_change_framerate(Video* video, size_t framerate)
{
    char tmp[MAX_BUF_SIZE];
    snprintf(tmp, sizeof(tmp), "-r %ld", framerate);
    video_add_etc(video, tmp);
}

void remove_audio(Video* video)
{
    video_add_etc(video, "-an");
}
