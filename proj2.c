#define _XOPEN_SOURCE 500
// Includes
/*********************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
/// Fork
#include <unistd.h>
/// Shared memory
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <fcntl.h>
/// Semaphore
#include <semaphore.h>

// Shared memory
/*********************/
/// Pointers to memory
int * shared_counter,
    * shared_H,
    * shared_S,
    * shared_boat,
    * shared_coast,
    * member_onboard;
/// File descriptors
int sh_counter_descriptor,
    sh_H_descriptor,
    sh_S_descriptor,
    sh_boat_descriptor,
    sh_coast_descriptor,
    member_descriptor;

// Semaphores
/*********************/
sem_t *starts, *enter_coast, *enter_boat, *captain_lock, *crew_lock, *sem666, *sem1, *sem2, *sem3;

// Declared functions
/*********************/
int init_shared();
void *create_shared_memory();
void kill_shared();
int init_sem();
void kill_sem();
void rand_wait();
int close_file();
void fork_child();
void full_queue();

/* Main */
int main(int argc, char *argv[]) {

    // Checks for arguments
    if (argc != 7) {
        fprintf(stderr, "ERROR! Invalid lenght of input arguments.\n");
        return 1;
    }
    // Checks if arguments are decimals
    for (int i = 1; i < argc; ++i) {
        for (unsigned long j = 0; j < strlen(argv[i]); ++j){
            if (!isdigit(argv[i][j])) {
                fprintf(stderr, "ERROR! Input arguments are not numbers.\n");
                return 1;
            }
        }
    }
    // Loads arguments
    int P, H, S, R, W, C; 
    int *args[] = {&P, &H, &S, &R, &W, &C};
    for (int i = 0; i < 6; ++i) sscanf(argv[i+1],"%d", args[i]);
    
    // Checks if loaded values are correct
    if ((P >= 2  && (P % 2) == 0   ) &&
        (H >= 0  &&  H      <= 2000) &&
        (S >= 0  &&  S      <= 2000) &&
        (R >= 0  &&  R      <= 2000) &&
        (W >= 20 &&  W      <= 2000) &&
        (C >= 5)) ;
    else {
        fprintf(stderr, "ERROR! Invalid values of input arguments.\n");
        return 1;
    }

    // Initialize shared memory and semaphores
    if (init_shared() == 1) return 1;
    if (init_sem() == 1) return 1;

    // Opens the "proj2.out" file for writing
    FILE *fp;
    fp = fopen("proj2.out", "w");
    if(fp == NULL) {
        fprintf(stderr, "ERROR! Could not write to the 'proj2.out' file.\n");
        return 1;
    }
    // UNbuffered!
    setbuf(fp, NULL);

    // Set coast capacity
    for (int i = 0; i < C; ++i) {
        sem_post(sem666);
    }

    // Used for MAIN waiting for sub-processes
    pid_t wait_pid;
    int status = 0;        

    // Forking into MAIN and sub-processes
    pid_t main_process = fork();
    // Child process
    if (main_process == 0) {
        
        // Forking into HACKERS and SERFS
        pid_t sub_process = fork();
        
        // SERFS - Child process
        if (sub_process == 0) {

            char * type = "SERF";
            for(int i = 1; i <= P; ++i) {

                rand_wait(S, 0);
                pid_t child = fork();

                if (child == 0) {
                    
                    fork_child(fp, type, i, R);//, W, C); TODO

                } else if (child < 0) {
                    fprintf(stderr, "ERROR! Could not create a child process from PPID %d.", getppid());
                    exit(1);
                }
            }
            waitpid(-1, NULL, 0);
            exit(0);
        // HACKERS - Parent process
        } else if (sub_process > 0) {

            char * type = "HACK";
            for(int i = 1; i <= P; ++i) {
                
                rand_wait(H, 0);
                pid_t child = fork();

                if (child == 0) {

                    fork_child(fp, type, i, R);//, W, C); TODO
                    
                } else if (child < 0) {
                    fprintf(stderr, "ERROR! Could not create a child process from PPID %d.", getppid());
                    exit(1);
                }
            }
            waitpid(-1, NULL, 0);
            exit(0);
        // Fail
        } else {
            fprintf(stderr, "ERROR! Could not create a child processes for HACKERS and SERFS from PPID %d.", getppid());
            close_file(fp);
            return 1;
        }
    // MAIN - Parent process
    } else if (main_process > 0) {
        // Waits for all child processes to end
        waitpid(-1, NULL, 0);
        while ((wait_pid = wait(&status)) > 0);
        usleep(1000*P);
    
    // Fail
    } else {
        fprintf(stderr, "ERROR! Could not create a child process for MAIN functuon and sub-processe from PPID %d.", getppid());
        close_file(fp);
        return 1;
    }

    // Closes the file
    if (!close_file(fp) == 0) return 1;
    // Success
    kill_sem();
    kill_shared();
    return 0;
}



