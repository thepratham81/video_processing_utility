#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define IMPLEMENT_VECTOR
#include "vector.h"

#include "subprocess.h" 
#include "cJSON.c"

typedef struct{
    uint64_t total_processed;
    int res;
    bool is_finished;
}VideoProgress;

#define MAX_BUF_SIZE 1 << 8

#define video_add_filter(dst, x)                                               \
    do                                                                         \
    {                                                                          \
        char* tm___p;                                                          \
        if (vector_length(dst))                                                \
        {                                                                      \
            vector_pop(dst);         \
            vector_append(dst, ',');                                           \
        }                                                                      \
        tm___p = x;                                                            \
        while (*tm___p)                                                        \
            vector_append(dst, *tm___p++);                                     \
        vector_append(dst, '\0');         \
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
        vector_append(video->me__m, '\0');                                      \
        vector_append(video->et__c, lo___c);                                     \
    } while (0)
typedef struct
{
    int file_name;
    char* v__f;
    char* a__f;
    int* et__c;
    char* me__m; /* arena like*/
} Video;

const int CMD__FLAG = subprocess_option_search_user_path|subprocess_option_no_window;

void free_video(Video* video);
int video_init(Video* video, char* input,FILE* log_file)
{

    if (!input)
        return -1;

    video->v__f      = Vector(*video->v__f);
    video->a__f      = Vector(*video->a__f);
    video->et__c     = Vector(*video->et__c);
    video->me__m     = Vector(*video->me__m);
    video->file_name = 0;
    if (!video->v__f || !video->a__f || !video->et__c || !video->me__m)
        goto fail;
    while(*input)
    {
        vector_append(video->me__m,*input++);
    }

    vector_append(video->me__m,'\0');
    return 0;

fail:
    if(log_file) fprintf(log_file,"[Error] unable to allocate memory\n");
    free_video(video);
    perror("malloc");
    return -1;
}

