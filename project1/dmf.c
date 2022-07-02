#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define K 5
#define EMPTY_ARR -1
#define FILE_ERR -100
#define REQUEST 100
#define PIVOT 200
#define LARGE 300
#define SMALL 400
#define INT_SIZE sizeof(int)
#define PAUSE 0.5

int childProcess(int id);
void childPipeSetUp(int id);
int parentProcess(int *pidArr);
void request(int id, int *childArr, int arraySize);
int pivot(int id, int *childArr, int arraySize);
int small(int pivot, int *childArr, int arraySize);
int large(int pivot, int *childArr, int arraySize);
int readFile(int id);
void closeChildren(int *pidArr);
void handler();
void halt();

int parent_fd[K][2], child_fd[K][2];
int *numPtr;   // declared globally in order to be closed when handling kill signal
int processID; // declared globally in order to close related pipe file descriptor when handling kill signal

int main(int argc, char const *argv[])
{
    int pidArr[K];
    int pid = 1; // initialized to 1 to run the parent code initially

    // create pipes for communication
    for (int i = 0; i < K; i++)
    {
        pipe(parent_fd[i]);
        pipe(child_fd[i]);
    }

    // fork 5 children
    for (int i = 0; i < K; i++)
    {
        if (pid > 0)
        {
            pid = fork();
            pidArr[i] = pid;
        }

        if (pid < 0)
        {
            printf("Could not create process number %d!", (i + 1));
            closeChildren(pidArr);
            exit(1);
        }

        if (pid == 0)
        {
            childProcess(i);
            break;
        }
    }

    // run the parent process
    if (pid > 0)
    {
        parentProcess(pidArr);
    }

    return 0;
}

/*
    call when program exits to close child processes and file descripters
*/
void closeChildren(int *pidArr)
{

    printf("\nParent sends kill signals to children\n\n");

    for (int i = 0; i < K; i++)
    {
        close(child_fd[i][0]);
        close(parent_fd[i][1]);
        kill(pidArr[i], SIGTERM);
        waitpid(pidArr[i], NULL, 0);
        printf("Child %d terminated\n", (i + 1));
    }
}

