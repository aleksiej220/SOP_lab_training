#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void writenread(FILE * WRITER, FILE * READER){
    char str[100] = "WITAJ";
    int size = (int)strlen(str);
    if(fwrite(&size, sizeof(size), 1, WRITER) != 1) ERR("FWRITE");
    if(fwrite(str, sizeof(char), (size_t)size, WRITER) != (size_t)size) ERR("FWRITE");
    if(fflush(WRITER) == EOF) ERR("FFLUSH");
    printf("SUCCESS1\n");
    if(fread(&size, sizeof(size), 1, READER) != 1) ERR("FREAD");
    if(fread(str, sizeof(char), (size_t)size, READER) != (size_t)size) ERR("FREAD");
    str[size] = '\0';
    printf("SUCCESS2\n");
}

int main(){
    int zeroone[2];
    int onetwo[2];
    int twozero[2];

    printf("PARENT 0\n");
    int status;
    status = pipe(zeroone);
    if(status)ERR("PIPE");
    printf("Creates r - %d d - %d [0->1]\n",zeroone[0],zeroone[1]);
    status = pipe(onetwo);
    if(status)ERR("PIPE");
    printf("Creates r - %d d - %d [1->2]\n",onetwo[0],onetwo[1]);
    status = pipe(twozero);
    if(status)ERR("PIPE");
    printf("Creates r - %d d - %d [2->0]\n",twozero[0],twozero[1]);
    pid_t pid1 = fork();
    if(pid1 == -1){
        ERR("FORK");
    }
    if(pid1 == 0){
        printf("DZIECKO 1\n Zamyka: %d %d %d %d\n",zeroone[0],onetwo[1],twozero[0],twozero[1]);
        //Dziecko 1
        close(zeroone[1]);
        close(onetwo[0]);
        close(twozero[0]);
        close(twozero[1]);
        FILE * stream01 = fdopen(zeroone[0],"r");
        if(stream01 == NULL) ERR("FDOPEN");
        FILE * stream12 = fdopen(onetwo[1],"w");
        if(stream12 == NULL) ERR("FDOPEN");
        if(setvbuf(stream12, NULL, _IONBF, 0)) ERR("SETVBUF");

        writenread(stream12,stream01);

        fclose(stream01);
        fclose(stream12);
        _exit(0);
    }
    pid_t pid2 = fork();
    if(pid2 == -1){
        ERR("FORK");
    }
    if(pid2 == 0){
        //Dziecko 2
        printf("DZIECKO 2\n Zamyka: %d %d %d %d\n",onetwo[1],twozero[0],zeroone[0],zeroone[1]);
        close(onetwo[1]);
        close(twozero[0]);
        close(zeroone[0]);
        close(zeroone[1]);
        FILE * stream12 = fdopen(onetwo[0],"r");
        if(stream12 == NULL) ERR("FDOPEN");
        FILE * stream20 = fdopen(twozero[1],"w");
        if(stream20 == NULL) ERR("FDOPEN");
        if(setvbuf(stream20, NULL, _IONBF, 0)) ERR("SETVBUF");

        writenread(stream20,stream12);

        fclose(stream12);
        fclose(stream20);
        _exit(0);
    }
    printf("RODZIC 0\n Zamyka: %d %d %d %d\n", twozero[1], zeroone[0], onetwo[0],onetwo[1]);
    close(twozero[1]);
    close(zeroone[0]);
    close(onetwo[0]);
    close(onetwo[1]);
    FILE * stream20 = fdopen(twozero[0],"r");
    if(stream20 == NULL) ERR("FDOPEN");
    FILE * stream01 = fdopen(zeroone[1],"w");
    if(stream01 == NULL) ERR("FDOPEN");
    if(setvbuf(stream01, NULL, _IONBF, 0)) ERR("SETVBUF");

    writenread(stream01,stream20);

    fclose(stream20);
    fclose(stream01);
    int wstatus;
    if(waitpid(pid1, &wstatus, 0) == -1) ERR("WAITPID");
    if(waitpid(pid2, &wstatus, 0) == -1) ERR("WAITPID");
    return 0;
}