void free_video(Video* video)
{
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

void video_change_bitrate(Video *video, char* bitrate)
{
    if(!bitrate) return;
    video_add_etc(video, "-b:v");
    video_add_etc(video, bitrate);
}

void video_change_framerate(Video* video, size_t framerate)
{
    char tmp[MAX_BUF_SIZE];
    snprintf(tmp, sizeof(tmp), "-r %ld", framerate);
    video_add_etc(video, tmp);
}

void video_remove_audio(Video* video)
{
    video_add_etc(video, "-an");
}

void* slurp__stream(FILE* f,size_t* outlen)
{

    int bytes_read;
    size_t capacity = (1 << 20);  // 1MB 
    size_t index = 0;
    #define CHUNK (1<<10)  // 1KB chunks

    unsigned char* data = malloc(capacity);
    if (!data) {
        *outlen = 0;
        return NULL;
    }

    do
    {
        bytes_read = fread(data+index, 1, CHUNK,f);
        if (bytes_read > 0) {
            index += bytes_read;
            
            // Check if we need to expand buffer
            if (index + CHUNK > capacity) {
                capacity <<= 1;
                unsigned char* temp = realloc(data, capacity);
                if (!temp) {
                    free(data);
                    *outlen = 0;
                    return NULL;
                }
                data = temp;
            }
        }
    } while (bytes_read > 0);

    *outlen = index;
    if (index == 0) {
        free(data);
        return NULL;
    }

    return data;

}
// TODO: This code expect no error occor.
void* video_get_thumbnail(const char*ffmpeg,const char* file_name, size_t* outlen)
{
    const char* command[] = {
        ffmpeg,
        "-i",
        file_name,
        "-ss",
        "00:00:03",
        "-vframes",
        "1",
        "-f",
        "image2pipe",
        "-vcodec",
        "bmp",
        "-",
        NULL
    }; 
    struct subprocess_s process;
    int result = subprocess_create(command,CMD__FLAG,&process);
    if (result != 0)
    {
        *outlen = 0;
        return NULL;
    }
    
    FILE * f = subprocess_stdout(&process); 
    return slurp__stream(f,outlen);
}

static float video_get_duration(const char* ffprobe,const char *file_name) {
    //TODO: get more info about video
    const char *command[] = {
    ffprobe,
    "-v",
    "error",
    "-show_entries",
    "format=duration",
    "-of",
    "default=noprint_wrappers=1:nokey=1",
    file_name,
    NULL
    };
     
    struct subprocess_s process;
    int result = subprocess_create(command,CMD__FLAG,&process);
    float video_length;
    if(result!=0){
        fprintf(stderr,"Error: Unable to create subprocess");
        return -1.0;
    }
    FILE* p_stdout = subprocess_stdout(&process);
    if(!p_stdout) return 0; 
    if(fscanf(p_stdout, "%f",&video_length)==1){
        return video_length;
    }
    return -1;
}

char* video_filename(Video* v)
{
    return &v->me__m[v->file_name];
}

void default_callback(VideoProgress *v, void *data){
    (void)data;
    (void)v;
}

static void _get_progress(const char *line, float video_duration_sec, float *progress) {
    long time_processed;
    if (sscanf(line, "out_time_ms=%ld", &time_processed) == 1) {
        puts("asdfasdf");
        double seconds_processed = time_processed / 1000000.0;
        *progress = (seconds_processed /video_duration_sec) * 100.0;
    }
}

void video_render(char* ffmpeg, Video* v, char* output,
                  void (*callback)(VideoProgress*, void*), void* user_data,
                  bool* stop_rendering)
{

    int apply_video_filter = vector_length(v->v__f);
    char** cmd = Vector(*cmd);
    VideoProgress vp;
    vector_append(cmd,ffmpeg);
    vector_append(cmd,"-i");
    vector_append(cmd,video_filename(v));
    if(apply_video_filter)
    {

        vector_append(cmd,"-vf");
        vector_append(cmd,v->v__f);
    }

    int len = vector_length(v->et__c);
    if(len)
    {
        for(int i = 0;i<len;i++)
        {
            vector_append(cmd,&v->me__m[v->et__c[i]]);
        }
    }
    vector_append(cmd, "-progress");
    vector_append(cmd, "pipe:2");

    // vector_append(cmd, "-");
    // vector_append(cmd, "-nostats");
    vector_append(cmd, output);
    vector_append(cmd,NULL);
    for(int i=0;i<vector_length(cmd);i++)
    {
        printf("%s ",cmd[i]);
    }
    printf("\n"); 


    struct subprocess_s process;
    if(!callback) callback = default_callback;
    int result = subprocess_create((const char * const *)cmd,CMD__FLAG,&process);
    if(result!=0)
    {
        vp.res = -1;
        return;
    }

    char process_stdout[4192];

    FILE* p_stderr = subprocess_stderr(&process);
    uint64_t tmp;

    vp.is_finished = false;
    vp.total_processed = 0;
    vp.is_finished = false;
    while(subprocess_alive(&process))
    {
        if(*stop_rendering)
        {
            subprocess_destroy(&process);
            break;
        }
        fgets(process_stdout,sizeof(process_stdout), p_stderr);
        if(sscanf(process_stdout, "out_time_ms=%ld",&tmp)==1) vp.total_processed = tmp;
        callback(&vp,user_data);
    }
    vp.res = 0;
    vp.is_finished = true;
    callback(&vp,user_data);
}

typedef struct
{
    uint64_t file_size;
}VideoInfo;

int get_json_key_count(cJSON *json_object)
{
    if (json_object == NULL || !cJSON_IsObject(json_object))
    {
        return -1; 
    }
    
    return cJSON_GetArraySize(json_object);
}

char* bytes_to_humanreadable(uint64_t size)
{

#define make_str(buffer, fmt, size)                                            \
    do                                                                         \
    {                                                                          \
        int n = snprintf(NULL, 0, fmt, size);                                  \
        buffer = malloc(n + 1);                                                \
        if (buffer)                                                            \
            snprintf(buffer, n + 1, fmt, size);                                \
    } while (0)

    uint64_t kb = 1 << 10;
    uint64_t mb = 1 << 20;
    uint64_t gb = 1 << 30;
    uint64_t tb = 1ULL << 40;
    char* res   = NULL;

    if(size >= tb)
    {
        make_str(res,"%.2fTB",(float)size/tb);
        return res;
    }

    else if(size >= gb)
    {
        make_str(res,"%.2fGB",(float)size/gb);
        return res;
    }


    else if(size >= mb)
    {
        make_str(res,"%.2fMB",(float)size/mb);
        return res;
    }

    else if(size >= kb)
    {
        make_str(res,"%.2fKB",(float)size/kb);
        return res;
    }

    else
    {
        make_str(res,"%ldB",size/kb);
        return res;
    }

}

int video_get_info(VideoInfo* inf, const char* input)
{

    struct subprocess_s process;
    const char* cmd[] = {
                        "ffprobe",
                        "-v",
                        "quiet",
                        "-print_format",
                        "json",
                        "-show_format",
                        "-show_streams",
                        input,
                        NULL
                        };
    int result = subprocess_create(cmd,CMD__FLAG,&process);
    if (result != 0)
        return -1;

    
    FILE* p_stdout  = subprocess_stdout(&process);
    size_t outlen   = 0;
    char* data      = slurp__stream(p_stdout, &outlen);
    char* file_size = NULL;
    if(!data) return -1;

    cJSON* json = cJSON_Parse(data);

    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
            result = -1;
            goto cleanup;
        }
    }

    if(get_json_key_count(json)<=0)
    {
        result = -1;
        goto cleanup;
    }
    
    cJSON* format    = cJSON_GetObjectItem(json,   "format");
    cJSON* size      = cJSON_GetObjectItem(format, "size");
    if(size)
    {

        uint64_t sze = 0;;
        sscanf(size->valuestring,"%"SCNu64,&sze);
        file_size = bytes_to_humanreadable(sze);
    }

    cJSON* no_of_stream = cJSON_GetObjectItem(format,"nb_streams");


    result = 0;
cleanup:
    cJSON_Delete(json);
    free(data);
    free(file_size);
    return result;
}

#if 0
int main()
{
    Video v;
    VideoInfo info;
    video_get_info(&info,"/home/user/input.mp4");
    // video_init(&v,"/home/user/input.mp4");
    // video_rotate(&v,90);
    // video_fliph(&v);
    // video_flipv(&v);
    // video_rotate(&v,345);
    // video_scale_volume(&v,10);
    // generate_command(&v);
}
#endif
