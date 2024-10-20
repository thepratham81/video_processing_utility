#ifndef _FILE_UITL_H_
#include <stdio.h>
#include <limits.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <shlobj.h>
    #define PATH_SEP '\\'
#else

    #include <unistd.h>
    #define PATH_SEP '/'

#endif

long get_file_size(FILE *file);
char *add_extension(const char *file_name , const char *extension);
char *str_duplicate(const char * str);
char *get_basename(const char *path);
char *get_dirname(const char *path);
int is_file(const char *const path);
int is_dir(const char *const path);
int is_valid_path(const char *const path);
char *get_filename(const char *path);
char *get_extension(const char *path);
void get_video_folder(char *path);
void get_executable_path(char* path);
char *join_path(const char *base, ...);
#endif // _FILE_UITL_H_
