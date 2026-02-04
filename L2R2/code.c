#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#define STD_BUFF 1024
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void msleep(long ms) { struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); }

timer_t initTimer(int signal_val){
    struct sigevent sev; 
    timer_t timerid; 
    sev.sigev_notify = SIGEV_SIGNAL; 
    sev.sigev_signo = signal_val;
    sev.sigev_value.sival_int = 1;
    if(timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
        ERR("timer_create");
    return timerid;
}

void setTimer(timer_t timerid, int ms){
    struct itimerspec its; 
    its.it_value.tv_sec = ms / 1000;
    its.it_value.tv_nsec = (ms % 1000) * 1000000L; 
    its.it_interval.tv_sec = 0; 
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
}
int main(int argc, char** argv){
    int opt;
    int t,k,n,p;
    pid_t ids[30];
    while ((opt = getopt(argc, argv, "t:k:n:p:")) != -1) {
        switch (opt)
        {
        case 't':
            t = atoi(optarg);
            if(t<1 || t>100) ERR("t = [1,100]");
            break;
        case 'k':
            k = atoi(optarg);
            if(k<1 || k>100) ERR("k = [1,100]");
            break;
        case 'n':
            n = atoi(optarg);
            if(n<1 || n>30) ERR("n = [1,30]");
            break;
        case 'p':
            p = atoi(optarg);
            if(p<1 || p>100) ERR("p = [1,100]");
            break;
        default:
            ERR("BAD ARGUMENT");
            break;
        }
    }
    for(int i=0;i<n;i++){
        pid_t pid = fork();
        if(pid == -1) ERR("FORK ERROR");
        if(pid == 0){
            //zachowanie dziecka
            printf("Utworzono %i\n",i);
            //Zdrowe
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR1);
            sigaddset(&mask, SIGTERM);
            sigprocmask(SIG_BLOCK, &mask,NULL);
            int signo;
            unsigned int seed = time(NULL) ^ getpid();  // unikalny seed dla każdego dziecka
            int coughs = 0;
            while(1){
                if(i==0){
                    //jedno dziecko musi być chore
                    break;
                }
                if (sigwait(&mask, &signo))
                    ERR("sigwait failed");
                if(signo == SIGUSR1){
                    //na dziecko kalsznieto
                    int diceRoll = rand_r(&seed)%100;
                    if(diceRoll > p){
                        //nie zarazono
                        printf("Dziecko %i uniknęło zarażenia\n",i);
                        continue;
                    }
                    //zarazenie
                    printf("Dziecko %i zostało zarażone!\n",i);
                    break;
                }
                else if(signo == SIGTERM){
                    exit(coughs);
                }
                else{
                    ERR("SIG?");
                }
            }

            //Zarazone

            sigemptyset(&mask);
            sigaddset(&mask, SIGALRM);
            sigaddset(&mask, SIGTERM);
            sigaddset(&mask, SIGUSR1);
            sigaddset(&mask, SIGUSR2);
            sigprocmask(SIG_BLOCK, &mask,NULL);
            alarm(k); //nastawienie odbioru
            timer_t cough_timer = initTimer(SIGUSR2);  // stwórz timer raz
            setTimer(cough_timer, (rand_r(&seed)%151) + 50); //nastawiene kaszlu
            while(1){
                if (sigwait(&mask, &signo))
                    ERR("sigwait failed");
                if(signo == SIGALRM){
                    //odbior
                    printf("Dziecko %i zostało odebrane przez rodziców\n",i);
                    timer_delete(cough_timer);
                    exit(coughs);
                }
                else if(signo == SIGUSR2){
                    //pora na zarazanie
                    printf("Dziecko %i kaszle!\n",i);
                    coughs++;
                    kill(-getpgrp(),SIGUSR1);
                    //nastaw kolejne
                    setTimer(cough_timer, (rand_r(&seed)%151) + 50);
                }
                else if(signo == SIGTERM){
                    timer_delete(cough_timer);
                    exit(coughs);
                }
                else if(signo == SIGUSR1){
                    continue;
                }
                else{
                    ERR("SIG?");
                }
            }

        }
        else{
            //zapisz id
            ids[i] = pid;
        }
    }
    // glowny proces
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask,NULL);
    sleep(t);
    printf("Kończenie symulacji\n");
    kill(-getpgrp(),SIGTERM);
    for(int i=0;i<n;i++){
        int status; 
        waitpid(ids[i],&status,0);
        int coughs = WEXITSTATUS(status);
        printf("Dziecko %i kaszlnęło %i razy\n",i,coughs);
    }
    return 0;
}