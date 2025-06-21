#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

void get_path(char **path, char *full_path){
    *path = strtok(full_path, "=");
        if(strcmp(*path, "path") !=0){
            printf("ERROR\n");
            printf("wrong file\n");
            return;
        }
    //  Get path
    *path = strtok(NULL, " ");
}

int get_filter(char *initial_filter, char **filter){
    if(strstr(initial_filter, "size_smaller") == NULL &&
        strstr(initial_filter, "name_ends_with") == NULL){
            printf("ERROR\nInvalid options\n");
            return -1;
        }
        //  Check what option was given
        *filter = strtok(initial_filter, "=");
        if(strstr(*filter, "size") != NULL){
            *filter = strtok(NULL, " ");
            return 1;   //  1 => size filter
        }
        *filter = strtok(NULL, " ");
        return 2;   //  2 => name filter
}


int list_directory(const char* path, int type, char *filter){
    DIR *directory = NULL;
    struct dirent *entry = NULL;
    struct stat statbuf;
    char file_path[512];
    directory = opendir(path);
    //  Check path
    if(directory == NULL){
        printf("ERROR\ninvalid directory path\n");
        return -1;
    }
    printf("SUCCESS\n");
    while((entry = readdir(directory)) != NULL){
        if(strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0){
                if(type == 0){
                    //  No filters
                    printf("%s/%s\n",path, entry->d_name);
                }
                else if (type == 1){
                    //  Check for size
                    snprintf(file_path, 512, "%s/%s", path, entry->d_name);
                    if(lstat(file_path, &statbuf) == 0){
                        //  If it's a file
                        if(S_ISREG(statbuf.st_mode)){
                            //  Get size
                            int fd = open(file_path, O_RDONLY);
                            int size = lseek(fd, 0, SEEK_END);
                            close(fd);
                            int filter_value = atoi(filter);
                            if(size < filter_value){
                                printf("%s/%s\n",path, entry->d_name);
                            }
                        }
                    }
                }else if (type == 2){
                    //  Check for name
                    int suffix_size = 0;
                    suffix_size = strlen(filter);
                    snprintf(file_path, 512, "%s/%s", path, entry->d_name);
                    if(suffix_size == 0){
                        return -1;
                    }
                    if(strcmp(file_path + strlen(file_path) - suffix_size, filter) == 0){
                        printf("%s/%s\n",path, entry->d_name);
                    }
                }
            }
    }
    closedir(directory);
    return 0;
}

int list_directory_recursive(const char* path, int type, char *filter){
    DIR *directory = NULL;
    struct dirent *entry = NULL;
    //  List all items of current directory
    directory = opendir(path);
    if(directory == NULL){
        printf("Could not open directory");
        return -1;
    }
    while((entry = readdir(directory)) != NULL){
        if(strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0){
                char fullPath[512];
                struct stat buf_stat;
                snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
                if(lstat(fullPath, &buf_stat) == 0){
                    if(type == 0){
                        printf("%s\n", fullPath);
                    }else if (type == 1){
                        if(S_ISREG(buf_stat.st_mode)){
                            //  Get size
                            int fd = open(fullPath, O_RDONLY);
                            int size = lseek(fd, 0, SEEK_END);
                            close(fd);
                            int filter_value = atoi(filter);
                            if(size < filter_value){
                                printf("%s/%s\n",path, entry->d_name);
                            }
                        }
                    }else if (type == 2){
                        //  Check for name
                        int suffix_size = 0;
                        suffix_size = strlen(filter);
                        snprintf(fullPath, 512, "%s/%s", path, entry->d_name);
                        if(suffix_size == 0){
                            return -1;
                        }
                        if(strcmp(fullPath + strlen(fullPath) - suffix_size, filter) == 0){
                            printf("%s/%s\n",path, entry->d_name);
                        }
                    }
                    if(S_ISDIR(buf_stat.st_mode)){
                        list_directory_recursive(fullPath, type, filter);
                    }
                }
            }
    }
    closedir(directory);
    return 0;
}

typedef struct{
    char magic[2];
    u_int16_t header_size;
    u_int8_t no_of_sections;
    u_int32_t version;    
    char sect_name[2000][14];
    u_int32_t sect_type[2000];
    u_int32_t sect_offset[2000];
    u_int32_t sect_size[2000];
}MY_HEADER;