/*
    parent process
*/
int parentProcess(int *pidArr)
{
    int emptyProcesses[K] = {0}; // flag for process being closed
    srand(time(NULL));           // set seed for pseudo random number generator
    int k = 0;
    int evenFlag = 0;
    int medianBuf = 0;

    // close unused pipe for parent process
    for (int i = 0; i < K; i++)
    {
        close(child_fd[i][1]);
        close(parent_fd[i][0]);
    }

    // wait for children to be ready and get total quantity of numbers
    for (int i = 0; i < K; i++)
    {
        int childArrSize;
        if (read(child_fd[i][0], &childArrSize, INT_SIZE) == -1)
        {
            printf("Could not read ready confirmation from child %d\n", i);
            closeChildren(pidArr);
            exit(2);
        }
        // exit if child contains in incorrect quantity of numbers / file error
        if (childArrSize < 0)
        {
            printf("Child %d responded with a wrong quantity of numbers, program ending.\n", (i + 1));
            closeChildren(pidArr);
            exit(3);
        }
        k += childArrSize; // increment quantity of numbers
    }

    // set even qunatity of numbers flag
    if (k % 2 == 0)
    {
        evenFlag = 1;
    }

    // set up initial value of k
    k = (k / 2) + 1; // + 1 since the median is also included

    printf("Parent is READY\n");

    if (evenFlag)
    {
        printf("Since there are even quantity of numbers, k = %d and k = %d.\n", (k - 1), k);
    }
    else
    {
        printf("Initial value of k is %d.\n", k);
    }

    halt(); // wait for user to press enter to continue

    // enter loop for median finding algorithm
    while (1)
    {
        int m = 0;
        int randChild, pivot, code;

        // loop until the random child selected is not empty
        while (emptyProcesses[(randChild = rand() % K)] == 1)
            ;

        code = REQUEST;
        printf("\nParent sends requests to child %d\n", (randChild + 1));
        if (write(parent_fd[randChild][1], &code, INT_SIZE) == -1)
        {
            printf("Could not send request to child %d\n", randChild);
            closeChildren(pidArr);
            exit(1);
        }

        if (read(child_fd[randChild][0], &pivot, INT_SIZE) == -1)
        {
            printf("Could not read pivot from child %d\n", randChild);
            closeChildren(pidArr);
            exit(2);
        }

        if (pivot >= 0)
        {
            printf("Child %d sends %d to parent. Parent broadcast pivot to children", (randChild + 1), pivot);
        }
        else
        {
            // no pivot was obtained
            printf("Child %d has no numbers left!\n", (randChild + 1));
            emptyProcesses[randChild] = 1; // set flag to state that the process have no element
            // test to see if there are any element left
            int numEmptyChild = 0;
            for (int i = 0; i < K; i++)
            {
                numEmptyChild += emptyProcesses[i];
            }
            // close program if there are no numbers left
            if (numEmptyChild == K)
            {
                printf("\nAll children have no elements left!\n");
                printf("median could not be found.\n");
                closeChildren(pidArr);
                exit(0);
            }

            continue;
        }

        code = PIVOT; // use to send PIVOT code to children
        // send PIVOT to all children
        for (int i = 0; i < K; i++)
        {
            // send PIVOT code
            if (write(parent_fd[i][1], &code, INT_SIZE) < 0)
            {
                printf("Could not send pivot code to child %d\n", i);
                closeChildren(pidArr);
                exit(1);
            }
            // send the pivot number
            if (write(parent_fd[i][1], &pivot, INT_SIZE) < 0)
            {
                printf("Could not send the pivot value to child %d\n", i);
                closeChildren(pidArr);
                exit(1);
            }
        }

        sleep(PAUSE); // wait in order for the following text to be printed last

        // display m calculation
        printf("\nm = ");
        for (int i = 0; i < K; i++)
        {
            int buf;
            if (read(child_fd[i][0], &buf, INT_SIZE) == -1)
            {
                printf("Could not read amount of numbers greater than pivot from child %d\n", randChild);
                closeChildren(pidArr);
                exit(2);
            }
            m += buf;
            printf("%d %s ", buf, (i == K - 1) ? "=" : "+");
        }
        printf("%d\n", m);

        // check if we got a/the median value
        if (k == m || (evenFlag && !medianBuf && (k - 1) == m))
        {
            // if check even flag set
            if (evenFlag)
            {
                // check if mdeian value need to be stored
                if (medianBuf == 0)
                {
                    medianBuf = pivot;

                    printf("\nm(%d) = k(%d). First component of median found! It is %d.\n", m, (k == m) ? k : (k - 1), medianBuf);

                    // check if number was found through k or (k - 1)
                    if (k == m)
                    {
                        printf("Values less than the higher median are being dropped (values less than and equal to %d are being dropped)\n", medianBuf);
                        code = SMALL;
                        k--; // update k value since k for even (k - 1) will not be used
                    }
                    else
                    {
                        printf("Values greater than the lower median are being dropped (values greater than and equal to %d are being dropped)\n", medianBuf);
                        printf("k set to 1 since values greater than median are being dropped.\n");
                        code = LARGE;
                        k = 1; // update k since all values greater than the median we need to find next will be removed
                    }

                    // sending code to drop values greter/lower and equal to pivot
                    for (int i = 0; i < K; i++)
                    {
                        if (write(parent_fd[i][1], &code, INT_SIZE) < 0)
                        {
                            printf("Could not send %s code to child %d\n", (code == SMALL ? "SMALL" : "LARGE"), i);
                            closeChildren(pidArr);
                            exit(1);
                        }
                    }

                    halt(); // wait for user to press enter to continue

                    continue; // do not run the follwing code
                }

                printf("\nm(%d) = k(%d). Second component of median found! It is %d.", m, k, pivot);
                printf("\nMedian is (%d + %d) / 2 = %.2f (2dp)\n", pivot, medianBuf, (pivot + medianBuf) / 2.0);
            }
            else
            {
                printf("\nm(%d) = k(%d). Median Found!\n", m, k);
                printf("Median is %d \n", pivot);
            }

            halt(); // wait for user to press enter to continue

            closeChildren(pidArr);

            exit(0);
        }
        else
        {
            // check if SMALL or LARGE code need to be sent
            code = m > k ? SMALL : LARGE;

            printf("\n"); // empty line

            // display whether to drop smaller or larger values than pivot
            if (code == SMALL)
            {
                if ((evenFlag && !medianBuf))
                {
                    printf("m(%d) > k(%d) and ", m, (k - 1));
                }

                printf("m(%d) > k(%d). send SMALL code to drop values smaller than and equal to pivot.\n", m, k);
            }
            else
            {
                if ((evenFlag && !medianBuf))
                {
                    printf("m(%d) < k(%d) and ", m, (k - 1));
                }
                printf("m(%d) < k(%d). send LARGE code to drop values larger than and equal to pivot.\n", m, k);

                printf("Decrement k(");
                if (evenFlag && !medianBuf)
                {
                    printf("%d and ", (k - 1));
                }
                printf("%d) by m(%d) since values larger than and equal to the pivot are being dropped.\n", k, m);
                k -= m;

                printf("Updated ");
                if (evenFlag && !medianBuf)
                {
                    printf("k = %d and ", (k - 1));
                }
                printf("k = %d\n", k);
            }

            halt(); // wait for user to press enter to continue

            // send appropriate code to children
            for (int i = 0; i < K; i++)
            {
                if (write(parent_fd[i][1], &code, INT_SIZE) < 0)
                {
                    printf("Could not send %s code to child %d\n", (code == SMALL ? "SMALL" : "LARGE"), i);
                    closeChildren(pidArr);
                    exit(1);
                }
            }
        }
    }

    return 0;
}

