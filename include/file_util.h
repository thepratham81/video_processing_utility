#ifndef _FILE_UITL_H_
#include<stdio.h>
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
#endif // _FILE_UITL_H_
