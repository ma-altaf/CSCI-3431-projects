#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAXVAL 100

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    FILE *fp;
    char filename[255];

    int numNodes;
    printf("Please, enter the number of nodes: ");
    if (scanf("%d", &numNodes) != 1)
    {
        exit(1);
    }

    snprintf(filename, sizeof(filename), "RandomNodes_%d.txt", numNodes);

    fp = fopen(filename, "w");

    fprintf(fp, "%d %d\n", numNodes, ((numNodes * numNodes) / 2 - (numNodes / 2)));

    for (int i = 0; i < numNodes - 1; i++)
    {
        for (int j = i + 1; j < numNodes; j++)
        {
            fprintf(fp, "%d %d %d\n", i + 1, j + 1, (rand() % MAXVAL) + 1);
        }
    }

    fclose(fp);

    printf("random nodes created in file name: RandomNodes_%d.txt\n", numNodes);

    return 0;
}
