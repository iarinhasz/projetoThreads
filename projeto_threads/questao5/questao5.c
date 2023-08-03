#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define P 3
#define C 3
#define B 5

typedef struct elem{
int value;
struct elem *prox;
}Elem;

typedef struct blockingQueue{
unsigned int sizeBuffer, statusBuffer;
Elem *head,*last;
}BlockingQueue;

BlockingQueue* Q;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex
pthread_cond_t empty = PTHREAD_COND_INITIALIZER; // Variável de condição que sinaliza se a fila ta vazia
pthread_cond_t fill = PTHREAD_COND_INITIALIZER; // Variável de condição que sinaliza se a fila ta cheia



BlockingQueue* newBlockingQueue(unsigned int SizeBuffer){
    Elem* newBQ = (Elem*)malloc(sizeof(Elem));
    newBQ->prox = NULL;
    //Inicializando parâmetros
    BlockingQueue* BQ = (BlockingQueue*)malloc(sizeof(BQ));
    
    BQ->sizeBuffer = SizeBuffer;
    BQ->statusBuffer = 0;
    BQ->head = newBQ;
    BQ->last = BQ->head;
    return BQ;
}

//insere um elemento no final da fila bloqueante Q, bloqueando a thread que está inserindo, caso a fila esteja cheia.
void putBlockingQueue(BlockingQueue*Q,int newValue){   
    
    //Travando o mutex
    pthread_mutex_lock(&mutex);
    Elem *aux;

    while( (Q->statusBuffer) == (Q->sizeBuffer) ){
        printf(" A fila está cheia, indo nanar!\n");
        pthread_cond_wait(&empty, &mutex); // Faz a thread dormir até que libere uma posição na fila 
    }
    //insere elemento na fila
    Elem *newElem = (Elem *)(malloc(sizeof(Elem)));
    newElem->value = newValue;
    newElem->prox = NULL;
    Q->last->prox = newElem;
    Q->last = newElem;
    //Sinaliza o aumento de um elemento na fila
    Q->statusBuffer++;
    
    if(Q->statusBuffer == 1) pthread_cond_broadcast(&fill);
    //Destrava o mutex
    pthread_mutex_unlock(&mutex);
}

// retira o primeiro elemento da fila bloqueante Q, bloqueando a thread que está retirando, caso a fila esteja vazia.
int takeBlockingQueue(BlockingQueue* Q){     
    //Travando o mutex
    pthread_mutex_lock(&mutex);
    int result;
    //checa se a fila está vazia 
    while(Q->statusBuffer == 0 ){
        printf("A fila esta vazia, indo tira um cochilo\n");
        pthread_cond_wait(&fill, &mutex); // Faz a thread dormir até que haja elementos no buffer.
    }
    //Apagando o primeiro elemento
    Elem* temp = Q->head->prox;
    Q->head->prox = temp->prox;
    result = temp->value;    
    free(temp);
    Q->statusBuffer--;

    if(Q->statusBuffer==0) Q->last = Q->head;
    if( Q->statusBuffer == (Q->sizeBuffer)-1 ) pthread_cond_broadcast(&empty);
    //Destrava o mutex
    pthread_mutex_unlock(&mutex);
    return result;
    
}



void *rotina_produtor(void* threadid){   
    printf("produtor iniciou! thread %d\n", *((int*) threadid));
    while(1){
        putBlockingQueue(Q, rand()%99);
        printf("thread %d adicionou no buffer\n", *((int*) threadid));
    }
    pthread_exit(NULL);
}


void *rotina_consumidor(void *threadid){
    int rc;
    printf("consumidor iniciou! thread %d\n", *((int*) threadid));
    while(1){
    
        rc = takeBlockingQueue(Q);
        printf("thread %d retirou %d do buffer\n", *((int*) threadid), rc);

    }
    
    pthread_exit(NULL);

}

int main(){
    //Criando fila bloqueante
    Q = newBlockingQueue(B); 

    //Criando threads
    int* id_consumidor[C];
    int* id_produtor[P];
	pthread_t consumidor[C];
	pthread_t produtor[P];

    //Inicializando os vetores de threads

    //Produtor
    int i = 0;
    int rc;
    for(i=0;i<P;i++){
        id_produtor[i] = (int*)malloc(sizeof(int));
        *id_produtor[i] = i;
        rc = pthread_create(&produtor[i],NULL,rotina_produtor,(void *) id_produtor[i]);
        
        if(rc){
            printf("ERRO NA CRIACAO DA THREAD PRODUTOR!\n");
            exit(-1);
        }
    }

    //Consumidor
    i = 0;
    for(i=0;i<C;i++){
        id_consumidor[i] = (int *) malloc(sizeof(int));
        *id_consumidor[i] = i;
        rc = pthread_create(&consumidor[i],NULL,rotina_consumidor,(void *) id_consumidor[i]);

         if(rc){
            printf("ERRO NA CRIACAO DA THREAD CONSUMIDOR!\n");
            exit(-1);
        }
    
    }
    

  
    //Já que o pthread_exit espera todas as threads terminarem para encerrar o programa
    //(e nenhuma delas termina), basta chamá-lo em vez de 'return 0'. 
    pthread_exit(NULL);
}
