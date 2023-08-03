#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
    
int N = 4; //Quantidade máxima de valores de threads.
int cur_threads = 0; // Quantidade de threads executando

int scheduledExec = -1; //Execuções agendadas até então

//Data para buffer de entrada
typedef struct data{
    int a;
    int b;
    int id;
}Data;

//Data para a buffer de saida
typedef struct datar{
    int result;
    int id;
}DataR;

// Node para o buffer de saída
typedef struct noder{   
    DataR* data; 
    struct noder* next;
}NodeR;

//Node para o buffer de entrada 
typedef struct node{
    void* (*func)(void*);
    void* k; 
    struct node* next;
}Node;

//Buffer de entrada
typedef struct buffer{
    Node *head;
    Node *tail;
    int size;
}Buffer;

//Buffer de saída
typedef struct bufferR{
    int size;
    NodeR *head;
    NodeR *tail;
}BufferR;

//Buffer de entrada
Buffer* InputBuffer =NULL;
//Buffer de saida
BufferR* OutputBuffer =NULL;
// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// Variável de condição
pthread_cond_t condition = PTHREAD_COND_INITIALIZER; 

//Funcao que será requisitada

void* add( void* input){       
    //recebe os valores a ser somados 
    int c = ((Data*)input)->a;
    int d = ((Data*)input)->b;
    int id = ((Data*)input)->id;
    //Recebe o resultado da soma e o id 
    DataR* output = (DataR*)malloc(sizeof(DataR));
    output->result = c+d;
    output->id = id;
    NodeR* new = (NodeR*)malloc(sizeof(NodeR));
    new->data = output;
    new->next = NULL;

    //Tenta travar o mutex continuamente, caso não consiga, 
    //a thread é posta para dormir(pelo carater bloqueante do pthread_mutex_lock), 
    //caso consiga, pthread_mutex_lock retorna 0 e sai do while
    while(pthread_mutex_lock(&mutex)){}

    //adiciona ao buffer de respostas
    OutputBuffer->tail->next = new;
    OutputBuffer->tail = new;
    OutputBuffer->size++;
    //Sinaliza que a thread terminou.
    cur_threads--;

/*  Acorda o despachante e o usuário(
    caso tenha solicitado a função pegarResultadoFunção),
    avisando que há uma thread disponível
*/
    pthread_cond_broadcast(&condition);
    //Destrava o mutex
    pthread_mutex_unlock(&mutex);
    //fim de uma rotina    
    pthread_exit(NULL);
}

void* mult( void* input){       
    //recebe os valores a ser multiplicados 
    int c = ((Data*)input)->a;
    int d = ((Data*)input)->b;
    int id = ((Data*)input)->id;
    //Recebe o resultado da multplicação e o id 
    DataR* output = (DataR*)malloc(sizeof(DataR));
    output->result = c*d;
    output->id = id;
    NodeR* new = (NodeR*)malloc(sizeof(NodeR));
    new->data = output;
    new->next = NULL;

    //Tenta travar o mutex continuamente, caso não consiga, 
    //a thread é posta para dormir(pelo carater bloqueante do pthread_mutex_lock), 
    //caso consiga, pthread_mutex_lock retorna 0 e sai do while
    while(pthread_mutex_lock(&mutex)){}

    //adiciona ao buffer de respostas
    OutputBuffer->tail->next = new;
    OutputBuffer->tail = new;
    OutputBuffer->size++;
    //Sinaliza que a thread terminou.
    cur_threads--;

/*  Acorda o despachante e o usuário(
    caso tenha solicitado a função pegarResultadoFunção),
    avisando que há uma thread disponível
*/
    pthread_cond_broadcast(&condition);
    //Destrava o mutex
    pthread_mutex_unlock(&mutex);
    //fim de uma rotina    
    pthread_exit(NULL);
}




