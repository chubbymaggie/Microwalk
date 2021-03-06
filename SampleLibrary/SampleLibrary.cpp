/*
This small library contains functions to test the LeakageDetector's capabilities.
*/

/* INCLUDES */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <intrin.h>


/* IMPORTS */
extern "C" void different_branch_target(uint64_t x);
extern "C" uint64_t* select_address(uint64_t x, uint64_t *addr1, uint64_t *addr2);


/* GLOBALS */
const int heapMemoryTableSize = 500;
uint64_t *heapMemoryTable = NULL;
uint64_t imageMemoryTable[5] = { 123, 456, 789, 123, 456 };


/* FUNCTIONS */

__declspec(dllexport) uint32_t LeakInputBits(const unsigned char *input, int inputLength, bool randomize)
{
    // Generate some random substitution table
    uint32_t S[256];
    for(int i = 0; i < 256; ++i)
        S[i] = static_cast<uint32_t>(rand());

    // Calculate result by input-dependent access of substitution table entries
    uint32_t result = 0;
    for(int i = 0; i < inputLength; ++i)
        if(randomize)
        {
            unsigned int randomValue;
            _rdrand32_step(&randomValue);
            result += S[randomValue & 0xFF];
        }
        else
            result += S[input[i]];
    return result;
}

__declspec(dllexport) uint64_t LeakingFunction(int x)
{
    heapMemoryTable = (uint64_t *)malloc(sizeof(uint64_t) * heapMemoryTableSize);
    for(int i = 0; i < heapMemoryTableSize; ++i)
        heapMemoryTable[i] = i;

    // ExecutionFlow_BranchTakenIn1 / ExecutionFlow_BranchTakenIn2 triggered between testcase classes

    // Triggers ExecutionFlow_DifferentBranchTarget
    if(0 <= x && x < 5)
    {
        different_branch_target(x);
    }
    // Triggers MemoryAccess_DifferentAllocationSize 
    else if(105 <= x && x < 110)
    {
        uint64_t *garbage = (uint64_t *)malloc(sizeof(uint64_t) * x);
        free(garbage);
    }
    // Triggers MemoryAccess_FreedBlockNotMatching 
    else if(115 <= x && x < 120)
    {
        uint64_t **garbageArray = (uint64_t **)malloc(sizeof(uint64_t *) * 5);
        for(int i = 0; i < 5; ++i)
            garbageArray[i] = (uint64_t *)malloc(sizeof(uint64_t) * 5);

        uint64_t *addr = select_address(x - 115ull, garbageArray[0], garbageArray[1]);
        free(addr);
        for(int i = 0; i < 5; ++i)
            if(garbageArray[i] != addr)
                free(garbageArray[i]);
        free(garbageArray);
    }
    // Triggers MemoryAccess_DifferentImageMemoryReadOffset 
    else if(125 <= x && x < 130)
    {
        return imageMemoryTable[x - 125];
    }
    // Triggers MemoryAccess_DifferentImageMemoryWriteOffset 
    else if(235 <= x && x < 240)
    {
        imageMemoryTable[x - 235] = x;
    }
    // Triggers MemoryAccess_DifferentHeapMemoryReadOffset 
    else if(345 <= x && x < 350)
    {
        return heapMemoryTable[x - 345];
    }
    // Triggers MemoryAccess_DifferentHeapMemoryWriteOffset 
    else if(455 <= x && x < 460)
    {
        heapMemoryTable[x - 455] = x;
    }

    uint64_t result = heapMemoryTable[0];
    free(heapMemoryTable);
    return result;
}