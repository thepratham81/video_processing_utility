#include "file_util.h"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <commdlg.h>

char *generate_filter(char *filter){
    size_t n = strlen(filter);
    char *res = malloc(n+1);
    if(!res) return NULL;
    memcpy(res,filter,n+1);
    for(size_t i = 0 ; i<n;i++){
        if(res[i]=='|') res[i] = '\0';
    }
    return res;
}

void open_file_dialog(char *initial_dir , char *filter) {
    OPENFILENAME opendialog = {0};
    char szFile[1024] = {0};
    char *filter_string = generate_filter(filter);
    opendialog.lStructSize = sizeof(opendialog);
    opendialog.hwndOwner = GetForegroundWindow();;
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
    GetOpenFileName(&opendialog);
    free(filter_string);
}
#else

void open_file_dialog(char *initial_dir , char *filter) {
    char result[1024] = "yad --file-selection";
    if(filter){

    }
}
#endif
int main() {
    open_file_dialog("/home","Text file |*.txt|Image file|*.jpg");
}
