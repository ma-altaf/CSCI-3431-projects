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

int readInput();
void displayMatrix(int **matrix, int num_nodes);
void *worker(void *args);
float fwa(int num_nodes);
void ReaderEnter();
void Readerleave();
double timeTaken(struct timespec start, struct timespec end);

int **graph;
int **dist;

double timeTaken(struct timespec start, struct timespec end)
{
    return ((double)end.tv_sec + 1.0e-9 * end.tv_nsec) - ((double)start.tv_sec + 1.0e-9 * start.tv_nsec);
}

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

float fwa(int num_nodes)
{
    struct timespec start, end, timed, endTimed;
    pthread_t *threads = (pthread_t *)malloc(num_nodes * sizeof(pthread_t));
    struct arg_s arguments[num_nodes];

    clock_gettime(CLOCK_MONOTONIC, &start);
    // loop through transit nodes
    for (int k = 0; k < num_nodes; k++)
    {
        clock_gettime(CLOCK_MONOTONIC, &timed);
        // loop through start nodes
        for (int i = 0; i < num_nodes; i++)
        {
            arguments[i].n = num_nodes;
            arguments[i].i = i;
            arguments[i].k = k;
            pthread_create(&threads[i], NULL, worker, (void *)&arguments[i]);
        }

        clock_gettime(CLOCK_MONOTONIC, &endTimed);

        threadsTime += timeTaken(timed, endTimed);

        for (int i = 0; i < num_nodes; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    return timeTaken(start, end);
}

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
            Readerleave();

            sem_wait(&semWriter); // writer acquires the lock, or wait if any reader is in
            dist[i][j] = dist[i][k] + dist[k][j];
            dist[j][i] = dist[i][k] + dist[k][j];
            sem_post(&semWriter); // writer releases the lock as it leaves
        }
        else
        {
            Readerleave();
        }
    }
    pthread_exit(NULL);
}

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

void Readerleave()
{
    pthread_mutex_lock(&readLock);
    numReadersIn--;
    if (numReadersIn == 0)
    {
        sem_post(&semWriter);
    }
    pthread_mutex_unlock(&readLock);
}

int readInput()
{
    int num_nodes, num_edges;
    int edge[3];
    char filename[255];
    FILE *fp;

    printf("Enter filename for input: ");
    scanf("%s", filename);

    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("file could not be opened");
        exit(1);
    }

    fscanf(fp, "%d %d", &num_nodes, &num_edges);

    graph = malloc(num_nodes * sizeof(int *));
    dist = malloc(num_nodes * sizeof(int *));
    for (int i = 0; i < num_nodes; i++)
    {
        graph[i] = malloc(num_nodes * sizeof(int));
        dist[i] = malloc(num_nodes * sizeof(int));
        for (int j = 0; j < num_nodes; j++)
        {
            graph[i][j] = 0;                     // set the empty edge to 0
            dist[i][j] = (i == j) ? 0 : MAX_VAL; // set the empty edge to infinity
        }
    }

    for (int i = 0; i < num_edges; i++)
    {
        fscanf(fp, "%d %d %d", &edge[0], &edge[1], &edge[2]);

        dist[edge[0] - 1][edge[1] - 1] = edge[2];
        graph[edge[0] - 1][edge[1] - 1] = 1;
        dist[edge[1] - 1][edge[0] - 1] = edge[2];
        graph[edge[1] - 1][edge[0] - 1] = 1;
    }

    fclose(fp);

    return num_nodes;
}

void displayMatrix(int **matrix, int num_nodes)
{
    printf("\noutput:\n");
    for (int i = 0; i < num_nodes; i++)
    {
        for (int j = 0; j < num_nodes; j++)
        {
            if (matrix[i][j] == MAX_VAL)
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