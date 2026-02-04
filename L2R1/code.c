#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#define MIN_SIZE 10240    // 10 KB
#define MAX_SIZE 102400   // 100 KB
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void msleep(long ms) { struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); }


int main(int argc, char** argv){
    pid_t pIds[20];
    int opt;
    int count = 0;
    srand(time(NULL));
    int seed = rand();
    while ((opt = getopt(argc, argv, "0123456789")) != -1) {
        if(opt >= 48 && opt <= 57){
            int val = (int)opt - 48;
            pid_t pid;
            if ((pid = fork()) < 0) ERR("fork");
            if(pid==0){
                int n = val;
                unsigned int pSeed = count + seed;
                int s = (rand_r(&pSeed) % (MAX_SIZE - MIN_SIZE + 1)) + MIN_SIZE;
                printf("%i %i\n",n,s);

                char str[1024];
                sprintf(str,"%i.txt",getpid());
                //int file_d = open(str, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                //if(file_d == -1) ERR("BLAD OTWARCIA");

                sigset_t mask;
                sigemptyset(&mask);
                sigaddset(&mask, SIGUSR1);
                sigaddset(&mask, SIGALRM);
                sigprocmask(SIG_BLOCK, &mask,NULL);
                alarm(1);
                int signo;
                int received = 0;
                while(1){
                    if (sigwait(&mask, &signo))
                        ERR("sigwait failed.");
                    switch (signo)
                    {
                    case SIGUSR1:
                        received += 1;
                        printf("Proces: %i - %i\n",getpid(),received);
                        break;
                    case SIGALRM:
                        //close(file_d);
                        printf("Proces: %i - zamyka sie\n",getpid());
                        return 0;
                        break;
                    default:
                        break;
                    }

                }
                return 0;
            }
            else{
                pIds[count] = pid;
            }
        }
        else{
            ERR("Złe argumenty");
        }
        count++;
    }
    msleep(100);
    for(int i=0;i<100;i++){
        for(int p=0;p<count;p++){
            kill(pIds[p],SIGUSR1);
        }
        msleep(10);
    }
    for(int i=0;i<count;i++){
        int status;
        waitpid(pIds[i],&status, 0);
    }
    printf("KONIEC\n");
}