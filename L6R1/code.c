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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void msleep(long ms) { struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (ms % 1000) * 1000000L; nanosleep(&ts, NULL); }
#define BLOCKSIZE 4096
void readChars(char* addr, int pos, int size, int* result){
    if(size <=0){
        return;
    }
    int fd = open(addr, O_RDONLY);
    if(fd == -1){
        ERR("OPEN");
    }
    int offset = 0;
    while (pos>=BLOCKSIZE)
    {
        offset += BLOCKSIZE;
        pos-=BLOCKSIZE;
    }
    
    void* a = mmap(NULL,size+pos,PROT_READ, MAP_PRIVATE,fd,offset);
    if(a == MAP_FAILED){
        ERR("MAP");
    }
    char* c = (char*)a;
    c += pos;
    for(int i=0;i<size;i++){
        result[(int)*c]++;
        c++;
    }
    munmap(a,size+pos);
    close(fd);
}
int main(int argc, char** argv){
    int hashes[256];
    int N = atoi(argv[2]);
    for(int i=0;i<256;i++){
        hashes[i]= 0;
    }
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        ERR("open");
    }
    struct stat st;
    fstat(fd,&st);
    int charCount = st.st_size;
    //mmap do wyników
    void* a = mmap(NULL,256*N*sizeof(int), 
    PROT_READ | PROT_WRITE,
    MAP_SHARED | MAP_ANONYMOUS,
    -1, 0); 
    int* result = (int*)a;
    int pos = 0;
    int size = charCount/N;
    pid_t* pids = malloc(sizeof(pid_t)*N);
    for(int i=0;i<N;i++){
        if(i == N-1){
            //dociagniecie
            size = charCount - ((charCount/N)*(N-1));
        }
        pid_t pid = fork();
        if(pid == -1){
            ERR("FORK");
        }
        if(pid == 0){
            readChars(argv[1],pos,size,result+(i*256));
            exit(0);
        }
        else{
            pids[i] = pid;
        }
        pos += size;
    }
    close(fd);
    for(int i=0;i<N;i++){
        int status;
        waitpid(pids[i],&status,0);
        printf("PROCES %i:\n",pids[i]);
        for(int j=0;j<256;j++){
            int q =  result[(i*256) +j];
            hashes[j] += q;
            if(q>0){
                printf("%c : %i\n",j,q);
            }
        }
    }
    printf("SUMA:\n");
    for(int i=0;i<256;i++){
        if(hashes[i]>0){
            printf("%c : %i\n",i,hashes[i]);
        }
    }
    munmap(a,256*N*sizeof(int));
    free(pids);
    return 0;
}