/* Functions */
// random wait in interval
void rand_wait(int max_val, int min_val) {

    if (min_val > max_val) {
       int tmp = max_val;
       max_val = min_val;
       min_val = tmp;
    }

    time_t t;
    srand((unsigned) time(&t));
    if (max_val > 0)
        usleep((useconds_t)((rand() % (max_val + 1 - min_val) + min_val) * 1000));
}

// initialize shared memory
int init_shared() {
    kill_shared();
    // If anything fails, ELSE triggers
    if( // init sh_counter_descriptor
        (sh_counter_descriptor = shm_open("xmudry01_shared_counter", O_CREAT | O_EXCL | O_RDWR, 0644)) == -1 ||
        ftruncate(sh_counter_descriptor, sizeof(int))                                                  == -1 ||
        (shared_counter = create_shared_memory(sh_counter_descriptor))                         == MAP_FAILED ||
        
        // init sh_H_descriptor
        (sh_H_descriptor = shm_open("xmudry01_shared_H", O_CREAT | O_EXCL | O_RDWR, 0644))             == -1 ||
        ftruncate(sh_H_descriptor, sizeof(int))                                                        == -1 ||
        (shared_H = create_shared_memory(sh_H_descriptor))                                     == MAP_FAILED ||
        
        // init sh_S_descriptor
        (sh_S_descriptor = shm_open("xmudry01_shared_S", O_CREAT | O_EXCL | O_RDWR, 0644))             == -1 ||
        ftruncate(sh_S_descriptor, sizeof(int))                                                        == -1 ||
        (shared_S = create_shared_memory(sh_S_descriptor))                                     == MAP_FAILED || 
        
        // init sh_boat_descriptor
        (sh_boat_descriptor = shm_open("xmudry01_shared_boat", O_CREAT | O_EXCL | O_RDWR, 0644))       == -1 ||
        ftruncate(sh_boat_descriptor, sizeof(int))                                                     == -1 ||
        (shared_boat = create_shared_memory(sh_boat_descriptor))                               == MAP_FAILED ||
        
        // init sh_coast_descriptor
        (sh_coast_descriptor = shm_open("xmudry01_shared_coast", O_CREAT | O_EXCL | O_RDWR, 0644))     == -1 ||
        ftruncate(sh_coast_descriptor, sizeof(int))                                                    == -1 ||
        (shared_coast = create_shared_memory(sh_coast_descriptor))                             == MAP_FAILED ||

        // init member_descriptor
        (member_descriptor = shm_open("xmudry01_members", O_CREAT | O_EXCL | O_RDWR, 0644))           == -1 ||
        ftruncate(member_descriptor, sizeof(int))                                                     == -1 ||
        (member_onboard = create_shared_memory(member_descriptor))                           == MAP_FAILED
      )
        {
            fprintf(stderr, "ERROR! Initialization of shared memory failed.\n");
            kill_shared();
            return 1;
        }
    else return 0;
}

// Maps and creates shared memory
void *create_shared_memory(int file_descriptor) {
    return mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
}

// destroy shared memory
void kill_shared() {
    // Unmap memory
    munmap(shared_counter, sizeof(int));
    munmap(shared_H, sizeof(int));
    munmap(shared_S, sizeof(int));
    munmap(shared_boat, sizeof(int));
    munmap(shared_coast, sizeof(int));
    munmap(member_onboard, sizeof(int));

    // Unlink memory segment name
    shm_unlink("xmudry01_shared_counter");
    shm_unlink("xmudry01_shared_H");
    shm_unlink("xmudry01_shared_S");
    shm_unlink("xmudry01_shared_boat");
    shm_unlink("xmudry01_shared_coast");
    shm_unlink("xmudry01_members");

    // Close file descriptors
    close(sh_counter_descriptor);
    close(sh_H_descriptor);
    close(sh_S_descriptor);
    close(sh_boat_descriptor);
    close(sh_coast_descriptor);
    close(member_descriptor);
}

