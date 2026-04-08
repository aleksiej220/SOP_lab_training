#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

// Values of this function are in range (0,1]
double func(double x)
{
    return exp(-x * x);
}

/**
 * It counts hit points by Monte Carlo method.
 * Use it to process one batch of computation.
 * @param N Number of points to randomize
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return Number of points which was hit.
 */
int randomize_points(int N, float a, float b)
{
    int result = 0;
    for (int i = 0; i < N; ++i)
    {
        double rand_x = ((double)rand() / RAND_MAX) * (b - a) + a;
        double rand_y = ((double)rand() / RAND_MAX);
        double real_y = func(rand_x);

        if (rand_y <= real_y)
            result++;
    }
    return result;
}

/**
 * This function calculates approximation of integral from counters of hit and total points.
 * @param total_randomized_points Number of total randomized points.
 * @param hit_points Number of hit points.
 * @param a Lower bound of integration
 * @param b Upper bound of integration
 * @return The approximation of integral
 */
double summarize_calculations(uint64_t total_randomized_points, uint64_t hit_points, float a, float b)
{
    return (b - a) * ((double)hit_points / (double)total_randomized_points);
}

/**
 * This function locks mutex and can sometime die (it has 2% chance to die).
 * It cannot die if lock would return an error.
 * It doesn't handle any errors. It's users responsibility.
 * Use it only in STAGE 4.
 *
 * @param mtx Mutex to lock
 * @return Value returned from pthread_mutex_lock.
 */
int random_death_lock(pthread_mutex_t* mtx)
{
    int ret = pthread_mutex_lock(mtx);
    if (ret)
        return ret;

    // 2% chance to die
    if (rand() % 50 == 0)
        abort();
    return ret;
}

void usage(char* argv[])
{
    printf("%s a b N - calculating integral with multiple processes\n", argv[0]);
    printf("a - Start of segment for integral (default: -1)\n");
    printf("b - End of segment for integral (default: 1)\n");
    printf("N - Size of batch to calculate before reporting to shared memory (default: 1000)\n");
}
typedef struct {
    pthread_mutex_t mutex;
    int process_count;
    int initialized;
} SharedData;
int robust_mutex_lock(pthread_mutex_t *mutex)
{
    int ret = pthread_mutex_lock(mutex);

    if (ret == EOWNERDEAD) {
        printf("Poprzedni właściciel mutexa umarł!\n");

        // napraw dane współdzielone jeśli trzeba
        //repair_shared_state();

        // oznacz mutex jako ponownie spójny - BARDZO WAŻNE
        pthread_mutex_consistent(mutex);
    }
    else if (ret != 0) {
        perror("pthread_mutex_lock");
        exit(1);
    }
}
int main(int argc, char* argv[])
{
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int N = atoi(argv[3]);
    usage(argv);
    for(int i=0;i<10;i++){
        pid_t pid = fork();
        if(pid == -1){
            ERR("FORK");
        }
        if(pid == 0){
            //child
            sem_t* sem = sem_open("/init_sem", O_CREAT, 0666, 1);
            sem_wait(sem);
            int created = 0;
            int fd = shm_open("/shared_data", O_RDWR | O_CREAT | O_EXCL, 0666);
            if (fd >= 0) {
                created = 1;                 // świeżo utworzony
            } else if (errno == EEXIST) {
                fd = shm_open("/shared_data", O_RDWR, 0666);  // już istniał
                if (fd < 0) ERR("shm_open existing");
            } else {
                ERR("shm_open");
            }
            if(created){
                ftruncate(fd, sizeof(SharedData));
            }
            SharedData *shared = mmap(NULL, sizeof(SharedData),
                PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
            if(created){
                memset(shared,0,sizeof(*shared));
                pthread_mutexattr_t attr;
                pthread_mutexattr_init(&attr);
                pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
                pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
                pthread_mutex_init(&(shared->mutex), &attr);
                shared->process_count = 1;
                shared->initialized = 1;
            }
            else{
                shared->process_count++;
            }
            sem_post(sem);
            //Proces odpowiednio utworzony i dodany
            printf("Processes %i\n",shared->process_count);
            sleep(2);
            sem_wait(sem);
            if(shared->process_count == 1){
                //ostatni - sprzata
                shm_unlink("/shared_data");
            }
            else{
                shared->process_count--;
            }
            printf("Cleaned. Processes %i\n",shared->process_count);
            sem_post(sem);
            exit(0);
        }
        else{
            //parent
        }
    }
    while (1)
    {
        /* code */
    }
    
}