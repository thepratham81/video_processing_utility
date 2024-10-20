
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "file_util.h"
#include<libgen.h>

#if defined(_WIN32) || defined(_WIN64)

void get_video_folder(char *path) {
    SHGetFolderPathA(NULL, CSIDL_MYVIDEO, NULL, 0, path);
}

void get_executable_path(char* path) {
    GetModuleFileNameA(NULL, path, PATH_MAX);
}

#else

void get_video_folder(char *path) {
    const char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(path, PATH_MAX, "%s/Videos", home);
    } else {
        strcpy(path, "./Videos");
    }
}


void get_executable_path(char* path) {
    readlink("/proc/self/exe", path, PATH_MAX);
}

#endif


char *str_duplicate(const char * str){
    if(!str){
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *new_str = malloc(len);
    if (new_str) {
        strcpy(new_str, str);
    }

    return new_str;
}



long get_file_size(FILE *file){
    long file_size = -1;
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return file_size;
}
char *add_extension(const char *file_name , const char *extension){
    char *new_file_name = malloc(strlen(file_name)+strlen(extension)+1);

    if(!new_file_name) return NULL;

    strcpy(new_file_name,file_name);
    strcat(new_file_name,extension);
    return new_file_name;
}


char *get_dirname(const char *path){
    char *_path = malloc(strlen(path)+1);

    if(!path){
        return NULL;
    } 

    char *temp = _path;
    strcpy(_path,path);
    char *dir = str_duplicate(dirname(_path));
    free(temp);
    return dir;
}


char *get_basename(const char *path) {
    if (!path) {
        return NULL;
    }

    char *_path = str_duplicate(path);
    if (!_path) {
        return NULL; 
    }

    
    char *base = str_duplicate(basename(_path));
    if (!base) {
        free(_path);
        return NULL;
    }
    
    free(_path); 
    return base;
}

char *get_extension(const char *path){
    if(!path) return NULL;
    const char *temp = path;
    const char *dot = strrchr(temp,'.');
     if (!dot || dot == path) {
        return NULL;
    }
    return str_duplicate(dot);
}

char *get_filename(const char *path){
    if(!path) return NULL;
    const char *temp = path;
    const char *dot = strrchr(temp,'.');
     if (!dot || dot == path) {
        return NULL;
    }
    char *result = malloc(dot-path + 1);
    strncpy(result, path, dot-path);
    result[dot-path] = '\0';
    return result;
}

int is_file(const char *const path){
    struct stat st;
    stat(path,&st);
    return S_ISREG(st.st_mode);
}

int is_dir(const char *const path){
    struct stat st;
    stat(path,&st);
    return S_ISDIR(st.st_mode);
}

int is_valid_path(const char *const path){
    if(access(path,F_OK)!=-1){
        return 1;
    }
    return 0;
}

#ifdef TEST
int main(){
    char *file = "/home/user/a.bc.txt";
    puts(get_basename("/usr"));
    puts(get_extension(file));
    puts(get_dirname(file));
    puts(add_extension(file,"mps"));
    puts(get_filename(file));
}
#endif