/*
    read and populate array with content read from the file input_i.txt where i is the id

    return number of element in the array
*/
int readFile(int id)
{
    int numIndex = 0;
    int buffer;
    FILE *fp;
    char filename[255];

    // create the file name to be read by the child
    snprintf(filename, sizeof(filename), "input_%d.txt", id);

    // read the numbers in the appropriate file for the child
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("File %s could not be opened.\n", filename);
        int fileErr = FILE_ERR;
        if (write(child_fd[id][1], &fileErr, INT_SIZE) < 0)
        {
            printf("Could not write file open error from child %d to parent\n", id);
            exit(1);
        }
        // exit(4);
    }

    if ((numPtr = (int *)malloc(INT_SIZE)) == NULL)
    {
        printf("Could not create pointer for numbers array in child %d\n", id);
        exit(1);
    }

    // read the numbers from the file
    while ((fscanf(fp, "%d", &buffer)) == 1)
    {
        if ((numPtr = realloc(numPtr, INT_SIZE * (numIndex + 1))) == NULL)
        {
            printf("Could not increase pointer for numbers array in child %d\n", id);
            free(numPtr);
            exit(1);
        }
        numPtr[numIndex] = buffer;
        numIndex++;
    }

    if (fclose(fp) == EOF)
    {
        printf("%s file could not be closed.\n", filename);
    }

    return numIndex;
}

/*
    child process code
*/
int childProcess(int id)
{
    // set the global value processID to the child id, to be used during kill signal
    processID = id;
    int pivotNum;

    childPipeSetUp(id); // close unused pipe by child

    signal(SIGTERM, &handler); // handle kill signal by parent

    int numIndex = readFile(id);

    // display numbers stored in the child
    printf("Child %d: contains the following numbers: \t", (id + 1));
    for (int in = 0; in < (numIndex - 1); in++)
    {
        printf("%d, ", numPtr[in]);
    }
    printf("%d \n", numPtr[numIndex - 1]);

    // send ready code with quantity of numbers in the child
    printf("Child %d sends READY with %d numbers in it. \n\n", (id + 1), numIndex);
    if (write(child_fd[id][1], &numIndex, INT_SIZE) < 0)
    {
        printf("Could not write ready code / number of elements in child %d to parent\n", id);

        free(numPtr);
        exit(1);
    }

    // enter loop for median finding algorithm
    while (1)
    {
        int code = 0; // store the code to be handled

        if (read(parent_fd[id][0], &code, INT_SIZE) == -1)
        {
            printf("Could not read code from parent in child %d\n", id);

            free(numPtr);
            exit(2);
        }

        // call the appropriate function to handle code received
        switch (code)
        {
        case REQUEST:
            request(id, numPtr, numIndex);
            break;
        case PIVOT:
            pivotNum = pivot(id, numPtr, numIndex); // store the pivot value returned by the pivot function
            break;
        case SMALL:
            numIndex = small(pivotNum, numPtr, numIndex); // update index for quantity of numbers left in array
            break;
        case LARGE:
            numIndex = large(pivotNum, numPtr, numIndex); // update index for quantity of numbers left in array
            break;
        default:
            // no/wrong code was received
            break;
        }
    }

    return 0;
}

