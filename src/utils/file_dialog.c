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


char* select_folder_dialog(char *initial_dir) {
    IFileDialog *pfd;
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileDialog, (void**)&pfd);
        if (SUCCEEDED(hr)) {
            DWORD options;
            pfd->lpVtbl->GetOptions(pfd, &options);
            pfd->lpVtbl->SetOptions(pfd, options | FOS_PICKFOLDERS);  // Enable folder selection

            hr = pfd->lpVtbl->Show(pfd, GetForegroundWindow());
            if (SUCCEEDED(hr)) {
                IShellItem *psi;
                hr = pfd->lpVtbl->GetResult(pfd, &psi);
                if (SUCCEEDED(hr)) {
                    PWSTR path;
                    psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &path);

                    char folderPath[MAX_PATH];
                    wcstombs(folderPath, path, MAX_PATH);

                    CoTaskMemFree(path);
                    psi->lpVtbl->Release(psi);
                    pfd->lpVtbl->Release(pfd);
                    CoUninitialize();
                    return str_duplicate(folderPath);
                }
            }
            pfd->lpVtbl->Release(pfd);
        }
        CoUninitialize();
    }
    return NULL;
}

#else
typedef enum {DLG_OPEN,DLG_FOLDER,DLG_OPEN_MULTISELECT}DialogType;

char *generate_filter(const char *filter){
    char *temp = String_from(filter);
    size_t n = str_len(temp);
    for(size_t i = 0 ; i<n;i++){
        if(temp[i]==';') temp[i] = ' ';
    }
    vector_append(temp,'\0');
    char **splited = str_split(temp, "|");
    char *result = String_init(); 
    for(size_t i = 0 ; i < vector_length(splited)-1;i+=2){
        str_cat(&result,
                "--file-filter ",
                "\"",splited[i],"|",
                splited[i+1],
                "\"",
                NULL); 
                free_string(splited[i]);
                free_string(splited[i+1]);
    }
    free_vector(splited);
    free_string(temp);
    return result;
}

int is_command_installed(const char *cmd) {
    FILE *fp;
    int found = 0;

    char command[256];
    snprintf(command, sizeof(command), "command -v %s > /dev/null 2>&1", cmd);
    fp = popen(command, "r");
    if (fp == NULL) return 0;

    if (pclose(fp) == 0) found = 1;

    return found;
}

String *get_zenity_command(int dlg_type){
    int zenity_installed = is_command_installed("zenity");
    if(!zenity_installed) return NULL;
    switch(dlg_type){
        case DLG_OPEN_MULTISELECT:
            return String_from("zenity --file-selection --multiple --attach=$(xdotool getactivewindow) ");
        case DLG_FOLDER:
            return String_from("zenity --file-selection --directory --attach=$(xdotool getactivewindow) ");
    }
    return NULL;
}

String *get_yad_command(int dlg_type){
    int yad_installed = is_command_installed("yad");
    if(!yad_installed) return NULL;
    switch(dlg_type){
        case DLG_OPEN_MULTISELECT:
            return String_from("yad --file-selection --multiple --attach=$(xdotool getactivewindow) ");
        case DLG_FOLDER:
            return String_from("yad --file-selection --directory --attach=$(xdotool getactivewindow) ");
    }
    return NULL;
}



String *_execute_and_capture_output(const char *command){
    char temp[PATH_MAX];
    FILE *fp = popen(command,"r");
    if (fp == NULL) {
        perror("popen failed");
        return NULL;
    }

    char *process_stdout = String_init();
    while (fgets(temp, sizeof(temp), fp) != NULL) {
        str_cat(&process_stdout,temp,NULL);
    }

    pclose(fp);
    return process_stdout;

}

char *select_folder_dialog(char *initial_dir){
    String *cmd = get_zenity_command(DLG_FOLDER);
    if(!cmd){
        cmd = get_yad_command(DLG_FOLDER);
    }
    char *process_stdout = _execute_and_capture_output(cmd);
    if(!process_stdout){
        return NULL;
    }
    size_t output_len = str_len(process_stdout);
    if(process_stdout[output_len-1]=='\n'){
        process_stdout[output_len-1]='\0';
    }
    return process_stdout;
}
char **open_file_dialog(char *initial_dir , char *filter) {
    String *cmd = get_zenity_command(DLG_OPEN_MULTISELECT);
    if(!cmd){
        cmd = get_yad_command(DLG_OPEN_MULTISELECT);
    }
    if(!cmd) {
        return NULL;
    }

    char *res = generate_filter(filter);
    str_cat(&cmd,res,NULL);
    free_string(res);
    char temp[PATH_MAX];
    char *process_stdout = _execute_and_capture_output(cmd);
    if(!process_stdout){
        return NULL;
    }

    size_t output_len = str_len(process_stdout);
    if(process_stdout[output_len-1]=='\n'){
        process_stdout[output_len-1]='\0';
    }
    char **result = str_split(process_stdout,"|");
    
    free_string(process_stdout);
    return result;
}
#endif
