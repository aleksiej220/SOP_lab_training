#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#define STD_BUFF 1024
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
int main(int argc, char** argv){
    int ins = 0;
    int isOutput = 0;
    char* inputs[20];
    char* output;
    int opt;
    while ((opt = getopt(argc, argv, "p:o:")) != -1) {
        switch (opt)
        {
        case 'p':
            inputs[ins] = (char*) malloc(sizeof(char) * strlen(optarg)+1);
            strcpy(inputs[ins],optarg);
            ins++;
            break;
        case 'o':
            if(isOutput==0){
                isOutput = 1;
                output = (char*) malloc(sizeof(char)*(strlen(optarg)+1));
                strcpy(output,optarg);
            }
            break;
        default:
            ERR("Zly argument");
            break;
        }
    }

    FILE * stream;

    stream = fopen(output, "w");
    if(stream == NULL) ERR("Blad otwarcie out");

    for(int i=0;i<ins;i++){
        DIR* dir;   
        if((dir = opendir(inputs[i])) == NULL) ERR("opendir");
        struct dirent* d;
        struct stat filestat;
        while ((d = readdir(dir)) != NULL)
        {
            char path[STD_BUFF];
            strcpy(path,inputs[i]);
            strcat(path,"/");
            strcat(path,d->d_name);
            if(strcmp(d->d_name,".")==0 || strcmp(d->d_name,"..")==0){
                continue;
            }
            if(lstat(path,&filestat)){
                fprintf(stream,"%s %i\n",d->d_name,-1);
                continue;
            }
            fprintf(stream,"%s %li\n",d->d_name,filestat.st_size);
        } 
        free(inputs[i]);
        closedir(dir);
    }
    free(output);
    fclose(stream);
    return 0;
}