/*
    handle kill signal from parent
*/
void handler(int num)
{
    close(child_fd[processID][1]);
    close(parent_fd[processID][0]);
    free(numPtr);
    exit(0);
}

/*
    send a random number from the array held by the child to be the pivot
*/
void request(int id, int *childArr, int arraySize)
{
    int ret = EMPTY_ARR;
    // check if the child still have numbers in its array
    if (arraySize != 0)
    {
        int randomIndex = rand() % arraySize;
        ret = childArr[randomIndex];
    }

    if (write(child_fd[id][1], &ret, INT_SIZE) < 0)
    {
        printf("Could not write random number from child %d to parent\n", id);
        free(childArr);
        exit(1);
    }
}

/*
    send quantity of numbers greater than the pivot to parent

    return pivot value read from parent
*/
int pivot(int id, int *childArr, int arraySize)
{
    int pivotNum;
    int ret = 0;

    // read the pivot
    if (read(parent_fd[id][0], &pivotNum, INT_SIZE) == -1)
    {
        printf("Could not read pivot in child %d\n", id);

        free(childArr);
        exit(2);
    }

    // check if child still have numbers left
    if (arraySize != 0)
    {
        // count qunatity of numbers greater than pivot
        for (int i = 0; i < arraySize; i++)
        {
            if (childArr[i] >= pivotNum)
            {
                ret++;
            }
        }
    }

    printf("Child %d received pivot %d and sends %d.\n", (id + 1), pivotNum, ret);

    // send qunatity of numbers greater than pivot
    if (write(child_fd[id][1], &ret, INT_SIZE) < 0)
    {
        printf("Could not write number of values greater than pivot from child %d\n", id);

        free(childArr);
        exit(1);
    }

    // return the pivot for the child to use for SMALL / LARGE code
    return pivotNum;
}

/*
    update the original array with only numbers larger than pivot.

    return number of element left in array
*/
int small(int pivot, int *childArr, int arraySize)
{
    int index = 0;
    for (int i = 0; i < arraySize; i++)
    {
        if (childArr[i] > pivot)
        {
            // since going from start to end of the array we can overwrite the original array itself
            childArr[index] = childArr[i];
            index++;
        }
    }

    return index;
}

/*
    update the original array with only numbers smaller than pivot

    return number of element left in array
*/
int large(int pivot, int *childArr, int arraySize)
{
    int index = 0;
    for (int i = 0; i < arraySize; i++)
    {
        if (childArr[i] < pivot)
        {
            // since going from start to end of the array we can overwrite the original array itself
            childArr[index] = childArr[i];
            index++;
        }
    }

    return index;
}

/*
    close unused pipe for the child calling the method
*/
void childPipeSetUp(int id)
{
    for (int i = 0; i < K; i++)
    {
        if (i != id)
        {
            // close all unused pipes for the corresponding parent-child
            close(parent_fd[i][0]);
            close(parent_fd[i][1]);
            close(child_fd[i][0]);
            close(child_fd[i][1]);
        }
        else
        {
            // close the appropriate pipes for the child-parent for this id
            close(child_fd[i][0]);
            close(parent_fd[i][1]);
        }
    }
}

/*
    pause the program and prompt user to press enter to continue
*/
void halt()
{
    printf("\nPress enter to continue.");
    getchar();
}