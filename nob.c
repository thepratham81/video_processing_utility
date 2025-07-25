#include <sys/stat.h>
#include <stdarg.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#define IMPLEMENT_VECTOR
#define INIT_VECTOR_CAPACITY 1<<10
#include "./vector.h"



#ifdef _WIN32
    #define PATH_SEP '\\'
#else
    #define PATH_SEP '/'
#endif

#define CIMGUI_PATH "cimgui"
#define APP_NAME "vpt"
int join_path(char* memory,...)
{
    va_list args;
    va_start(args, memory);
    char* tmp = NULL;
    int res = vector_length(memory);
    while((tmp=va_arg(args,char*))!=NULL)
    {
        while(*tmp) vector_append(memory,*tmp++);
        vector_append(memory,PATH_SEP);
    }
    vector_pop(memory);
    vector_append(memory,'\0');
    va_end(args);
    return res;
}

int join_str(char* memory,...)
{
    va_list args;
    va_start(args, memory);
    char* tmp = NULL;
    int res = vector_length(memory);
    while((tmp=va_arg(args,char*))!=NULL)
    {
        while(*tmp) vector_append(memory,*tmp++);
    }
    vector_append(memory,'\0');
    va_end(args);
    return res;
}
int folder_exists(const char* path)
{
    struct stat s;
    if (stat(path, &s) == 0 && S_ISDIR(s.st_mode))
        return 1;
    return 0;
}


int main(int argc, char** argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    char* mem = Vector(*mem);

    char* MAIN_FILE = &mem[join_path(mem,"src","main.c",NULL)];;
    char* MOVIEC_C  = &mem[join_path(mem,"src","moviec.c",NULL)];;
    char* backend_folder  = &mem[join_path(mem,CIMGUI_PATH,"imgui","backends",NULL)];
    if(!folder_exists(backend_folder))
    {
        backend_folder    = &mem[join_path(mem,CIMGUI_PATH,"imgui","examples",NULL)];
    }


    char* table_source    = &mem[join_path(mem,CIMGUI_PATH,"imgui","imgui_tables.cpp",NULL)];
    if(!nob_file_exists(table_source))
    {
        table_source      = "";
    }

    if (!folder_exists(CIMGUI_PATH))
    {
        nob_cmd_append(&cmd                                  , 
                       "git"                                 ,
                       "clone"                               , 
                       "--recursive"                         ,
                       "https://github.com/cimgui/cimgui.git",
                       "--depth=1"                           ,
                       "--shallow-submodules"
                       );
        if(!nob_cmd_run_sync_and_reset(&cmd)) {goto fail;}
    }
    
    char* IMGUI_PATH          = &mem[join_path(mem,CIMGUI_PATH,"imgui",NULL)];
    char* imgui_include_path  = &mem[join_str(mem,"-I",IMGUI_PATH,NULL)];
    char* cimgui_include_path = &mem[join_str(mem,"-I",CIMGUI_PATH,NULL)];
   if(!nob_file_exists("libcimgui.a"))
   {
        nob_cmd_append(&cmd,
                   "g++",
                   "-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1",
                   "-DIMGUI_IMPL_API=extern \"C\"",
                    "-DIMGUI_USER_CONFIG=\"../cimconfig.h\"",
                   "-DIMGUI_IMPL_OPENGL_LOADER_GL3W",
                   "-std=gnu++11",
                   "-c",
                   "-O0",
                   imgui_include_path,
                   cimgui_include_path,
                   &mem[join_path(mem,CIMGUI_PATH,"cimgui.cpp",NULL)],
                   &mem[join_path(mem,IMGUI_PATH,"imgui.cpp",NULL)],
                   &mem[join_path(mem,IMGUI_PATH,"imgui_draw.cpp",NULL)],
                   &mem[join_path(mem,IMGUI_PATH,"imgui_demo.cpp",NULL)],
                   &mem[join_path(mem,IMGUI_PATH,"imgui_widgets.cpp",NULL)],
                   &mem[join_path(mem,backend_folder,"imgui_impl_opengl3.cpp",NULL)],
                   &mem[join_path(mem,backend_folder,"imgui_impl_glfw.cpp",NULL)],

                   table_source
                   );

        if (!nob_cmd_run_sync_and_reset(&cmd))
            {goto fail;}

        nob_cmd_append(&cmd                  ,
                       "ar"                  ,
                       "rcs"                 ,
                       "libcimgui.a"         ,
                       "imgui.o"             ,
                       "imgui_draw.o"        ,
                       "imgui_demo.o"        ,
                       "imgui_widgets.o"     ,
                       "imgui_tables.o"      ,
                       "imgui_impl_opengl3.o",
                       "imgui_impl_glfw.o"   ,
                       "cimgui.o"
                       );

        if (!nob_cmd_run_sync_and_reset(&cmd))
            {goto fail;}
        }
        nob_cmd_append(&cmd,
                    "gcc",imgui_include_path,cimgui_include_path,
                       MAIN_FILE,"-c","-o","main.o"
                       );

    if (!nob_cmd_run_sync_and_reset(&cmd))
        {goto fail;}

    nob_cmd_append(&cmd                                     ,
                    "gcc"                                   ,
                    imgui_include_path                      ,
                    cimgui_include_path                     ,
                    "-DCIMGUI_USE_GLFW"                     ,
                    "-DCIMGUI_USE_OPENGL3"                  ,
                    "-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1"  ,
                    "-DIMGUI_IMPL_API=extern \"C\""         ,
                    "-DIMGUI_USER_CONFIG=\"../cimconfig.h\"",
                    "-DIMGUI_IMPL_OPENGL_LOADER_GL3W"       ,
                    MAIN_FILE                               ,
                   "-c"                                     ,
                   "-std=c11"                               ,
                   "-pedantic"                              ,
                   "-Wall"                                  ,
                   // "-Werror"                                ,
                   "-o"                                     ,
                   "main.o"
                   );

    if (!nob_cmd_run_sync_and_reset(&cmd))
        {goto fail;}

    nob_cmd_append(&cmd,
                   "gcc",
                   MOVIEC_C,
                   "-c",
                   "-std=c99",
                   "-pedantic",
                   "-Wall",  
                   "-o",
                   "moviec.o"
                   );
     if (!nob_cmd_run_sync_and_reset(&cmd))
        {goto fail;}

    nob_cmd_append(&cmd         ,
                   "g++"        ,
                   "main.o",
                   "moviec.o",
                   "libcimgui.a",
                   "-o"         ,
                   APP_NAME     ,
                   "-lGL"       ,
                   "-lglfw3"
                );

    if (!nob_cmd_run_sync_and_reset(&cmd))
        {goto fail;}
    free_vector(mem); 
    return 0;
fail:

    free_vector(mem); 
    return 1;
}
