#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include "interface.h"

// Declare your own data structures and functions here...

enum buddy_status
{
    AVAILABLE = 0, 
    OCCUPIED = 1,  
};

typedef struct Buddy{      /* Node Data Structure for buddy allocation */        
    enum buddy_status status;             
    void* addr;             
    struct Buddy* next;     
}Buddy;

typedef struct Free_List{  /* Linked List Data Structure for buddy allocation */
    int size;              // Size in Bytes 
    int num_available;     // Available Buddy Nodes    
    Buddy* head;
}Free_List;

typedef struct Slab{       /* Linked List nested within another Linked List (Cache) with nodes being slabs */
    int utilization;       // 0 = empty, N_OBJS_PER_SLAB = FULL.
    int* bits;             // Bitmap Array of (ints) what is allocated and what is free in the Slab 
    void* addr;            // Address of where slab starts
    struct Slab* next;     // Next Slab
}Slab;

typedef struct Cache{      /*Linked List Structure for Slabs of a Specific Size*/
    int size;              // Cache size in Bytes
    int slab_size;         // Slab object size    
    int num_slabs;         // Total number of Slabs in the Cache 
    int slabs_used;        // Number of objects in slab.
    struct Slab* head;     // First Slab in the Cache
    struct Cache* next;    // Next Cache
}Cache;

typedef struct Allocator{   /* Allocator Data structure to preserve parameters from main.c as opposed */
    int mem_size;          
    enum malloc_type type;
    void* start_of_memory;
}Allocator;

// Declaring Global Variables
Allocator* allocator;   //Memory Allocator
Cache* c1;              // Primary Cache for slabs
Free_List* buddy_list;  // Primary List for Buddy  


// Function Prototypes
void* malloc_buddy(int size);
Cache* malloc_cache(int slab_size);
void free_slab(Cache* cache, Slab* slab);
void free_buddy(void* ptr);
Buddy* find_buddy(Buddy* buddy, void* addr);
void remove_buddy(Buddy* buddy_1, Buddy* buddy_2, int index);
void allocate_rmdr(int index1, int index2, void* addr);


#endif
