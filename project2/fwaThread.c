#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

#define MAX_VAL 1000

sem_t semWriter; // binary semaphore
pthread_mutex_t readLock;

struct arg_s
{
    int n;
    int i;
    int k;
};

int numReadersIn = 0;
float threadsTime = 0;

double timeTaken(struct timespec start, struct timespec end);
int readInput();
int validateEdgeInput(int *edge, int num_nodes);
float fwa(int num_nodes);
void *worker(void *args);
void ReaderEnter();
void readerLeave();
void displayMatrix(int **matrix, int num_nodes);

int **graph;
int **dist;

/**
 * @brief entry point
 */
int main(int argc, char const *argv[])
{

    sem_init(&semWriter, 0, 1);
    pthread_mutex_init(&readLock, NULL);
    int num_nodes = readInput();

    float elapsed = fwa(num_nodes);

    displayMatrix(dist, num_nodes);

    printf("\nTime elapsed: %f\n", elapsed);
    printf("Time elapsed due to thread creation: %f\n", threadsTime);
    printf("Time elapsed for FWA: %f\n", elapsed - threadsTime);

    sem_destroy(&semWriter);
    pthread_mutex_destroy(&readLock);
    free(graph);
    free(dist);
    return 0;
}

/**
 * @brief return the wall time passed between the start and end timers
 *
 * @param start strart time
 * @param end end time
 * @return double
 */
double timeTaken(struct timespec start, struct timespec end)
{
    return ((double)end.tv_sec + 1.0e-9 * end.tv_nsec) - ((double)start.tv_sec + 1.0e-9 * start.tv_nsec);
}

/**
 * @brief gets the input from the user, initialize the graph and dist matrix
 *
 * @return int
 */
int readInput()
{
    int num_nodes, num_edges;
    int edge[3];

    printf("Input:\n");

    scanf("%d %d", &num_nodes, &num_edges);

    // ensure the number of nodes for the matrix is at least 3
    while (num_nodes < 3)
    {
        printf("There must be at least 3 nodes for the algorithm to make sense\n");
        printf("Re-enter the number of nodes: ");
        scanf("%d", &num_nodes);
    }

    // ensure that the number of nodes requested to assign weight to is sensible
    while (num_edges < 0 || num_edges > num_nodes * num_nodes)
    {
        printf("Illegal number of edges to be inserted\n");
        printf("Re-enter the number of edges to add weight to: ");
        scanf("%d", &num_edges);
    }

    // initialize matrices
    graph = malloc(num_nodes * sizeof(int *));
    dist = malloc(num_nodes * sizeof(int *));

    for (int i = 0; i < num_nodes; i++)
    {
        graph[i] = malloc(num_nodes * sizeof(int));
        dist[i] = malloc(num_nodes * sizeof(int));
        for (int j = 0; j < num_nodes; j++)
        {
            graph[i][j] = 0;                     // set the empty edge to 0
            dist[i][j] = (i == j) ? 0 : MAX_VAL; // set the empty edge to infinity (0, when same node pointed by i and j)
        }
    }

    for (int i = 0; i < num_edges; i++)
    {
        scanf("%d %d %d", &edge[0], &edge[1], &edge[2]);
        while (!validateEdgeInput(edge, num_nodes))
        {
            printf("Previous line is ignored due to incorrect value, please enter valid inputs!\n");
            scanf("%d %d %d", &edge[0], &edge[1], &edge[2]);
        }

        // assign weight to edges in dist matrix and set graph matrix to 1
        dist[edge[0] - 1][edge[1] - 1] = edge[2];
        graph[edge[0] - 1][edge[1] - 1] = 1;
        dist[edge[1] - 1][edge[0] - 1] = edge[2];
        graph[edge[1] - 1][edge[0] - 1] = 1;
    }

    return num_nodes;
}

/**
 * @brief validate the 3 components for assigning a weight to an edge
 *
 * @param edge array containing the 3 components to be validated
 * @param num_nodes number of nodes
 * @return int
 */
