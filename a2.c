#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

typedef struct _proc7{
    int threadID;
    int procID;
    sem_t* sem_71;
    sem_t* sem_73;
    sem_t* sem_34_74;
    sem_t* sem_36_74;
}proc7;

void* process7_function(void *arg){
    proc7* th_info = (proc7*)arg;

    if(th_info->threadID == 1){
        //  Thread 1
        info(BEGIN, th_info->procID, th_info->threadID);    //  Thread1 starts
        sem_post(th_info->sem_73);      //  Tell thread3 to start
        sem_wait(th_info->sem_71);      //  Wait for thread3 to finish
        info(END, th_info->procID, th_info->threadID);      //  Thread1 ends
    }else if(th_info->threadID == 3){
        //  Thread 3
        sem_wait(th_info->sem_73);      //  Start thread3
        info(BEGIN, th_info->procID, th_info->threadID);    //  Do job
        info(END, th_info->procID, th_info->threadID);      //  End thread3
        sem_post(th_info->sem_71);      //  Tell thread1 to end
    }else if(th_info->threadID == 4)
    {   //  Thread4
        sem_wait(th_info->sem_34_74);   //  Wait for thread 3-4 to finish
        info(BEGIN, th_info->procID, th_info->threadID);    //  Do job
        info(END, th_info->procID, th_info->threadID); 
        sem_post(th_info->sem_36_74);   //  Tell thread 3-6 to start
    }else {
        //  Other threads
        info(BEGIN, th_info->procID, th_info->threadID);
        info(END, th_info->procID, th_info->threadID);
    }
    return NULL;
}

typedef struct _proc5{
    int threadID;
    int procID;
    sem_t* main_sem;
    sem_t* sem15;
    sem_t* sem_count;   
    sem_t* exit_sem_2;
}proc5;

int th_count = 48;

void* process5_function(void *arg){
    proc5* th_info = (proc5*)arg;
    int local_count = 0;
    //  Enter pool
    
    sem_wait(th_info->main_sem);   

    info(BEGIN, th_info->procID, th_info->threadID);

    sem_wait(th_info->sem_count);
    th_count--;
    local_count = th_count;
    sem_post(th_info->sem_count);

    if(local_count <= 4){
        if(local_count == 1){
            sem_post(th_info->sem15);
        }
        if(th_info->threadID != 15){
            sem_wait(th_info->exit_sem_2);
        }
    }

    if(th_info->threadID == 15){
        sem_wait(th_info->sem15);
        info(END, th_info->procID, th_info->threadID);

        sem_post(th_info->main_sem);
        //  Tell the others to leave
        sem_post(th_info->exit_sem_2);
        sem_post(th_info->exit_sem_2);
        sem_post(th_info->exit_sem_2);
        sem_post(th_info->exit_sem_2);
        sem_post(th_info->exit_sem_2);

        return NULL;
    }

    info(END, th_info->procID, th_info->threadID);

    sem_post(th_info->main_sem);
    return NULL;
}


typedef struct _proc3{
    int threadID;
    int procID;
    sem_t *sem_34_74;
    sem_t *sem_36_74;
}proc3;

void* process3_function(void *arg){
    proc3* th_info = (proc3*)arg;
    
    if(th_info->threadID == 4){
        info(BEGIN, th_info->procID, th_info->threadID);
        info(END, th_info->procID, th_info->threadID);
        sem_post(th_info->sem_34_74);       //  Tell thread 7-4 to start
    }else if(th_info->threadID == 6){
        sem_wait(th_info->sem_36_74);       //  Wait for thread 7-4 to finish
        info(BEGIN, th_info->procID, th_info->threadID);
        info(END, th_info->procID, th_info->threadID);
    }else {
        //  All the other threads
        info(BEGIN, th_info->procID, th_info->threadID);
        info(END, th_info->procID, th_info->threadID);
    }
    
    return NULL;
}