MY_HEADER* read_header(const int fd){
    MY_HEADER *header = malloc(sizeof(MY_HEADER));
    if(header == NULL){
        printf("Failed @read_header allocating header\n");
        return NULL;
    }
    memset(header, 0 , sizeof(MY_HEADER));
    if(read(fd, &header->magic, 2) <= 0) {
        free(header);
        return NULL;
    }
    if(read(fd, &header->header_size, 2) <= 0) {
        free(header);
        return NULL;
    }

    if(read(fd, &header->version, 4) <= 0) {
        free(header);
        return NULL;
    }
    if(read(fd, &header->no_of_sections, 1) <= 0) {
        free(header);
        return NULL;
    }
    for(int i = 0; i < header->no_of_sections; i++){
        if(read(fd, &header->sect_name[i], 13) <= 0) {
            free(header);
            return NULL;
        }
        if(read(fd, &header->sect_type[i], 4) <= 0) {
            free(header);
            return NULL;
        }
        if(read(fd, &header->sect_offset[i], 4) <= 0) {
            free(header);
            return NULL;
        }
        if(read(fd, &header->sect_size[i], 4) <= 0) {
            free(header);
            return NULL;
        }
    }
    return header;
}

int parse(const char *path){
    MY_HEADER *header = NULL;
    int fd = -1;
    fd = open(path, O_RDONLY);
    if(fd == -1){
        printf("Wrong path @parse\n");
        return -1;
    }
    header = read_header(fd);
    if(header == NULL){
        close(fd);
        return -1;
    }
        if(header->magic[0] != 'q' || header->magic[1] != '8'){
        printf("ERROR\nwrong magic\n");
        close(fd);
        free(header);
        return -1;
    }
        if(header->version < 35 || header->version > 124){
        printf("ERROR\nwrong version\n");
        close(fd);
        free(header);
        return -1;
    }
    if(header->no_of_sections != 2){
        if(header->no_of_sections < 4 || header->no_of_sections > 20){
            printf("ERROR\nwrong sect_nr\n");
                close(fd);
                free(header);
                return -1;
        }
    }
    for(int i = 0; i < header->no_of_sections; i++){
        if(header->sect_type[i] != 94 && 
           header->sect_type[i] != 38 &&
           header->sect_type[i] != 97){
            printf("ERROR\nwrong sect_types\n");
            free(header);
            close(fd);
            return -1;
           }
    }
    

    printf("SUCCESS\n");
    printf("version=%d\n", header->version);
    printf("nr_sections=%d\n", header->no_of_sections);
    for(int i = 0; i < header->no_of_sections; i++){
        printf("section%d: %s %d %d\n", i+1, header->sect_name[i], header->sect_type[i], header->sect_size[i]);
    }
    free(header);
    close(fd);        
    return 0;
}

int parse_no_print(const char *path){
    MY_HEADER *header = NULL;
    int fd = -1;
    fd = open(path, O_RDONLY);
    if(fd == -1){
        printf("Wrong path @parse\n");
        return -1;
    }
    header = read_header(fd);
    if(header == NULL){
        close(fd);
        return -1;
    }
        if(header->magic[0] != 'q' || header->magic[1] != '8'){
        close(fd);
        free(header);
        return -1;
    }
        if(header->version < 35 || header->version > 124){
        close(fd);
        free(header);
        return -1;
    }
    if(header->no_of_sections != 2){
        if(header->no_of_sections < 4 || header->no_of_sections > 20){
                close(fd);
                free(header);
                return -1;
        }
    }
    for(int i = 0; i < header->no_of_sections; i++){
        if(header->sect_type[i] != 94 && 
           header->sect_type[i] != 38 &&
           header->sect_type[i] != 97){
            close(fd);
            free(header);
            return -1;
           }
    }
    
    free(header);
    close(fd);        
    return 0;
}

