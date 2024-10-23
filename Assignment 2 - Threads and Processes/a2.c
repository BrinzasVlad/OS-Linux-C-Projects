#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>
#include "a2_helper.h"

//For the sake of clarity, I will leave slot 0 empty in my pthread_t arrays.
//If optimisation is direly needed, this can be changed, but will affect readability.

//P6 globals
int p6_th_2_started;
pthread_t th_p6[5];

//P7 globals
int p7_th_sems;
pthread_t th_p7[42];

//P6-8 globals
int p_68_th_sems;
pthread_t th_p8[5];

int P(int semId, int semNr)
{
    struct sembuf op = {semNr, -1, 0};

    return semop(semId, &op, 1);
}

int V(int semId, int semNr)
{
    struct sembuf op = {semNr, +1, 0};

    return semop(semId, &op, 1);
}

void* p6_th(void* arg) {
    int th_id = *( (int*)arg );

    if(3 == th_id) {
        //Thread 3 must not start before thread 2
        P(p6_th_2_started, 0);
    }
    else if(4 == th_id) {
        //T6.4 must not start before T8.1 ends
        P(p_68_th_sems, 0);
    }
    info(BEGIN, 6, th_id);
    if(2 == th_id) {
        //Thread 2 should announce when it has started, for T3
        V(p6_th_2_started, 0);
    }


    if(2 == th_id) {
        //Thread 2 must not end before thread 3
        pthread_join(th_p6[3], NULL);
    }
    info(END, 6, th_id);
    if(4 == th_id) {
        //T6.4 should announce when it ends, so that T8.2 can start
        V(p_68_th_sems, 1);
    }
    return NULL;
}

void process6() {
    //Function to be called within P6, to execute it.
    info(BEGIN, 6, 0); //Announce P6 start

    p6_th_2_started = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600); //Create semaphore to synchronise T2 and T3
    semctl(p6_th_2_started, 0, SETVAL, 0); //First semaphore in set is 0 until T2 starts

    //Create child threads
    int thread_id[5];
    for(int i = 1; i <= 4; ++i) {
        thread_id[i] = i;
        pthread_create(&th_p6[i], NULL, p6_th, (void*)&thread_id[i]);
    }

    //Wait for child threads EXCEPT T3 (which was joined by T2 already), to terminate
    pthread_join(th_p6[1], NULL);
    pthread_join(th_p6[2], NULL);
    pthread_join(th_p6[4], NULL);

    //No children for P6, do not wait for processes
    info(END, 6, 0); //Announce P6 end
}

void* p7_th(void* arg) {
    int th_id = *( (int*)arg );

    struct sembuf t_enter_barrier[2] = { {2, -1, 1}, {1, -1, 1} };
    //P(p7_th_sems, 1); //See if there is room for one more running thread
    semop(p7_th_sems, t_enter_barrier, 2); //Begin only when no-one is quitting, to synchronise the info() call.
    info(BEGIN, 7, th_id); //If yes, begin.
    V(p7_th_sems, 2); //Allow others to go in/out again

    if(10 == th_id) {
        //Thread 10 can only end when there are 5 other threads running, AKA when the slots semaphor is zero.
        struct sembuf t10_check_6processes[2] = { {2, -6, 1}, {1, 0, 1} };
        struct sembuf t10_allow_closing[3] = { {0, +5, 1}, {1, +1, 1}, {2, +6, 1} };

        semop(p7_th_sems, t10_check_6processes, 2); //Wait until 6 total threads are running and we can close synchronously
        info(END, 7, 10); //End T10 - take care, this must be carefully synchronised!
        semop(p7_th_sems, t10_allow_closing, 3); //Release the other threads so they can finish only *after* T10 end; also quit thread pool
        //V(p7_th_sems, 1); //Quit the 6-thread pool, thus freeing a slot
    }
    else {
        //Other threads may end freely, unless there are only 5 of them left, case in which they must wait for T10 to end first
        struct sembuf t_other_check_canexit[2] = { {2, -1, 1}, {0, -1, 1} };
        struct sembuf t_other_exit[2] = { {1, +1, 1}, {2, +1, 1} };

        semop(p7_th_sems, t_other_check_canexit, 2); //Make sure we can exit safely and don't need to wait
        info(END, 7, th_id); //End the thread
        semop(p7_th_sems, t_other_exit, 2); //Allow others to exit now and quit the 6-thread pool
    }

    return NULL;
}

