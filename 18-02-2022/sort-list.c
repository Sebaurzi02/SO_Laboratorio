//SOLUZIONE BY SEBASTIANO URZI' ;D Faulter32
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFFER_SIZE 32
#define MSG_SIZE sizeof(Sorter_msg) - sizeof(long)
#define MSG_SIZE1 sizeof(Comparer_msg) - sizeof(long)
#define MSG_SIZE2 sizeof(Out_msg) - sizeof(long)

typedef struct{
    char key[BUFFER_SIZE];
}entry;

typedef struct{
    long type;
    entry s1;
    entry s2;
    char done;
}Sorter_msg;

typedef struct{
    long type;
    int cmp;
    char done;
}Comparer_msg;

typedef struct{
    long type;
    entry s;
    char done;
}Out_msg;

entry create_entry(char *data){
    entry e;
    strncpy(e.key,data,BUFFER_SIZE);
    return e;
}

unsigned number_of_lines(char* path){
    FILE *f;
    char buffer[BUFFER_SIZE];

    unsigned nlines = 0 ;

    if((f = fopen(path,"r")) == NULL){
        perror("fopen");
        exit(1);
    }

    while(fgets(buffer,BUFFER_SIZE,f)){
        nlines++;
    }


    fclose(f);
    return nlines;
}

/*
void bubblesort(entry *p, unsigned n){
    entry *temp = malloc(sizeof(entry));

    for(int i=0; i<n-1; i++){
        for(int j=n-1; j > i ; j--){

            if(strcasecmp(p[j].key,p[j-1].key) < 0){
                strncpy(temp->key,p[j].key,BUFFER_SIZE);//temp->key = p[j].key;
                strncpy(p[j].key,p[j-1].key,BUFFER_SIZE);//p[j].key = p[j-1].key;
                strncpy(p[j-1].key,temp->key,BUFFER_SIZE);//p[j-1].key = temp->key;
            }
        }
    }
    free(temp);
}
*/

void Sorter(int queue, char* path){
    FILE *f;
    char buffer[BUFFER_SIZE];
    unsigned nlines = number_of_lines(path);
    Sorter_msg m;
    Comparer_msg mc;
    Out_msg mo;
    mo.type = 3;
    mo.done = 0;
    m.type = 1;
    m.done = 0;
    entry *temp = malloc(sizeof(entry));
    entry *parole = malloc(sizeof(entry) * nlines);

    if((f = fopen(path,"r")) == NULL){
        perror("fopen");
        exit(1);
    }

    for(int i=0 ; i<nlines; i++){
        fgets(buffer,BUFFER_SIZE,f);
        parole[i] = create_entry(buffer);
    }

    for(int x = 0 ; x<nlines-1; x++){
        for(int y = nlines-1; y > x ; y--){
            strncpy(m.s1.key,parole[y].key,BUFFER_SIZE);
            strncpy(m.s2.key,parole[y-1].key,BUFFER_SIZE);
            if(msgsnd(queue,&m,MSG_SIZE,0)== -1){
                perror("msgsnd");
                exit(1);
            }
            while(1){
                if(msgrcv(queue,&mc,MSG_SIZE1,2,0) == -1){
                    perror("msgrcv");
                    exit(1);
                }

                if(mc.done == 1){
                    break;
                }

                if(mc.cmp < 0){
                    strncpy(temp->key,parole[y].key,BUFFER_SIZE);//temp->key = p[j].key;
                    strncpy(parole[y].key,parole[y-1].key,BUFFER_SIZE);//p[j].key = p[j-1].key;
                    strncpy(parole[y-1].key,temp->key,BUFFER_SIZE);//p[j-1].key = temp->key;
                }
            }
        }
    }
    m.done = 1;
    if(msgsnd(queue,&m,MSG_SIZE,0)== -1){
        perror("msgsnd");
        exit(1);
    }

    for(int w = 0 ; w<nlines ; w++){
        strncpy(mo.s.key,parole[w].key,BUFFER_SIZE);
        if(msgsnd(queue,&mo,MSG_SIZE2,0) == -1){
            perror("msgsnd");
            exit(1);
        }
    }
    mo.done = 1;
    if(msgsnd(queue,&mo,MSG_SIZE2,0) == -1){
        perror("msgsnd");
        exit(1);
    }

    free(temp);
    free(parole);
    fclose(f);
    exit(0);
}

void Comparer(int queue){
    Sorter_msg m;
    Comparer_msg mc;
    mc.type = 2;
    while(1){
        if(msgrcv(queue,&m,MSG_SIZE,1,0) == -1){
              perror("msgrcv");
              exit(1);
        }

        if(m.done == 1){
            break;
        }
    
        mc.done = 0;
        mc.cmp=strcasecmp(m.s1.key,m.s2.key);
        if(msgsnd(queue,&mc,MSG_SIZE1,0) == -1){
            perror("msgsnd");
            exit(1);
            
        }
        mc.done = 1;
        if(msgsnd(queue,&mc,MSG_SIZE1,0)== -1){
            perror("msgsnd");
            exit(1);
        }
    }

    exit(0);
}

int main(int argc, char **argv){
    if(argc != 2 ){
        fprintf(stderr, "Usage: %s <file>",argv[0]);
        exit(1);
    }

    int queue;
    Out_msg m;

    if((queue = msgget(IPC_PRIVATE,IPC_CREAT | 0600)) == -1){
        perror("msgget");
        exit(1);
    }

    if(!fork()){
        Sorter(queue,argv[1]);
    }
    
    if(!fork()){
        Comparer(queue);
    }

    while(1){
        if(msgrcv(queue,&m,MSG_SIZE2,3,0) == -1){
            perror("msgrcv");
            exit(1);
        }

        if(m.done == 1){
            break;
        }

        printf("%s",m.s.key);
    }


    for(int i=0; i<2; i++){
        wait(NULL);
    }

    msgctl(queue,IPC_RMID,NULL);
}