int main() {
    init();
    //  Inter process semaphores
    sem_t *sem_34_74 = NULL;
    sem_t *sem_36_74 = NULL;
    sem_34_74 = sem_open("/a2_sem_34_74", O_CREAT, 0644, 0);
    if(sem_34_74 == SEM_FAILED){
        perror("Couldn't open semaphore 34_74\n");
        return 1;
    }
    sem_36_74 = sem_open("/a2_sem_36_74", O_CREAT, 0644, 0);
    if(sem_36_74 == SEM_FAILED){
        perror("Couldn't open semaphore 36_74\n");
        return 1;
    }

    info(BEGIN, 1, 0);
    pid_t pid[8];   //  index = process nr

    pid[1] = getpid();  //  Parent id
    
    //  Create P2
    pid[2] = fork();
    if (pid[2] == 0) {
        //  Inside P2
        info(BEGIN, 2, 0);
        
        //  Create P4
        pid[4] = fork();
        if (pid[4] == 0) {
            //  Inside P4
            info(BEGIN, 4, 0);
            
            //  Create P6
            pid[6] = fork();
            if (pid[6] == 0) {
                //  Inside P6
                info(BEGIN, 6, 0);
                info(END, 6, 0);
                exit(0);
            }
            waitpid(pid[6], NULL, 0);  
            info(END, 4, 0);
            exit(0);
        }
        
        //  Create P5
        pid[5] = fork();
        if (pid[5] == 0) {
            //  Inside P5
            info(BEGIN, 5, 0);
            pthread_t tid[48];
            // for(int i = 0; i < 48; i++){
            //     tid[i] = -1;
            // }
            proc5 th_info[48];
            sem_t main_sem;
            sem_t sem15;
            sem_t sem_count;
            sem_t exit_sem_2;
            if(sem_init(&main_sem, 0, 5) != 0){
                perror("Could not init main sem.\n");
                return -1;
            }
            if(sem_init(&exit_sem_2, 0, 0) != 0){
                perror("Could not init EXIT sem 2.\n");
                return -1;
            }
            if(sem_init(&sem15, 0, 0) != 0){
                perror("Could not init sem 15.\n");
                return -1;
            }
            if(sem_init(&sem_count, 0, 1) != 0){
                perror("Could not init sem count.\n");
                return -1;
            }

            for(int i = 0; i < 48; i++){
                th_info[i].threadID = i + 1;
                th_info[i].procID = 5;
                th_info[i].main_sem = &main_sem;
                th_info[i].sem15 = &sem15;
                th_info[i].sem_count = &sem_count;
                th_info[i].exit_sem_2 = &exit_sem_2;
                if(pthread_create(&tid[i], NULL, process5_function, &th_info[i]) != 0){
                    perror("Error creating thread");
                    return 1;
                }
            }
            for(int i = 0; i < 48; i++){
                pthread_join(tid[i], NULL);
            }
            sem_destroy(&main_sem);
            sem_destroy(&sem15);
            sem_destroy(&sem_count);
            info(END, 5, 0);
            exit(0);
        }
        
        
        waitpid(pid[4], NULL, 0);
        waitpid(pid[5], NULL, 0);
        info(END, 2, 0);
        exit(0);
    }
    
    //  Create P3
    pid[3] = fork();
    if (pid[3] == 0) {
        //  Inside P3
        info(BEGIN, 3, 0);
        
        //  Create P7
        pid[7] = fork();
        if (pid[7] == 0) {
            //  Inside P7
            info(BEGIN, 7, 0);
            pthread_t tid[4];   //  Thread id array (excl main thread)
            //  Create threads
            proc7 th_info[4];
            sem_t sem_71;
            sem_t sem_73;
            if(sem_init(&sem_71, 0, 0) != 0){
                perror("Could not init semaphore.\n");
                return -1;
            }
            if(sem_init(&sem_73, 0, 0) != 0){
                perror("Could not init semaphore.\n");
                return -1;
            }
            
            for(int i = 0; i < 4; i++){
                th_info[i].procID = 7;
                th_info[i].threadID = i + 1;
                th_info[i].sem_71 = &sem_71;
                th_info[i].sem_73 = &sem_73;
                th_info[i].sem_34_74 = sem_34_74;
                th_info[i].sem_36_74 = sem_36_74;

                if(pthread_create(&tid[i], NULL, process7_function, &th_info[i]) != 0){
                    perror("Error creating thread");
                    return 1;
                }
            }
            //  Wait for all threads to finish
            for(int i = 0; i < 4; i++){
                pthread_join(tid[i], NULL);
            }
            sem_destroy(&sem_71);
            sem_destroy(&sem_73);
            info(END, 7, 0);
            exit(0);
        }else{
            //  Inside P3
            pthread_t tid[6];
            // for(int i = 0; i < 6; i++){
            //     tid[i] = -1;
            // }
            proc3 th_info[6];
            for(int i = 0; i < 6; i++){
                th_info[i].threadID = i + 1;
                th_info[i].procID = 3;
                th_info[i].sem_34_74 = sem_34_74;
                th_info[i].sem_36_74 = sem_36_74;
                if(pthread_create(&tid[i], NULL, process3_function, &th_info[i]) != 0){
                    perror("Error creating thread");
                    return 1;
                }
            }
            for(int i = 0; i < 6; i++){
                pthread_join(tid[i], NULL);
            }
        }
        waitpid(pid[7], NULL, 0);  
        info(END, 3, 0);
        exit(0);
    }
    
    
    waitpid(pid[2], NULL, 0);
    waitpid(pid[3], NULL, 0);
    sem_unlink("/a2_sem_34_74");
    sem_unlink("/a2_sem_36_74");

    info(END, 1, 0);
    return 0;
}