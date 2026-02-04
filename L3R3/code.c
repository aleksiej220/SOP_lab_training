#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void msleep(long ms) { struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); }

typedef struct cell{
    int val;
    pthread_mutex_t mutex;
}  cell;

typedef struct dogArgs{

    int* dogsOnFinish;
    pthread_mutex_t* dogsOnFinishMutex;

    unsigned int seed;
    int index;
    int position;
    int direction;
    int trackSize;
    cell* track;
    pthread_mutex_t posMutex;
    pthread_t thread;
    int finished;
} dogArgs;

typedef struct sigArgs{
    cell* track;
    dogArgs* args;
    int n;
    int m;
    pthread_t* mainThread;
    pthread_mutex_t* DONFM;
}sigArgs;


void *signal_handling(void *voidArgs)
{
    sigArgs* a = (sigArgs*) voidArgs;
    sigset_t set; sigemptyset(&set); // start with an empty set
    sigaddset(&set, SIGINT); // add a signal
    int signo;
    srand(time(NULL));
    for (;;)
    {
        if (sigwait(&set, &signo))
            ERR("sigwait failed.");
        switch (signo)
        {
            case SIGINT:
                printf("CLEAN\n");
                for(int i=0;i<a->m;i++){
                    pthread_cancel(a->args[i].thread);
                }
                pthread_cancel(*(a->mainThread));
                pthread_mutex_destroy(a->DONFM);
                for(int i=0;i<a->n;i++){
                    pthread_mutex_destroy(&a->track[i].mutex);
                }
                free(a->track);
                for(int i=0;i<a->m;i++){
                    pthread_mutex_destroy(&a->args[i].posMutex);
                }
                free(a->args);
                exit(0);
                break;
            default:
                printf("unexpected signal %d\n", signo);
                exit(1);
        }
    }
    return NULL;
}




void* dogWork(void* a){
    dogArgs* arg = (dogArgs*)a;
    while(1){
        //spanie i skakanie
        int sleepTime = (rand_r(&arg->seed) % (1520-200)) + 200;
        msleep(sleepTime);
        int jump = (rand_r(&arg->seed) % (5-1)) + 1;
        int prev = arg->position;
        int i = (arg->position + (jump * arg->direction));
        if(i>=arg->trackSize){
            i = arg->trackSize-1;
        }
        if(i<0){
            i = 0;
        }
        if(i == arg->trackSize-1 || i == 0){
            arg->direction *= -1;
        }
        arg->position = i;
        //sczekanie
        if(arg->track[i].val>0){
            printf("waf waf waf\n");
        }
        // obsluga mety
        printf("Pies: %i biegnie na nową pozycję %i.\n",arg->index,i);
        if(!arg->finished && i == arg->trackSize-1){
            printf("Pies: %i dotarł do końca toru!\n",arg->index);
            arg->finished = 1;
            pthread_mutex_lock(arg->dogsOnFinishMutex);
            *(arg->dogsOnFinish) += 1;
            pthread_mutex_unlock(arg->dogsOnFinishMutex);
        }
        //usuniec starego
        pthread_mutex_lock(&arg->track[prev].mutex);
        arg->track[prev].val -= 1;
        pthread_mutex_unlock(&arg->track[prev].mutex);

        //dodanie nowego
        pthread_mutex_lock(&arg->track[i].mutex);
        arg->track[i].val += 1;
        pthread_mutex_unlock(&arg->track[i].mutex);
    }

    int i = rand_r(&arg->seed) % arg->trackSize;
    pthread_mutex_lock(&arg->track[i].mutex);
    arg->track[i].val += 1;
    printf("%i\n",i);
    pthread_mutex_unlock(&arg->track[i].mutex);
}
void writeTrack(cell* track, int trackSize){
    for(int i=0;i<trackSize;i++){
        pthread_mutex_lock(&track[i].mutex);
        printf("%i ",track[i].val);
        pthread_mutex_unlock(&track[i].mutex);
    }
    printf("\n");
}
int main(int argc, char** argv){

    sigset_t set; sigemptyset(&set); sigaddset(&set, SIGINT); pthread_sigmask(SIG_BLOCK, &set, NULL);

    if(argc != 3){
        return 1;
    }
    int n =  atoi(argv[1]);
    int m =  atoi(argv[2]);
    if(n <= 20 || m<=2){
        return 1;
    }

    cell* track = malloc(sizeof(cell)*n);
    for(int i= 0;i<n;i++){
        track[i].val = 0;
        if(i==0){
            track[i].val = m;
        }
        if(pthread_mutex_init(&track[i].mutex,NULL))ERR("Error in mutex creation");
    }
    dogArgs* args = malloc(sizeof(dogArgs)*m); 


    int dogsOnFinish = 0;
    pthread_mutex_t dogsOnFinishMutex;
    pthread_mutex_init(&dogsOnFinishMutex,NULL);

    srand(time(NULL));
    for(int i= 0;i<m;i++){
        args[i].seed = (unsigned int) rand();

        args[i].index = i;
        args[i].direction = 1;
        args[i].trackSize = n;
        args[i].track = track;
        args[i].finished = 0;
        args[i].position = 0;

        args[i].dogsOnFinish = &dogsOnFinish;
        args[i].dogsOnFinishMutex = &dogsOnFinishMutex;

        if(pthread_mutex_init(&args[i].posMutex,NULL)) ERR("Error in mutex creation");
        if(pthread_create(&args[i].thread,NULL,dogWork,&args[i])) ERR("Error in thread creation");
    }

    //oblsuga sigint
    pthread_t sigThread;
    pthread_t mainThread = pthread_self();
    sigArgs a;
    a.args = args;
    a.m = m;
    a.n = n;
    a.track = track;
    a.mainThread = &mainThread;
    a.DONFM = &dogsOnFinishMutex;
    pthread_create(&sigThread,NULL,signal_handling,&a);



    while (dogsOnFinish<m)
    {
        pthread_mutex_lock(&dogsOnFinishMutex);
        if(dogsOnFinish >= m){
            pthread_mutex_unlock(&dogsOnFinishMutex);
            break;
        }
        pthread_mutex_unlock(&dogsOnFinishMutex);
        writeTrack(track,n);
        msleep(1000);
    }
    
    //CLEAN UP
    for(int i=0;i<m;i++){
        pthread_cancel(args[i].thread);
    }
    pthread_mutex_destroy(&dogsOnFinishMutex);
    for(int i=0;i<n;i++){
        pthread_mutex_destroy(&track[i].mutex);
    }
    free(track);
    for(int i=0;i<m;i++){
        pthread_mutex_destroy(&args[i].posMutex);
    }
    free(args);
    return 0;
}