void process7() {
    info(BEGIN, 7, 0); //Announce P7 start

    p7_th_sems = semget(IPC_PRIVATE, 3, IPC_CREAT | 0600); //Semaphores must be in the same group, so we can do semop on them at once
    //Semaphore 0 to count number of threads we can still end without locking T10 forever
    //Semaphore 1 to count the number of threads that still have room to run
    //Semaphore 2 to make sure only ONE thread is exiting at a time, thus guarantee synchronisation on info()
    semctl(p7_th_sems, 0, SETVAL, 35); //We start with 35 threads left, since of 41 threads we must keep 6 open until T10 ends
    semctl(p7_th_sems, 1, SETVAL, 6); //We start with 6 empty slots
    semctl(p7_th_sems, 2, SETVAL, 6); //Only one thread can end at a time, plus no-one should enter *while T10 is exiting*
    //To explain the trick: this third semaphore acts as a lock for synchronising the info() function
    //Normal threads modify it by 1 at a time - up to 6 of them can enter/exit at once.
    //T10, however, modifies it by 6; this means it always has full priority when entering/exiting.
    //This way, once it's checked that there are exactly 6 threads running and begins to close, all other P7 threads "freeze".
    //However, outside of that case, everything else keeps moving as normal, more or less.
    //Notably, other threads can still start 6 at a time.

    //Create child threads
    int thread_id[42];
    for(int i = 1; i <= 41; ++i) {
        thread_id[i] = i;
        pthread_create(&th_p7[i], NULL, p7_th, (void*)&thread_id[i]);
    }

    //Wait for child threads to terminate
    for(int i = 1; i <= 41; ++i) {
        pthread_join(th_p7[i], NULL);
    }

    //No children for P7, do not wait for processes
    info(END, 7, 0); //Announce P7 end
}

void* p8_th(void* arg) {
    int th_id = *( (int*)arg );

    if(2 == th_id) {
        //T8.2 must not start until T6.4 ends
        P(p_68_th_sems, 1);
    }
    info(BEGIN, 8, th_id);


    info(END, 8, th_id);
    if(1 == th_id) {
        //T8.1 should signal T6.4 when ending, so T6.4 can start
        V(p_68_th_sems, 0);
    }
    return NULL;
}

void process8() {
    info(BEGIN, 8, 0); //Announce P8 start

    //Create child threads
    int thread_id[5];
    for(int i = 1; i <= 4; ++i) {
        thread_id[i] = i;
        pthread_create(&th_p8[i], NULL, p8_th, (void*)&thread_id[i]);
    }

    //Wait for child threads to terminate
    for(int i = 1; i <= 4; ++i) {
        pthread_join(th_p8[i], NULL);
    }

    //No children for P8, do not wait for processes
    info(END, 8, 0); //Announce P8 end
}

int main(){
    init(); //Call init() at the start
    info(BEGIN, 1, 0); //Announce P1 start

    //Initialise interprocess semaphores here, before the processes using it fork.
    p_68_th_sems = semget(IPC_PRIVATE, 2, IPC_CREAT | 0660); //Might need to allow such permissions, so that other Ps can see it
    semctl(p_68_th_sems, 0, SETVAL, 0); //Semaphore 0 is set to 1 once T8.1 ends, so that T6.4 can start
    semctl(p_68_th_sems, 1, SETVAL, 0); //Semaphore 1 is set to 1 once T6.4 ends, so that T8.2 can start

    int child_pid_p2, child_pid_p8; //pid for P2 and P8
    if ( (child_pid_p2 = fork()) ) {
        //Here we are in P1
        if ( (child_pid_p8 = fork()) ) {
            //Here we are still in P1
        }
        else {
            //Here we are in P8
            process8(); //Just run it
            exit(0); //Exit P8
        }
    }
    else {
        //Here we are in P2
        info(BEGIN, 2, 0); //Announce P2 start

        int child_pid_p3, child_pid_p4, child_pid_p6, child_pid_p7;
        if ( (child_pid_p3 = fork()) ) {
            //Here we are still in P2
            if ( (child_pid_p4 = fork()) ) {
                //Here we are still in P2

                if ( (child_pid_p6 = fork()) ) {
                    //Here we are still in P2
                    if ( (child_pid_p7 = fork()) ) {
                        //Here we are still, still in P2
                    }
                    else {
                        //Here we are in P7
                        process7(); //Just run it
                        exit(0); //Exit P7
                    }
                }
                else {
                    //Here we are in P6
                    process6(); //Just run it
                    exit(0); //Exit P6
                }
            }
            else {
                //Here we are in P4
                info(BEGIN, 4, 0); //Announce P4 start

                int child_pid_p5;
                if ( (child_pid_p5 = fork()) )  {
                    //Here we are still in P4
                }
                else {
                    //Here we are in P5
                    info(BEGIN, 5, 0); //Announce P5 start

                    //No children for P5, do not wait for processes
                    info(END, 5, 0); //Announce P5 end
                    exit(0); //Exit P5
                }

                //Wait for child processes of P4
                waitpid(child_pid_p5, NULL, 0);
                info(END, 4, 0); //Announce P4 end
                exit(0); //Exit P4
            }
        }
        else {
            //Here we are in P3
            info(BEGIN, 3, 0); //Announce P3 start

            //No children for P3, do not wait for processes
            info(END, 3, 0); //Announce P3 end
            exit(0); //Exit P3
        }

        //Wait for child processes of P2
        waitpid(child_pid_p3, NULL, 0);
        waitpid(child_pid_p4, NULL, 0);
        waitpid(child_pid_p6, NULL, 0);
        waitpid(child_pid_p7, NULL, 0);
        info(END, 2, 0); //Announce P2 end
        exit(0); //Exit P2
    }


    //Wait for child processes of P1
    waitpid(child_pid_p2, NULL, 0);
    waitpid(child_pid_p8, NULL, 0);
    info(END, 1, 0); //Announce P1 end
    return 0; //Exit P1
}