int validateEdgeInput(int *edge, int num_nodes)
{
    int isValid = 1;

    // ensures weight of nodes to themselves stays 0
    if (edge[0] == edge[1])
    {
        printf("weight of nodes to themselves is always 0!\n");
        isValid = 0;
    }

    // ensures a correct nodes is selected
    for (int i = 0; i < 2; i++)
    {
        if ((edge[i] < 1 || edge[i] > num_nodes))
        {
            printf("%s node %d does not exist!\n", i == 0 ? "Starting" : "Destination", edge[i]);
            isValid = 0;
        }
    }

    // ensures weight assigned is of proper weight
    if (edge[2] < 0 || edge[2] > MAX_VAL)
    {
        printf("Weight %d is too %s!\n", edge[2], edge[2] < 0 ? "small" : "large");
        isValid = 0;
    }

    return isValid;
}

/**
 * @brief iterate through the transit nodes and start node and create a thread for the destination node
 *
 * @param num_nodes number of nodes
 * @return float
 */
float fwa(int num_nodes)
{
    struct timespec start, end, startThreadTime, endThreadTime;
    pthread_t *threads = (pthread_t *)malloc(num_nodes * sizeof(pthread_t));
    struct arg_s arguments[num_nodes];

    clock_gettime(CLOCK_MONOTONIC, &start);
    // loop through transit nodes
    for (int k = 0; k < num_nodes; k++)
    {
        clock_gettime(CLOCK_MONOTONIC, &startThreadTime);
        // loop through start nodes
        for (int i = 0; i < num_nodes; i++)
        {
            arguments[i].n = num_nodes;
            arguments[i].i = i;
            arguments[i].k = k;
            pthread_create(&threads[i], NULL, worker, (void *)&arguments[i]);
        }

        clock_gettime(CLOCK_MONOTONIC, &endThreadTime);

        threadsTime += timeTaken(startThreadTime, endThreadTime);

        for (int i = 0; i < num_nodes; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    return timeTaken(start, end);
}

/**
 * @brief iterate through the destination nodes
 *
 * @param args struct of type arg_s which stores the number of nodes (n), start node(i), and the transit node (k)
 * @return void*
 */
void *worker(void *args)
{
    // get the value of n,i and k
    struct arg_s arguments = *(struct arg_s *)args;
    int n = arguments.n;
    int i = arguments.i;
    int k = arguments.k;

    // loop through destination nodes
    for (int j = 0; j < n; j++)
    {
        ReaderEnter();
        if (dist[i][k] + dist[k][j] < dist[i][j])
        {
            readerLeave();

            sem_wait(&semWriter); // writer acquires the lock, or wait if any reader is in
            dist[i][j] = dist[i][k] + dist[k][j];
            dist[j][i] = dist[i][k] + dist[k][j];
            sem_post(&semWriter); // writer releases the lock as it leaves
        }
        else
        {
            readerLeave(); // leaves without writing
        }
    }

    pthread_exit(NULL);
}

/**
 * @brief implements the lock to allow multiple readers and lock the writer/
 * block the readers incase a writer is already in critical section
 */
void ReaderEnter()
{
    pthread_mutex_lock(&readLock);
    numReadersIn++;
    if (numReadersIn == 1)
    {
        sem_wait(&semWriter); // block the writer from getting in, also blocks if writer was already in
    }
    pthread_mutex_unlock(&readLock);
}

/**
 * @brief implements the unlock that allow the last reader to leave to unlock the writer
 */
void readerLeave()
{
    pthread_mutex_lock(&readLock);
    numReadersIn--;
    if (numReadersIn == 0)
    {
        sem_post(&semWriter);
    }
    pthread_mutex_unlock(&readLock);
}

/**
 * @brief display the matrix passed to it
 *
 * @param matrix the matrix to be displayed
 * @param num_nodes number of nodes
 */
void displayMatrix(int **matrix, int num_nodes)
{
    printf("\nOutput:\n");
    for (int i = 0; i < num_nodes; i++)
    {
        for (int j = 0; j < num_nodes; j++)
        {
            if (matrix[i][j] >= MAX_VAL)
            {
                printf("INF ");
            }
            else
            {
                printf("%d ", matrix[i][j]);
            }
        }
        printf("\n");
    }
}