// initialize semaphores
int init_sem() {
    kill_sem();
    // If anything fails, ELSE triggers
    if( 
        (starts         = sem_open("xmudry01_starts", O_CREAT | O_EXCL, 0644, 1))         == SEM_FAILED ||
        (enter_coast    = sem_open("xmudry01_enter_coast", O_CREAT | O_EXCL, 0644, 1))    == SEM_FAILED ||
        (enter_boat     = sem_open("xmudry01_enter_boat", O_CREAT | O_EXCL, 0644, 0))     == SEM_FAILED ||
        (captain_lock   = sem_open("xmudry01_captain_lock", O_CREAT | O_EXCL, 0644, 0))   == SEM_FAILED ||
        (crew_lock      = sem_open("xmudry01_crew_lock", O_CREAT | O_EXCL, 0644, 0))      == SEM_FAILED ||
        (sem666         = sem_open("xmudry01_sem666", O_CREAT | O_EXCL, 0644, 0))         == SEM_FAILED ||
        (sem1           = sem_open("xmudry01_semH", O_CREAT | O_EXCL, 0644, 0))           == SEM_FAILED ||
        (sem2           = sem_open("xmudry01_semS", O_CREAT | O_EXCL, 0644, 0))           == SEM_FAILED ||
        (sem3           = sem_open("xmudry01_sem3", O_CREAT | O_EXCL, 0644, 1))           == SEM_FAILED
      )
        {
            fprintf(stderr, "ERROR! Initialization of semaphores failed.\n");
            kill_sem();
            return 1;
        }
    else return 0;
}

// destroy all semaphores
void kill_sem() {
    sem_unlink("xmudry01_starts");
    sem_unlink("xmudry01_enter_coast");
    sem_unlink("xmudry01_enter_boat");
    sem_unlink("xmudry01_captain_lock");
    sem_unlink("xmudry01_crew_lock");
    sem_unlink("xmudry01_sem666");
    sem_unlink("xmudry01_semH");
    sem_unlink("xmudry01_semS");
    sem_unlink("xmudry01_sem3");

    sem_close(starts);
    sem_close(enter_coast);
    sem_close(enter_boat);
    sem_close(captain_lock);
    sem_close(crew_lock);
    sem_close(sem666);
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);
}

// Closing files
int close_file(FILE *fp) {
    if(fclose(fp) == 0) return 0;
    else {
        fprintf(stderr, "ERROR! Could not close the 'proj2.out' file.\n");
        return 1;
    }
}

// update counter and return value, easy
int update_counter_get_val(int *counter, int value) {
    *counter = *counter + value;
    return *counter;
}
                   
