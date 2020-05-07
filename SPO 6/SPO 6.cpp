#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <iostream>

using namespace std;

const int MAX_POOLS = 1000;
const long BUF_SIZE = 104888320;

static char ALLBUF[BUF_SIZE];             
static unsigned long AVAIBLE = BUF_SIZE; 
static char* pools[MAX_POOLS];              
static unsigned int POOLS_COUNT = 1;        
static unsigned int pools_size[MAX_POOLS]; 

static char* blocks[MAX_POOLS];  
static unsigned int BLOCKS_COUNT = 0;  
static unsigned int block_size[MAX_POOLS]; 
static unsigned int RIGHT_BLOCK;  


static int ARS_ALLOC_ERR = 0;   

#define NO_MEMORY 1
#define BLOCK_NOT_FOUND 2


void allocationInitialisation(void);   
char* Malloc(unsigned long Size); 
int Free(char*);                  
int Defragmentation(void);               

void allocationInitialisation(void)
{
    pools[0] = ALLBUF; 
    pools_size[0] = AVAIBLE;
}

char* Malloc(unsigned long Size)
{
    unsigned long int i, k;
    char* p;

    if (Size > AVAIBLE)
    {
        ARS_ALLOC_ERR = NO_MEMORY;
        return 0;
    }
     
    p = 0;
    
    for (i = 0; i < POOLS_COUNT; ++i)
        if (Size <= pools_size[i])
        {
            p = pools[i]; 
            k = i;      
            break;
        }

    if (!p)  
    {
        ARS_ALLOC_ERR = NO_MEMORY;
        return 0;
    }

    blocks[BLOCKS_COUNT] = p;  
    block_size[BLOCKS_COUNT] = Size;
    ++BLOCKS_COUNT;
    ++RIGHT_BLOCK;
    pools[k] = (char*)(p + Size + 1);  
    pools_size[k] = pools_size[k] - Size;  

    AVAIBLE -= Size;  
    return p;
}

int Free(char* block)
{
    unsigned int i, k;
    char* p = 0;
    for (i = 0; i < RIGHT_BLOCK; ++i)
        if (block == blocks[i])
        {
            p = blocks[i];
            k = i;
            break;
        }
    if (!p)
    {
        ARS_ALLOC_ERR = BLOCK_NOT_FOUND;
        return BLOCK_NOT_FOUND; 
    }

    blocks[k] = 0;
    --BLOCKS_COUNT;
    pools[POOLS_COUNT] = block; 
    pools_size[POOLS_COUNT] = block_size[k];
    ++POOLS_COUNT;
    AVAIBLE += block_size[k]; 

    return 0;
}

int Defragmentation(void)
{
    unsigned int i, k;
    char* p = ALLBUF;
    char* t, * tmp;

    for (i = 0; i < RIGHT_BLOCK; ++i)
    {
        t = blocks[i];
        if (t == ALLBUF)
        {
            p = (char*)(blocks[i] + block_size[i] + 1);
            continue;
        }
        tmp = p;
        for (k = 0, t = blocks[i]; k < block_size[i]; ++k) 
            *p++ = *t++;
        blocks[i] = tmp;
    }

    POOLS_COUNT = 1;
    pools[0] = p;
    AVAIBLE = BUF_SIZE - (unsigned long)(p - ALLBUF);
    pools_size[0] = AVAIBLE;
    RIGHT_BLOCK = 0;
    return 0;
}

int main() {

	allocationInitialisation();
    char* ptr0 = Malloc(11);
	Free(ptr0);
	char* ptr2 = Malloc(12);
	Free(ptr2);
	char* ptr4 = Malloc(9);
    char* ptr5 = Malloc(7);
	Defragmentation();
	getchar();
	return 0;

}