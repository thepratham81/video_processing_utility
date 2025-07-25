#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IMPLEMENT_VECTOR
#include "vector.h"

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

#define video_add_etc(video, x)                                               \
    do                                                                         \
    {                                                                          \
        char* tm___p;                                                          \
        size_t lo___c;\
        tm___p = x;                                                            \
        lo___c = vector_length(video->me__m);\
        while (*tm___p)                                                        \
            vector_append(video->me__m, *tm___p++);                                     \
        vector_append(video->me__m,'0');                                     \
        vector_append(video->etc,lo___c);\
    } while (0)
typedef struct
{
    char* input;
    char* vf;
    char* af;
    int*  etc;
    char* me__m; /* arena */ 
} Video;

int video_init(Video* video, char* input)
{
    video->input = calloc(strlen(input) + 1, sizeof(*input));

    if (!video->input)
        return -1;
    strcpy(video->input, input);

    video->vf    = Vector(*video->vf);
    video->af    = Vector(*video->af);
    video->etc   = Vector(*video->etc);
    video->me__m = Vector(*video->me__m);
    return 0;
}

void free_video(Video* video)
{
    free(video->input);
    free_vector(video->vf);
    free_vector(video->af);
    free_vector(video->etc);
    free_vector(video->me__m);
}

void video_rotate(Video* video, int angle)
{
    switch (angle)
    {
    case 90: {
        video_add_filter(video->vf, "transpose=1");
    }
    break;

    case 180: {
        video_add_filter(video->vf, "transpose=2");
    }
    break;

    case 270: {
        video_add_filter(video->vf, "transpose=3");
    }
    break;

    default: {
        float degree;
        degree = (angle % 360) * (3.14159265358979323846 / 180.0f);
        if (degree)
        {
            char tmp[100];
            snprintf(tmp, sizeof(tmp)-1, "rotate=%.6f", degree);
            video_add_filter(video->vf,tmp);
        }
    }
    }
}

void video_fliph(Video* video)
{
    video_add_filter(video->vf,"hflip");
}

void video_flipv(Video* video)
{
    video_add_filter(video->vf,"vflip");
}

void video_set_aspect_ratio(Video *video,int x,int y)
{

    char tmp[100];
    snprintf(tmp, sizeof(tmp)-1, "setdar=%d/%d",x,y);
    video_add_filter(video->vf,tmp);
}


void video_stereo_to_mono(Video *video)
{
    video_add_etc(video,"-ac");
    video_add_etc(video,"1");
}


void video_change_framerate(Video *video, size_t framerate) 
{
    char temp[100];
    snprintf(temp,sizeof(temp)-1, "-r %ld", framerate);
    video_add_etc(video,temp);
}


// int main()
// {
//     Video v;
//     video_init(&v,"a.mp4");
//
// }
