# file description

## fwa.c

This file is the Floyd-Warshall alogorithm implemented in singled theaded.

input:  
NN NP  
SN EN PW

NN: number of nodes  
NP: number of paths whose weigh will be inserted below  
SN: start node  
EN: end node  
PW: path weight

## fwaF.c

Same as above (fwa.c) with the only difference being it uses a file as input.

## fwaThread.c

This file is the Floyd-Warshall alogorithm implemented in multi theaded.

input:  
NN NP  
SN EN PW

NN: number of nodes  
NP: number of paths whose weigh will be inserted below  
SN: start node  
EN: end node  
PW: path weight

## fwaThreadF.c

Same as above (fwaThread.c) with the only difference being it uses a file as input.

## testing.c

This file contains the algorithm used to create a file that can be used as input to the programs expecting a file as input.