void reverse_string(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int extract(const char* path, const int sect_nr, const int line_nr){
    MY_HEADER header;
    memset(&header, 0, sizeof(header));
    int fd = -1;

    fd = open(path, O_RDONLY);
    if(fd == -1){
        printf("ERROR\nwrong path\n");
        return -1;
    }
    //  magic
    if(read(fd, &header.magic, 2) <= 0) {
        close(fd);
        return -1;
    }
    //  header size
    if(read(fd, &header.header_size, 2) <= 0){
        close(fd);
        return -1;
    }
    
    //  version
    if(read(fd, &header.version, 4) <=0){
        close(fd);
        return -1;
    }
    //  no_of_sections    
    if(read(fd, &header.no_of_sections, 1) <= 0){
        close(fd);
        return -1;
    }
    for(int i = 0; i < header.no_of_sections; i++){
        if(read(fd, &header.sect_name[i], 13) <= 0){
            close(fd);
            return -1;
        }
        if(read(fd, &header.sect_type[i], 4) <= 0){
            close(fd);
            return -1;
        }
        if(read(fd, &header.sect_offset[i], 4) <= 0){
            close(fd);
            return -1;
        }
        if(read(fd, &header.sect_size[i], 4) <= 0){
            close(fd);
            return -1;
        }
    }

    int offset = header.sect_offset[sect_nr - 1];
    //  move cursor to offset
    lseek(fd, offset, SEEK_SET);
    char *buf = malloc(header.sect_size[sect_nr - 1] + 1);
    if(buf == NULL){
        close(fd);
        return -1;
    }
    memset(buf, 0, header.sect_size[sect_nr - 1] + 1);
    read(fd, buf, header.sect_size[sect_nr - 1]);
    char *p = strtok(buf, "\r\n");
    int line_no = 0;
    while(p){
        line_no++;
        if(line_no == line_nr){
            reverse_string(p);
            printf("SUCCESS\n%s\n", p);
            free(buf);
            close(fd);
            return 0;
        }
        p = strtok(NULL, "\r\n");
    }
    free(buf);
    close(fd);
    return -1;
}

int findall(const char* path){
    DIR *directory = NULL;
    struct dirent *entry = NULL;
    directory = opendir(path);
    if(directory == NULL){
        printf("ERROR\ninvalid directory path\n");
        return -1;
    }
    while((entry = readdir(directory)) != NULL){
        if(strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0){
                char fullPath[50000];
                memset(fullPath, 0, 50000);
                struct stat buf_stat;
                snprintf(fullPath, 50000, "%s/%s", path, entry->d_name);
                if(lstat(fullPath, &buf_stat) == 0){
                    if(S_ISDIR(buf_stat.st_mode)){
                        //  call recursively on next directory
                        findall(fullPath);
                    }
                    if(S_ISREG(buf_stat.st_mode)){
                        //  normal file
                        MY_HEADER *header = NULL;
                        int fd = -1;
                        fd = open(fullPath, O_RDONLY);
                        if(fd == -1){
                            printf("ERROR opening file @findall\n");
                            return -1;
                        }
                        if(parse_no_print(fullPath) == 0){
                            header = read_header(fd);
                            
                            for(int i = 0; i < header->no_of_sections; i++){
                                int offset = header->sect_offset[i];
                                lseek(fd, offset, SEEK_SET);
                                char *buf = malloc(header->sect_size[i] + 1);
                                if (buf == NULL) {
                                    close(fd);
                                    closedir(directory);
                                    return -1;
                                }
                                memset(buf, 0, header->sect_size[i] + 1);
                                read(fd, buf, header->sect_size[i]);
                                char *p = strtok(buf, "\r\n");
                                int line_no = 0;
                                int found = 0;
                                while(p){
                                    line_no++;
                                    if(line_no > 13){
                                        found = 1;
                                        printf("%s\n", fullPath);
                                        break;  //  exit while loop
                                    }
                                    p = strtok(NULL, "\r\n");
                                }
                                free(buf);  
                                if(found){
                                    break;  //  exit for loop
                                }
                            }
                            close(fd); 
                        }
                        if(header != NULL){
                            free(header);
                        }
                        
                    }
                }
                
            }
    }
    closedir(directory);

    return 0;
}

int main(int argc, char **argv)
{
    if(argc >= 2) {
        if(strcmp(argv[1], "variant") == 0) {
            printf("70144\n");
            return 0;
        }
        if(strcmp(argv[1], "parse") == 0 || (argc == 3 && strcmp(argv[2], "parse")==0)){
            char *path;
            for(int i = 1; i < argc; i++){
                if(strstr(argv[i], "path") != NULL){
                    //  contains path
                    get_path(&path, argv[i]);
                }
            }
            for(int i = 1; i < argc; i++){
                if(strstr(argv[i], "parse") != NULL){
                    return parse(path);
                }
            }
        }
        if(strcmp(argv[1], "extract") == 0){
            char *path;
            int sect_nr = 0;
            int line_nr = 0;
            for(int i = 1; i < argc; i++){
                if(strstr(argv[i], "path") != NULL){
                    //  contains path
                    get_path(&path, argv[i]);
                }
                if(strstr(argv[i], "section") != NULL){
                    char* p = strtok(argv[i], "=");
                    p = strtok(NULL, "=");
                    sect_nr = atoi(p);
                }
                if(strstr(argv[i], "line") != NULL){
                    char* p = strtok(argv[i], "=");
                    p = strtok(NULL, "=");
                    line_nr = atoi(p);
                }
            }
    
            return extract(path, sect_nr, line_nr);
        }
        if(strcmp(argv[1], "findall") == 0){
            char *path;
            get_path(&path, argv[2]);
            //  im just assuming this is the order
            if(path != NULL){
                printf("SUCCESS\n");    
            }
            return findall(path);
        }
        if(strcmp(argv[1], "list") == 0){
            if(argc == 3){
                //  Only path is specified
                char *path = NULL;
                get_path(&path, argv[2]);
                if(path == NULL){
                    printf("ERROR\ninvalid directory path");
                }
                int res = list_directory(path, 0, NULL);
                return res;
            }
            if(argc == 4){
                //  holy spaghetti code

                //  Either recursive is specified or filters
                if(strstr(argv[2], "path") != NULL){
                    //  argv[2] contains path
                    char* path = NULL;
                    get_path(&path, argv[2]);
                    if(path == NULL){
                        return -1;
                    }
                    printf("SUCCESS\n");
                    //  path + recursive case
                    if(strstr(argv[3], "recursive")!= NULL){
                        int res = list_directory_recursive(path, 0, NULL);
                        return res;
                        
                    }else {
                        //  path + filtering options
                        char *path = NULL;
                        char *filter = NULL;
                        get_path(&path, argv[2]);
                        int type = get_filter(argv[3], &filter);
                        if(type == -1){
                            printf("ERROR\nfailed parsing filter\n");
                            return -1;
                        }
                        int res = list_directory(path, type, filter);
                        return res;
                    }
                }else {
                    //  recursive + path
                    if(strstr(argv[2], "recursive")!= NULL){
                        char* path = NULL;
                        //  get_path checks for a valid command
                        get_path(&path, argv[3]);
                        if(path == NULL){
                            return -1;
                        }
                        printf("SUCCESS\n");
                        int res = list_directory_recursive(path, 0, NULL);
                        if(!res){
                            //  Error
                            return res;
                        }
                    }else {
                        //  filtering options + path
                        char *path = NULL;
                        char *filter = NULL;
                        get_path(&path, argv[3]);
                        int type = get_filter(argv[2], &filter);
                        if(type == -1){
                            printf("ERROR\nfailed parsing filter\n");
                            return -1;
                        }
                        //  Fails here
                        int res = list_directory(path, type, filter);
                        return res;
                    }
            }
        } else {
            if (argc == 5){
                //  rec + options + path
                char parameters[512];
                //  build string
                snprintf(parameters, 512, "%s %s %s", argv[2], argv[3], argv[4]);
                //  parse parameters
                char *filter;
                char *path;
                if(strstr(argv[4], "path") != NULL){
                    get_path(&path, argv[4]);
                }
                if(strstr(argv[3], "path") != NULL){
                    get_path(&path, argv[3]);
                }
                if(strstr(argv[2], "path") != NULL){
                    get_path(&path, argv[2]);
                }
                int type = -1;
                if(strstr(parameters, "recursive") == NULL){
                    //  wrong number of args if there is no rec
                    printf("ERROR\nWrong arguments given\n");
                    return -1;
                }
                char *p = strtok(parameters, " ");
                while(p){
                    //  get options
                    if(strstr(p, "name_ends_with") != NULL
                            || strstr(p, "size_smaller") != NULL){
                                type = get_filter(p, &filter);
                            }
                    p = strtok(NULL, " ");
                }
                int res = -1;
                if(path != NULL && filter != NULL && type != -1){
                    printf("SUCCESS\n");
                    res = list_directory_recursive(path, type, filter);
                }
                
                return res;
            }
        }
    }
    return 0;
}
}