#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 10
#define VALOR 1000000

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int contador = 0, flag = 0;

void *inc (void *threadid){
    int ID = (*(int *)threadid);

    while(contador < VALOR){
        // Região crítica: contador e flag
        pthread_mutex_lock(&count_mutex);
        if(contador < VALOR){
            printf("A thread %d esta incrementando (VALOR ATUAL: %d).\n", ID, contador); 
            contador++; 
        }
        if(contador == VALOR && flag == 0){
            printf("A Thread %d foi a ultima incrementadora!\n", ID);
            flag = 1;
        }
        pthread_mutex_unlock(&count_mutex);
    }
    
    pthread_exit(NULL);
}

int main (){
    pthread_t threads[NUM_THREADS];
    int ThreadID[NUM_THREADS];
    int i, rc;

    //Inicializando os ID's
    for(i = 0; i < NUM_THREADS; i++){
        ThreadID[i] = i;
        printf("Criando Thread: %d", i);
        rc = pthread_create(&threads[i], NULL, inc, &ThreadID[i]);
        if(rc){
            printf("Fomos de Base na %da Thread\n", i);
            exit(1);
        }
    }

    for(i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }
    printf("Valor final: %d\n", contador);

    pthread_exit(NULL);
}
