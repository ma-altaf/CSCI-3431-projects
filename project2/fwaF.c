#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_VAL 1000

float fwa(int num_nodes);
int readInput();
void displayMatrix(int **matrix, int num_nodes);
double timeTaken(struct timespec start, struct timespec end);

int **graph;
int **dist;

double timeTaken(struct timespec start, struct timespec end)
{
    return ((double)end.tv_sec + 1.0e-9 * end.tv_nsec) - ((double)start.tv_sec + 1.0e-9 * start.tv_nsec);
}

int main(int argc, char const *argv[])
{
    int num_nodes = readInput();

    float elapsed = fwa(num_nodes);

    displayMatrix(dist, num_nodes);
    printf("\nTime elapsed for FWA: %f\n", elapsed);
    free(graph);
    free(dist);
    return 0;
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

float fwa(int num_nodes)
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int k = 0; k < num_nodes; k++)
    {
        for (int i = 0; i < num_nodes; i++)
        {
            for (int j = 0; j < num_nodes; j++)
            {
                if (dist[i][k] + dist[k][j] < dist[i][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    dist[j][i] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    return timeTaken(start, end);
}

void displayMatrix(int **matrix, int num_nodes)
{
    printf("\nOutput:\n");
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