#include "file_util.h"
#include "cstring.h"


#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <commdlg.h>

char *generate_filter(const char *filter){
    char *res = String_from(filter);
    size_t n = str_len(res);
    for(size_t i = 0 ; i<n;i++){
        if(res[i]=='|') res[i] = '\0';
    }
    vector_append(res,'\0');
    return res;
}
char **_get_selected_files(OPENFILENAME *opendialog) {
    String **result = Vector(*result);
    if (GetOpenFileName(opendialog)) {
        char *p = opendialog->lpstrFile;
        char *can_be_dir = String_from(p);
        p += strlen(p) + 1;
        while (*p) {
            String *temp = String_from(can_be_dir);
            str_cat(&temp,"\\",p);
            vector_append(result,temp);
            p += strlen(p) + 1;
        }
        if(vector_length(result)>0){
            free_string(can_be_dir);
        }else{
            vector_append(result,can_be_dir);
        }
    }
    return result;
}

char ** open_file_dialog(const char *initial_dir , const char *filter) {
    OPENFILENAME opendialog = {0};
    char szFile[1024] = {0};
    char *filter_string = generate_filter(filter);
    opendialog.lStructSize = sizeof(opendialog);
    opendialog.hwndOwner = GetForegroundWindow();
    opendialog.hInstance = GetModuleHandle(NULL);
    opendialog.lpstrFile = szFile;
    opendialog.nFilterIndex = 0;
    opendialog.nMaxFile = sizeof(szFile);
    if(is_dir(initial_dir)){
        opendialog.lpstrInitialDir = initial_dir;
    }
    opendialog.lpstrFilter = filter_string;
    opendialog.nFilterIndex = 1;
    opendialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    char **result = _get_selected_files(&opendialog);
    free_vector(filter_string);
    return result;
}
#else

char **open_file_dialog(char *initial_dir , char *filter) {
    char result[1024] = "yad --file-selection";
    if(filter){

    }
    return NULL;
}
#endif