int pegarResultadoFuncao(int id){
    int val = id;
    int out_of_bounds = 1;

    //Se a função está no buffer de saída retorna o valor, caso não esteja, printa uma mensagem pro usuário e retorna -1.
    if(id<= scheduledExec && id>=0) out_of_bounds = 0; 
    while(out_of_bounds){
        printf("O id %d esta errado! Por favor, digite um id valido ou para voltar ao menu, digite  ' - 1 ' \n", val); // ID fora dos limites
        scanf("%d", &val);
        if(val == -1) return -1;
        if(val<= scheduledExec && val>=0 ) out_of_bounds = 0;
    }
    //Trava o mutex (para fazer a busca no buffer)
    pthread_mutex_lock(&mutex);
    //Checa se tem o id no buffer de saída
    NodeR* cur = NULL;
    cur = OutputBuffer->head;
    //Se o id já foi retirado do buffer(já que os ids são sequenciais), printa uma mensagem, libera o mutex e retorna -1
    if(cur->next ==NULL || cur->next->data->id > val){
        printf("Esta id ja foi usada !! E o resultado ja foi passado ao usuario!\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    //Faz a busca no buffer de saída, caso não encontre o id válido,
    // dorme até um próximo elemento ser adicionado no buffer de saída
    while(1){    //Busca pelo id na fila. 
        for(cur = OutputBuffer->head;cur->next!=NULL;cur=cur->next){
            if(cur->next->data->id == val){   
                //Se encontrou, salva o resultado e retira o elemento do buffer de saída
                int ret = cur->next->data->result;
                NodeR* temp = cur->next;
                cur->next = temp->next;
                OutputBuffer->tail = cur; 
                free(temp);
                pthread_mutex_unlock(&mutex);
                return ret;
            }
        }
/*      
        Se não encontrou, dorme até outro elemento ser adicionado no buffer de saída.
        A variável de condição é a mesma do despachante pois, quando o despachante é acordado,
        significa que uma das threads adicionou no buffer de saída e agora está livre,
        Então há possibilidade do novo elemento ser o elemento com o id procurado.
*/      pthread_cond_wait(&condition, &mutex);
    }
}




int agendarExecucao( void* (*routine)(void*), void* data ){
    //Tenta travar o mutex
    while(pthread_mutex_lock(&mutex)){};
    
    //Aloca memória para o buffer de entrada
    Data* inp = (Data*)malloc(sizeof(Data));
    *inp = *(Data*)data;

    // Adicionando ao Buffer de entrada
    Node* new = (Node*)malloc(sizeof(Node));
    new->next = NULL;
    new->func = routine;
    new->k = (void*) inp;
    InputBuffer->tail->next = new;
    InputBuffer->tail = new;
    InputBuffer->size++;

    //Printando o identificador para o usuário
    printf("Execucao agendada com sucesso! identificador = %d\n", inp->id);

    //Envia o sinal e libera o mutex
    pthread_cond_broadcast(&condition);
    pthread_mutex_unlock(&mutex);

    return inp->id;
}

void* user_routine(){
    Data input; // parâmetros de entrada da função
    int m = -1;
    int id; // id que vai ser retornada ao usuário ao agendar execução
    int in; // id que vai ser checada 
    
    //MENU
   while(1){ 
    printf("Digite a operacao desejada : \n");
    printf("1 -> Agendar Execucao\t2 -> Pegar resultado\t3 -> Sair\n");
    scanf("%d", &m);
    switch (m){
        case 1:
            printf("Voce deseja : 1 -> Somar  2 -> Multiplicar ");    
            scanf("%d",&m);
            if(m == 1){

                printf("digite o primeiro valor: ");
                scanf("%d", &input.a);
                printf("Digite o segundo valor: ");
                scanf("%d", &input.b);
                scheduledExec++;
                input.id = scheduledExec;
                id = agendarExecucao(add, (void*) &input);

            }
            else if(m == 2){

                printf("digite o primeiro valor: ");
                scanf("%d", &input.a);
                printf("Digite o segundo valor: ");
                scanf("%d", &input.b);
                scheduledExec++;
                input.id = scheduledExec;
                id = agendarExecucao(mult, (void*) &input);

            }
            else
            {
                printf("valor invalido\n\n");
            }
            break;
        case 2 : 
            printf("Digite o id: ");
            scanf("%d", &in);
            int out = pegarResultadoFuncao(in);
            printf("O valor retornado é %d!\n\n", out);
            break;
        case 3 : 
            printf("FINALIZANDO O PROGRAMA\n\n");
            pthread_exit(NULL);
            break;
        default:
            printf("Digite uma opcao valida\n\n");
            break;
        } 
   }
}

void* dispatcher_routine()
{

    //Tenta travar o mutex
    pthread_mutex_lock(&mutex);

    //Se o buffer tá vazio ou as threads estão ocupadas, a thread dorme
    while(InputBuffer->size==0 || cur_threads==N){
        if(InputBuffer->size==0) InputBuffer->tail = InputBuffer-> head;
        pthread_cond_wait(&condition,&mutex);
    }

    //Retirada do elemento
    Node* temp=NULL;
    temp = InputBuffer->head->next;
    InputBuffer->head->next = temp->next;
    //printf("Na despachante: temp->k->a = %d,temp->k->b = %d \n",((Data*)temp->k)->a,((Data*)temp->k)->b);
    
    //Criação de uma nova thread
    pthread_t new_thread;
    
    pthread_create(&new_thread, NULL, temp->func, temp->k );

    // Atualizando o size do InputBuffer e o cur_threads
    InputBuffer->size--;
    cur_threads++;
    
    //liberando o mutex
    pthread_mutex_unlock(&mutex);
}

void* dispatch(){   
    while(1){
        dispatcher_routine();
    }
    pthread_exit(NULL);
}



int main(){
    //inicializando o buffer de entrada
    InputBuffer = (Buffer*)(malloc(sizeof(Buffer)));
    InputBuffer->head = (Node*)(malloc(sizeof(Node)));
    InputBuffer->tail = InputBuffer->head;
    InputBuffer->size = 0; 

    //Inicializando o buffer de saída
    OutputBuffer = (BufferR*)(malloc(sizeof(BufferR)));
    OutputBuffer->head = (NodeR*)malloc(sizeof(NodeR));
    OutputBuffer->tail = OutputBuffer->head;
    OutputBuffer->size = 0;

    //Thread despachante
    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL,dispatch,NULL);

    //Thread de usuario
    pthread_t user;
    pthread_create(&user,NULL,user_routine,NULL);

    //Espera a thread usuário terminar
    pthread_join(user,NULL);

    return 0;
}
