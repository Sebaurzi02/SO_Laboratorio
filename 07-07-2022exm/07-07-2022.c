#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFFER_SIZE 32

enum SEM_TYPE {S_MNG,S_ADD,S_SUB,S_MUL};

typedef struct{
    long s;
    long o;
    char done;
}shm_query;

int WAIT(int sem_id, int sem_num) {
  struct sembuf ops[1] = {{sem_num, -1, 0}};
  return semop(sem_id, ops, 1);
}

int SIGNAL(int sem_id, int sem_num) {
  struct sembuf ops[1] = {{sem_num, +1, 0}};
  return semop(sem_id, ops, 1);
}

int init_shm(){ // inizializzazione e creazione area memoria condivisa
    int shm_des;
    if((shm_des = shmget(IPC_PRIVATE,sizeof(shm_query),IPC_CREAT | 0600)) == -1){
        perror("shmget");
        exit(1);
    }
    return shm_des;
}

int init_sem(){ // iniziallizzazione semafori tutti a 0 tranne MNG
    int sem_des;

    if((sem_des = semget(IPC_PRIVATE,4,IPC_CREAT | 0600)) == -1){
        perror("semget");
        exit(1);
    }

    if(semctl(sem_des,S_MNG,SETVAL,1) == -1){
        perror("semctl");
        exit(1);
    }

    if(semctl(sem_des,S_ADD,SETVAL,0) == -1){
        perror("semctl");
        exit(1);
    }

    if(semctl(sem_des,S_SUB,SETVAL,0) == -1){
        perror("semctl");
        exit(1);
    }

    if(semctl(sem_des,S_MUL,SETVAL,0) == -1){
        perror("semctl");
        exit(1);
    }

    return sem_des;
}

void MNG(int shm_des,int sem_des,char *path){
    FILE *f;
    shm_query *m;
    char buffer[BUFFER_SIZE];

    if((m = (shm_query *)shmat(shm_des,NULL,0)) == (shm_query *)-1){
        perror("shmat");
        exit(1);
    }
    WAIT(sem_des,S_MNG);
    m->s = 0;
    SIGNAL(sem_des,S_MNG);

    if((f = fopen(path,"r")) == NULL){
        perror("fopen");
        exit(1);
    }

    while(fgets(buffer,BUFFER_SIZE,f)){
        WAIT(sem_des,S_MNG);
        m->o = atol(&buffer[1]);
        m->done = 0;

        if(buffer[0] == '+'){
            SIGNAL(sem_des,S_ADD);
        }

        if(buffer[0] == '-'){
            SIGNAL(sem_des,S_SUB);
        }

        if(buffer[0] == '*'){
            SIGNAL(sem_des,S_MUL);
        }
    }
    while(m->done == 0){
        switch(buffer[0]){
            case '+':
                    WAIT(sem_des,S_MNG);
                    m->done = 1;
                    SIGNAL(sem_des,S_ADD);
                    break;
            
            case '-':
                    WAIT(sem_des,S_MNG);
                    m->done = 1;
                    SIGNAL(sem_des,S_SUB);
                    break;
            
            case '*':
                    WAIT(sem_des,S_MNG);
                    m->done = 1;
                    SIGNAL(sem_des,S_MUL);
                    break;
        }
    }

    fclose(f);
    exit(0);


}

void ADD(int shm_des,int sem_des){
    shm_query *m;
    if((m = (shm_query *)shmat(shm_des,NULL,0)) == (shm_query *)-1){
        perror("shmat");
        exit(1);
    }

    while(1){
         WAIT(sem_des,S_ADD);
        if(m->done == 1){
            printf("TOT : %ld",m->s);
            break;
        }
        //printf("Ricevuto ADD: %ld\n",m->o);
        printf("%ld + %ld\n",m->s,m->o);
        m->s = m->s + m->o;
        SIGNAL(sem_des,S_MNG);
    }

    exit(0);
}

void SUB(int shm_des,int sem_des){
    shm_query *m;
    if((m = (shm_query *)shmat(shm_des,NULL,0)) == (shm_query *)-1){
        perror("shmat");
        exit(1);
    }

    while(1){
         WAIT(sem_des,S_SUB);
        if(m->done == 1){
            printf("TOT : %ld",m->s);
            break;
        }
        //printf("Ricevuto SUB : %ld\n",m->o);
        printf("%ld - %ld\n",m->s,m->o);
        m->s = m->s - m->o;
        SIGNAL(sem_des,S_MNG);
    }
    exit(0);
    
}

void MUL(int shm_des,int sem_des){
    shm_query *m;
    if((m = (shm_query *)shmat(shm_des,NULL,0)) == (shm_query *)-1){
        perror("shmat");
        exit(1);
    }

    while(1){
         WAIT(sem_des,S_MUL);
        if(m->done == 1){
            printf("TOT : %ld",m->s);
            break;
        }
        //printf("Ricevuto MUL : %ld\n",m->o);
        printf("%ld * %ld\n",m->s,m->o);
        m->s = m->s * m->o;
        SIGNAL(sem_des,S_MNG);
    }

    exit(0);
}

int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr,"Usage : %s calculator.c <list.txt>\n",argv[0]);
        exit(1);
    }

    int shm_des = init_shm();
    int sem_des = init_sem();

    if(!fork()){
        MNG(shm_des,sem_des,argv[2]);
    }

    if(!fork()){
        ADD(shm_des,sem_des);
    }

    
    if(!fork()){
        SUB(shm_des,sem_des);
    }

    
    if(!fork()){
        MUL(shm_des,sem_des);
    }

    for(int i = 0 ; i<4 ; i++){
        wait(NULL);
    }

    shmctl(shm_des,IPC_RMID,NULL);
    semctl(sem_des,0,IPC_RMID);
}