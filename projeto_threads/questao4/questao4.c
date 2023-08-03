#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define _XOPEN_SOURCE_ 600
#define N 2
#define P 10
#define ROWS 2
#define COLS 2


int matA[ROWS][COLS]={ { 2, 1 },
                       { 5, 7 } };
double matX[COLS]={1.0,1.0};

int matB[COLS] = {11 , 13};

// vai guarar os index's que cada thread deve percorrer
int begin[N], end[N];
int exc[N] = {0}, indexi[N], k[N]={0}, indexi2[N];
            

int buffer;
pthread_t *threads;

pthread_barrier_t barrier;
pthread_barrier_t barrier2;
pthread_barrier_t barrier3;
pthread_mutex_t gen_mutex = PTHREAD_MUTEX_INITIALIZER;
int tota_subs = 0,ind=0,ans =0;

int i = 0;

//calcula o somátório da fórmula da questão
double calcsum(int i){
    double sum=0;
    for(int j=0;j<COLS;j++){
        if(i!=j) {
            sum += ((double)matA[i][j])*matX[j];
            //printf("%lf += %d*%lf | i=%d j=%d\n", sum, matA[i][j], matX[j], i, j);
        }
    }
    return sum;
}

// funcao de resolver pelo metodo de jacobi
void *solvejaco(void *threadid) {
    int id = *((int *)threadid);
    printf("inicializando thread %d\n", id);
    
    while(k[id] < P) {
        
        printf("begin[id]=%d | end[id]=%d\n", begin[id], end[id]);
        
        for(indexi[id] = begin[id]; indexi[id] <=end[id]; indexi[id]++){
            //aplicando a formula dada na questao
            double aii = (1/(double) matA[indexi[id]][indexi[id]]);
            double bi =  (double) matB[indexi[id]];
            double sum = calcsum(indexi[id]);
            //barreira para esperar as outras variaveis atualizarem
            pthread_barrier_wait(&barrier2);
            matX[indexi[id]] = aii * (bi - sum);
            printf("id=%d | index[id]=%d | aii=%lf | bi=%lf | sum=%lf | X%d=%lf\n", id, indexi[id],  aii, bi, sum, indexi[id], matX[indexi[id]]);
        }
        
        //incrementa o que excedeu;
        if(exc[id]!=0){
            printf("Entrou exc\n");
            //aplicando a formula dada na questao
            double aii = (1/(double) matA[exc[id]][exc[id]]);
            double bi =  (double) matB[exc[id]];
            double sum = (double) calcsum(exc[id]);
            //barreira para esperar as outras variaveis atualizarem
            pthread_barrier_wait(&barrier3);
            matX[exc[id]] = aii * (bi - sum);
            printf("id=%d | index[id]=%d | aii=%lf | bi=%lf | sum=%lf | X%d=%lf\n", id, indexi[id],  aii, bi, sum, indexi[id], matX[indexi[id]]);

        }
        k[id] = k[id] + 1;
        pthread_barrier_wait(&barrier);
    }
   

    pthread_exit(NULL);
}


int main() {
    
    
    //aloca a quantidade de threads requerida na memória
    threads = (pthread_t *)malloc(N*sizeof(pthread_t));
    
    //guardas os ids das threads criadas
    int *threads_id[N];
    
    //variável para armazenamento do retorno da thread_create
    
    int rc;

   	//criando barreiras
    pthread_barrier_init(&barrier,NULL,N);
    pthread_barrier_init(&barrier2,NULL,N);
    pthread_barrier_init(&barrier3,NULL,N);

    int tam = ROWS/N, cont = 0;
    //aqui estamos aplicando o numero minimo de variaveis que cada thread vai ser responsável
    for(int i=0; i < N ;i++){
        begin[i] = i*tam;
        end[i] = begin[i] + tam-1;
    }
    // como a divisão entre threads e variaveis pode nao ser exata, essa parte servira para calcular as vzriaveis "excedentes"
    int exceeded = ROWS%N;
    for(int i = 0;i < exceeded; i++){
        exc[i] = tam*N+i+1;
    }


    //criando ids e threads
    for(int i = 0; i < N; i++){
        threads_id[i] = (int *)malloc(sizeof(int));
        *(threads_id[i]) = i;
        rc = pthread_create(&threads[i], NULL, solvejaco, (void *)threads_id[i]);

        if(rc){
            printf("Deu problema na criacao da thread %d\n", rc);
            exit(-1);
        }

    }
    // liberando espaço da memoria
    for(int i = 0; i < N; i++) 
        pthread_join(threads[i], NULL);
        

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&gen_mutex);

    for(int i =0; i < N; i++) 
        free(threads_id[i]);    
    
    pthread_exit(NULL);

    free(threads);

    return 0;
    
    }