// function for forked children                           // TODO leaves queue
void fork_child(FILE *fp, char *type, int order, int R) { //, int W, int C) {

    // print start message
    sem_wait(starts);
        fprintf(fp, "%d\t:  %s %d\t:  starts\n", update_counter_get_val(shared_counter, 1), type, order);
    sem_post(starts);

    sem_wait(sem666);

/* TODO: leaves queue, does not work properly, creates zombie processes

    sem_wait(sem3);
        sem_getvalue(sem666, &*member_onboard);

    if (*member_onboard > C) {
        sem_post(sem3);
        //fprintf(stderr, "%d %s %d\t:  %d\t:  %d\n", getpid(), type, order, *shared_H, *shared_S);
        full_queue(fp, type, order, W, C);
    } else
        sem_post(sem3);
*/ 
    sem_wait(enter_coast);
    
        sem_wait(starts);

            // if hacker add to shared_h, if serf add to shared_s
            if (strcmp(type, "HACK") == 0)
                update_counter_get_val(shared_H, 1);
            
            else if (strcmp(type, "SERF") == 0)
                update_counter_get_val(shared_S, 1);
            
            // print message and update shared_counter
            fprintf(fp, "%d\t:  %s %d\t:  waits\t\t\t:  %d\t:  %d\n", update_counter_get_val(shared_counter, 1), type, order, *shared_H, *shared_S);
            update_counter_get_val(shared_coast, 1);
        
        sem_post(starts);

    // Is Captain Jack? Not yet! Maybe will be.
    int captain = 0;    
        
        // 3 ways of getting into the boat
        if (*shared_H >= 4) {

            sem_wait(starts);
                update_counter_get_val(shared_H, -4);                
                update_counter_get_val(shared_coast, -4);

                captain = 1;
                
                for (int i = 0; i < 4; ++i) {
                    sem_post(sem1);
                }
                update_counter_get_val(shared_boat, 4);
            sem_post(starts);
            
        } else if (*shared_S >= 4) {
            
            sem_wait(starts);
                update_counter_get_val(shared_S, -4);
                update_counter_get_val(shared_coast, -4);

                captain = 1;
                
                for (int i = 0; i < 4; ++i) {
                    sem_post(sem2);
                }
                update_counter_get_val(shared_boat, 4);
            sem_post(starts);

        } else if (*shared_H >= 2 && *shared_S >= 2) {
            
            sem_wait(starts);
                update_counter_get_val(shared_H, -2);
                update_counter_get_val(shared_S, -2);
                update_counter_get_val(shared_coast, -4);
                
                captain = 1;

                for (int i = 0; i < 2; ++i) {
                    sem_post(sem1);
                    sem_post(sem2);
                }

                update_counter_get_val(shared_boat, 4);
            sem_post(starts);

        } else {

            /* SAME PROBLEM
            
            if (*shared_coast >= C){
                sem_wait(starts);
                    if (strcmp(type, "HACK") == 0)
                        update_counter_get_val(shared_H, -1);
                    
                    else if (strcmp(type, "SERF") == 0)
                        update_counter_get_val(shared_S, -1);

                    update_counter_get_val(shared_coast, -1);
                sem_post(starts);

                sem_post(enter_coast);
                sem_post(sem666);

                full_queue(fp, type, order, W, C);

                sem_wait(starts);
                    if (strcmp(type, "HACK") == 0)
                        update_counter_get_val(shared_H, 1);
                    
                    else if (strcmp(type, "SERF") == 0)
                        update_counter_get_val(shared_S, 1);

                    update_counter_get_val(shared_coast, 1);
                sem_post(starts);

            } else { */

                sem_post(enter_coast);
                sem_post(sem666);
            //}
        }

    // allow 4 processes to pass semaphore
    for (int i = 0; i < 4; ++i) {
        sem_post(sem666);
    }

    // allow only hack or serf
    if (strcmp(type, "HACK") == 0)
        sem_wait(sem1);
                
    else if (strcmp(type, "SERF") == 0)
        sem_wait(sem2);


    // If captain unlocks boat
    if (captain == 1) {
        sem_wait(starts);
            fprintf(fp, "%d\t:  %s %d\t:  boards\t\t\t:  %d\t:  %d\n", update_counter_get_val(shared_counter, 1), type, order, *shared_H, *shared_S);   
        sem_post(starts);
        
        for (int i = 0; i < 4; ++i) {
            sem_post(enter_boat);
        }
    }

    sem_wait(enter_boat);
    
    // Captain
    if (captain == 1) {
                
        // Captain waits <0, R>
        rand_wait(R, 0);

        // Unlock crew
        for (int i = 0; i < 3; ++i)
            sem_post(crew_lock);
    // Crew
    } else if (captain == 0) {

        // wait for captain
        sem_wait(crew_lock);
        
        // member exits
        sem_wait(starts);
            update_counter_get_val(shared_boat, -1);
            fprintf(fp, "%d\t:  %s %d\t:  member exits\t\t:  %d\t:  %d\n", update_counter_get_val(shared_counter, 1), type, order, *shared_H, *shared_S);
        sem_post(starts);
        
        // unlock captain to exit
        if (*shared_boat == 1) {
            sem_post(captain_lock);
        }
    }

    // Captain again
    if (captain == 1) {
          
        // Leaves last
        sem_wait(captain_lock);

        // captain exits
        sem_wait(starts);
            update_counter_get_val(shared_boat, -1);
            fprintf(fp, "%d\t:  %s %d\t:  captain exits\t:  %d\t:  %d\n", update_counter_get_val(shared_counter, 1), type, order, *shared_H, *shared_S);
        sem_post(starts);

        // frees coast and boat
        sem_post(enter_boat);
        sem_post(enter_coast);
    }
    
    // exit process
    exit(0);
}

// does not work... properly
void full_queue(FILE *fp, char *type, int order, int W, int C) {

    while (*member_onboard >= C) {
        sem_wait(starts);
            fprintf(fp, "%d\t:  %s %d\t:  leaves queue\t\t:  %d\t:  %d\n", update_counter_get_val(shared_counter, 1), type, order, *shared_H, *shared_S);
        sem_post(starts);
        
        rand_wait(W, 20);
        
        sem_wait(starts);
            fprintf(fp, "%d\t:  %s %d\t:  is back\n", update_counter_get_val(shared_counter, 1), type, order);
        sem_post(starts);
        
        sem_getvalue(sem666, member_onboard);
        fprintf(stderr, "DOLE %d\n", *member_onboard);
        
        if (*member_onboard < C)
            break;
    }
}