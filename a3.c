#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define alignment 3072

void ping(int resp_fd){
    char response[512];
    memset(response, 0, 512);
    strcpy(response, "PING#");
    write(resp_fd, response, strlen(response));
    unsigned int uint_resp = 70144;
    write(resp_fd, &uint_resp, sizeof(unsigned int));
    strcpy(response, "PONG#");
    write(resp_fd, response, strlen(response));
}

void write_to_shm(int resp_fd, char* data, unsigned int offset,
                  unsigned int value, unsigned int size){
    char response[512];
    memset(response, 0, 512);
    if(offset > size || offset + sizeof(unsigned int) > size){
        strcpy(response, "WRITE_TO_SHM#ERROR#");
        write(resp_fd, response, strlen(response));
    }
   
    //  ar trebuie sa folosesc volatile pentru data, dar pare sa mearga si asa ???
    memcpy((void*)data + offset, (void*)&value, sizeof(unsigned int));
    strcpy(response, "WRITE_TO_SHM#SUCCESS#");
    write(resp_fd, response, strlen(response));
}

int main(int argc, char** argv){

    mkfifo("RESP_PIPE_70144", 0644);
    
    int req_fd = open("REQ_PIPE_70144", O_RDONLY);
    if(req_fd == -1){
        printf("ERROR\ncannot open the request pipe\n");
        return 0;
    }

    int resp_fd = open("RESP_PIPE_70144", O_WRONLY);
    if(resp_fd == -1){
        printf("ERROR\ncannot create the response pipe\n");
        return 0;
    }

    char s[6] = "START#";
    if(write(resp_fd, s, 6) == 6){
        printf("SUCCESS\n");
    }
    
    char input[512];
    char response[512];
    int shm_fd = -1, file_fd = -1;
    char file_name[1024];
    memset(file_name, 0, 1024);
    char* shm_data = NULL;
    char* file_data = NULL;
    unsigned int size = 0, file_size = 0;
    memset(input, 0, 512);

    for (;;)
    {
        memset(input, 0, 512);
        memset(response, 0, 512);
        int bytes_read = 0;
        while (bytes_read < sizeof(input)) {
            if(read(req_fd, input + bytes_read, 1) <= 0){
                break;
            } 
            bytes_read++;
            if (input[bytes_read - 1] == '#'){
                break;
            }
        }

        //  Ping
        if(strcmp(input, "PING#") == 0){
            ping(resp_fd);
            //  clear input buffer
            memset(input, 0, 512);
        }

        if(strncmp(input, "EXIT#", 5) == 0){
            break;
        }

        if(strncmp(input, "CREATE_SHM#", 11) == 0){
            //  shared memory creation
            shm_fd = shm_open("/anRO6T", O_RDWR | O_CREAT, 0664);
            
            read(req_fd, &size, sizeof(unsigned int));
            if(shm_fd == -1){
                strcpy(response, "CREATE_SHM#ERROR#");
                write(resp_fd, response, strlen(response));
            }else {
                ftruncate(shm_fd, size);
                shm_data = (char*)mmap(NULL, size, PROT_READ | PROT_WRITE
                , MAP_SHARED, shm_fd, 0);
                if(shm_data == (void*)-1){
                    //  exit
                    strcpy(response, "CREATE_SHM#ERROR#");
                    write(resp_fd, response, strlen(response));
                    continue;
                    return 0;
                }else {
                    strcpy(response, "CREATE_SHM#SUCCESS#");
                    write(resp_fd, response, strlen(response));
                }
            }
        }

        if(strncmp(input, "WRITE_TO_SHM#", 13) == 0){
            //  write memory
            unsigned int offset = 0, value = 0;
            read(req_fd, &offset, sizeof(unsigned int));
            read(req_fd, &value, sizeof(unsigned int));
            write_to_shm(resp_fd, shm_data, offset, value, size);

        }

        if(strncmp(input, "MAP_FILE#", 9) == 0){
            //  mapping file
            int count = 0;
            for(;;){
                //  read file name
                if(read(req_fd, file_name + count, 1) <= 0){
                    strcpy(response, "MAP_FILE#ERROR#");
                    write(resp_fd, response, strlen(response));
                    continue;
                }
                count++;
                if(file_name[count - 1] == '#'){
                    break;
                }
            }
            //  delete '#'
            file_name[count - 1] = 0;
            file_fd = open(file_name, O_RDONLY);
            if(file_fd == -1){
                strcpy(response, "MAP_FILE#ERROR#");
                write(resp_fd, response, strlen(response));
                continue;
            }
            file_size = lseek(file_fd, 0, SEEK_END);
            
            lseek(file_fd, 0, SEEK_SET);
            file_data = (char*)mmap(NULL, file_size, PROT_READ, MAP_SHARED,
            file_fd, 0);
            if(file_data == (void*)-1){
                strcpy(response, "MAP_FILE#ERROR#");
                write(resp_fd, response, strlen(response));
                continue;
            }
            strcpy(response, "MAP_FILE#SUCCESS#");
            write(resp_fd, response, strlen(response));
        }
        //  File offset read
        if(strncmp(input, "READ_FROM_FILE_OFFSET#", 22) == 0){
            unsigned int offset = 0, no_of_bytes = 0;
            if(file_data == (void*)-1){
                char response[512] = "READ_FROM_FILE_OFFSET#ERROR#";
                write(resp_fd, response, strlen(response));
                continue;
            }

            read(req_fd, &offset, sizeof(unsigned int));
            read(req_fd, &no_of_bytes, sizeof(unsigned int));
            
            if(offset > file_size || offset + no_of_bytes > file_size){
                char response[512] = "READ_FROM_FILE_OFFSET#ERROR#";
                write(resp_fd, response, strlen(response));
                continue;
            }
            
            memcpy((void*)shm_data, (void*)file_data + offset, no_of_bytes);
            char response[512] = "READ_FROM_FILE_OFFSET#SUCCESS#";
            write(resp_fd, response, strlen(response));
        }
        //  File section read
        if(strncmp(input, "READ_FROM_FILE_SECTION#", 23) == 0){
            unsigned int section_no = 0, offset = 0, no_of_bytes = 0;
            if(file_data == (void*)-1){
                char response[512] = "READ_FROM_FILE_SECTION#ERROR#";
                write(resp_fd, response, strlen(response));
                continue;
            }

            read(req_fd, &section_no, sizeof(unsigned int));
            read(req_fd, &offset, sizeof(unsigned int));
            read(req_fd, &no_of_bytes, sizeof(unsigned int));
            
            if(offset > file_size || offset + no_of_bytes > file_size){
                char response[512] = "READ_FROM_FILE_SECTION#ERROR#";
                write(resp_fd, response, strlen(response));
                continue;
            }

            //  parse header information
            unsigned int section_offset = 0;
            //  10 => header info
            //  section_no - 1 * 25 => start of section info
            //  +16 => start of sect offset
            unsigned int section_start = 10 + (section_no - 1) * 25 + 16;
            memcpy((void*)&section_offset, (void*)file_data + section_start, sizeof(unsigned int));
            if(section_offset + offset + no_of_bytes > file_size){
                char response[512] = "READ_FROM_FILE_SECTION#ERROR#";
                write(resp_fd, response, strlen(response));
                continue;
            }
            //printf("section start %u\n", section_offset);
            memcpy((void*)shm_data, (void*)file_data + section_offset + offset, no_of_bytes);
            
            char response[512] = "READ_FROM_FILE_SECTION#SUCCESS#";
            write(resp_fd, response, strlen(response));
        }

        if(strncmp(input, "READ_FROM_LOGICAL_SPACE_OFFSET#", 31) == 0){
            unsigned int logical_offset = 0, no_of_bytes = 0;
            read(req_fd, &logical_offset, sizeof(unsigned int));
            read(req_fd, &no_of_bytes, sizeof(unsigned int));
        
            if(file_data == (void*)-1 || shm_data == (void*)-1){
                strcpy(response, "READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#");
                write(resp_fd, response, strlen(response));
                continue;
            }
        
            unsigned char no_of_sections = *(file_data + 8);    
            unsigned int section_offset[20], section_size[20];
            unsigned int logical_start[20];
        
            for(int i = 0; i < no_of_sections; i++) {
                memcpy(&section_offset[i], file_data + 10 + i * 25 + 16, sizeof(unsigned int));
                memcpy(&section_size[i], file_data + 10 + i * 25 + 20, sizeof(unsigned int));
            }
        
            logical_start[0] = 0;   //  first section always starts at 0
            for(int i = 1; i < no_of_sections; i++) {
                unsigned int last_sect = logical_start[i-1] + section_size[i - 1];
                //  calculate the new aligned start
                unsigned int prev = last_sect / alignment;
                if(last_sect % alignment != 0){
                    prev++;
                }
                logical_start[i] = prev * alignment;
            }
        
            int found = 0;
            for(int i = 0; i < no_of_sections; i++) {
                unsigned int logical_end = logical_start[i] + section_size[i];
                //  check bounds
                if(logical_offset >= logical_start[i] && logical_offset + no_of_bytes <= logical_end) {
                    unsigned int file_offset = section_offset[i] + (logical_offset - logical_start[i]);
                    memcpy(shm_data, file_data + file_offset, no_of_bytes);
                    strcpy(response, "READ_FROM_LOGICAL_SPACE_OFFSET#SUCCESS#");
                    write(resp_fd, response, strlen(response));
                    found = 1;
                    break;
                }
            }
        
            if(!found) {
                strcpy(response, "READ_FROM_LOGICAL_SPACE_OFFSET#ERROR#");
                write(resp_fd, response, strlen(response));
            }
        }
    } 
        
    

    close(req_fd);
    close(resp_fd);
    munmap((void*)shm_data, 4713675);
    close(shm_fd);
    shm_unlink("/anRO6T");
    unlink("RESP_PIPE_70144");
    return 0;
}