#include <iostream>
#include <vector>
#include <pthread.h>

#define NUM_THREADS 10
#define MAX_NODES 10

//basicamente: criar um grafo, percorrer o grafo com multiplas threads ate encontrar um ciclo(deadlock)

typedef struct Graph {
    std::vector<int> adj[MAX_NODES]; //vetor p armazenar arestas
    bool visited[MAX_NODES]; //marcar vertices visitados
    pthread_mutex_t lock; //sincronizar acesso
} Graph;

typedef struct ThreadData {
    Graph* graph;
    int start_node; //no inicial da busca
    bool* has_cycle; //ponteiro p indicar se ha um deadlock(ciclo)
} ThreadData;

void* depth_first_search(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    Graph* graph = data->graph;
    int start_node = data->start_node;
    bool* has_cycle = data->has_cycle;

    //bloqueando o acesso ao grafo c mutex
    pthread_mutex_lock(&graph->lock);

    //condicao p verificar se o no ja foi visitado
    if (graph->visited[start_node]){
        //caso tenha sido, ha um ciclo
        *has_cycle = true;
        //libera
        pthread_mutex_unlock(&graph->lock);
        //encerra
        pthread_exit(NULL);
    }

    graph->visited[start_node] = true;
    pthread_mutex_unlock(&graph->lock);

    for (int i = 0; i < graph->adj[start_node].size(); i++) {
        int next_node = graph->adj[start_node][i];
        if (*has_cycle) {
            pthread_exit(NULL);
        }

        ThreadData next_data;
        next_data.graph = graph;
        next_data.start_node = next_node;
        next_data.has_cycle = has_cycle;

        pthread_t next_thread;
        pthread_create(&next_thread, NULL, depth_first_search, &next_data);
        pthread_join(next_thread, NULL);
    }

    pthread_exit(NULL);
}

int main() {
    Graph graph;
    for (int i = 0; i < MAX_NODES; i++) {
        graph.visited[i] = false; // Inicializa o vetor de visitados
    }
    pthread_mutex_init(&graph.lock, NULL); // Inicializa o mutex

    // Adiciona as arestas do grafo
    graph.adj[0].push_back(1);
    graph.adj[1].push_back(2);
    graph.adj[2].push_back(3);
    graph.adj[3].push_back(4);
    graph.adj[4].push_back(1); // Primeiro ciclo
    graph.adj[5].push_back(6);
    graph.adj[6].push_back(7);
    graph.adj[7].push_back(8);
    graph.adj[8].push_back(5); // Segundo ciclo

    bool has_cycle = false; // Flag para indicar se encontrou ciclo
    std::vector<ThreadData> thread_data(NUM_THREADS); // Cria o vetor de dados das threads

    // Define os dados das threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].graph = &graph; // Passa o ponteiro para o grafo
        thread_data[i].start_node = i; // Define o nó inicial da thread
        thread_data[i].has_cycle = &has_cycle; // Passa o ponteiro para a variável de ciclo
    }

    // Cria as threads
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, depth_first_search, (void*) &thread_data[i]);
    }

    // Espera as threads terminarem
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Verifica se algum nó ficou sem visitar
    for (int i = 0; i < MAX_NODES; i++) {
        if (!graph.visited[i]) {
            std::cout << "O grafo não é conexo." << std::endl;
            return 0;
        }
    }

    // Se chegou aqui, o grafo é conexo
    std::cout << "O grafo é conexo." << std::endl;

    // Verifica se encontrou ciclo
    if (has_cycle) {
        std::cout << "O grafo tem ciclo." << std::endl;
    } else {
        std::cout << "O grafo não tem ciclo." << std::endl;
    }

    return 0;
}
