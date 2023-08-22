#include "interface.h"
#include "my_memory.h"

// Interface implementation
// Implement APIs here...


/***********************************************************************************************************
*
*   File           : interface.c
*   Description    : This is the implementation of the memory allocation and freeing functions
*                  to be utilization  by the memory allocator in main.c .
*
*   Author         : *** Aly Ghallab ***
*
***********************************************************************************************************/


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : my_setup
// Description  : Initialization of variables and strucutres needed, parameters are passed in 
//                through main.c .
//
// Inputs       : *start_of_memory - pointer to the start_of_memory block
//                mem_size - size of memory to be utilization  (RAM_SIZE = MEMORY_SIZE)
//                malloc_type - type of memory allocator to be utilization 
//                 
// Outputs      : void
void my_setup(enum malloc_type type, int mem_size, void *start_of_memory)
{
    allocator = (Allocator*)malloc(sizeof(Allocator));
    allocator->start_of_memory = start_of_memory;
    allocator->mem_size = mem_size;
    allocator->type = type;
    

    buddy_list = (Free_List*)malloc(sizeof(Free_List)*(1+MEMORY_SIZE_EXP));  
    // int bytes = MIN_MEM_CHUNK_SIZE;
    
    /* Perhaps ask TA why this for loop wasnt working if cant figure it out by demo, theres 
    no reason as to why it didn't work 

    for (int i = MIN_MEM_CHUNK_SIZE_EXP; i <= MEMORY_SIZE_EXP; i++){
        buddy_list[i].size = bytes;
        buddy_list[i].num_available = 0;
        buddy_list[i].head = NULL;
        bytes *= 2;
    }
    */
    
    int bytes = MIN_MEM_CHUNK_SIZE;
    int i = MIN_MEM_CHUNK_SIZE_EXP;
    while (MEMORY_SIZE_EXP >= i){
        buddy_list[i].head = NULL;
        buddy_list[i].size = bytes;
        buddy_list[i].num_available = 0;
        bytes *= 2;
        i++;
    }
    /*  
        2^(MEMORY_SIZE_EXP) = MEMORY_SIZE. Here an array of 
        Linked Lists that tracks buddies is intialized. Each index is a power of 2.
    */
    buddy_list[MEMORY_SIZE_EXP].head = (Buddy*)malloc(sizeof(Buddy));
    buddy_list[MEMORY_SIZE_EXP].head->addr = allocator->start_of_memory;
    buddy_list[MEMORY_SIZE_EXP].head->next = NULL;
    buddy_list[MEMORY_SIZE_EXP].num_available = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : my_malloc
// Description  : This function should allocate size bytes of memory from the 
//                mem_size chunk of memory that is available for you (from the 
//                start_of_memory pointer) using the specified allocation algorithm. 
//                All the size bytes, starting from the first byte pointed to by the 
//                returned pointer, is available to it.
//
// Inputs       : size - # of Bytes
// Outputs      : (void* start_of_memory <= return ptr <= start_of_memory+mem_size) on success
//                -1 on failure
void *my_malloc(int size)
{
    switch (allocator->type){

        case MALLOC_BUDDY: 
            return malloc_buddy(size);
            break;
        
        case MALLOC_SLAB : ;
            int i = 0;
            Cache* cache;
            cache = c1;
            int nSize = size + HEADER_SIZE; 

            /* Iterate through caches until end of list of caches or a cache containing Cache of Slabs of nSize objects
            is found */
            while((cache != NULL) && (nSize != cache->slab_size)) {
                cache = cache->next;
            }
            /* If there are no Caches that satisfy the above conditions or the Cache that do are full
            then create a new cache with appropriate slab size */
            if((cache == NULL) || ((N_OBJS_PER_SLAB * cache->num_slabs) == (cache->slabs_used))) {
                c1 = malloc_cache(nSize);                                       // Create a cache of slabs of size nSize
                cache = c1;
                void* addr = malloc_buddy(c1->size);                            // address of first slab after primary cache
                Slab* new_Slab = (Slab*)malloc(sizeof(new_Slab));
                new_Slab->bits = (int*)malloc(sizeof(int) * N_OBJS_PER_SLAB);   // Instantiating Bit Array
                
                for (int j = 0; j < N_OBJS_PER_SLAB; j++){                      // Intializing Values of Bit Array
                    new_Slab->bits[j] = 0;
                }

                if(addr == NULL) {return NULL;}                                 // Error Handling

                new_Slab->next = c1->head;
                c1->head = new_Slab;
                c1->num_slabs++;
                new_Slab->addr = addr;
                new_Slab->utilization = 0;
            }

            Slab* temp_slab = cache->head;

            //While in cache Iterate to next slab the goal is to find the first slab not full
            while((temp_slab != NULL) && (N_OBJS_PER_SLAB == temp_slab->utilization)) {
                temp_slab = temp_slab->next;
            }
            //Once slab is found increment i to find how much that slab is occupied, i is the # of objects in slab.
            while((N_OBJS_PER_SLAB > i) && (temp_slab->bits[i] != 0)) {
                i++;
            }

            void* ret_addr = (i * nSize) + temp_slab->addr;          // Address after slab has been allocated for next operation
            ((int*)ret_addr)[0] = nSize;                             
            ret_addr += HEADER_SIZE;                                 // Modifying ret_addr so it returns addr + Header Offset.
            cache->slabs_used ++;                                    // Increment # of slabs in use by the Cache
            temp_slab->utilization ++;                               // Increment utilization (# of Objects) within slab
            temp_slab->bits[i] = 1;                                  // Increment bit value to indicate objects occupying slab.
            return ret_addr;                                         // return address with Header offset for next mem allocation
            break;
        
        default:
            return (int*) (-1);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : my_free
// Description  : function deallocates the memory segment being passed by the pointer. The resulting free 
//                segment will be merged with relevant neighboring holes (as per the algorithm/scheme) 
//                whenever possible to create holes of larger size. 
//
// Inputs       : *ptr - pointer to a segment of memory to be freed.
// Outputs      : void
void my_free(void *ptr)
{
    switch (allocator->type){

        case (MALLOC_BUDDY):
            return free_buddy(ptr);

        case (MALLOC_SLAB):;
            Cache* tempCache = c1;                          // temp variable to iterate through caches
            void* lAddr;                                    // last mem address occupied by slab
            ptr = ptr - HEADER_SIZE;                        // HEADER OFFSET
            int size = ((int*)ptr)[0];                      // Size of a bit (integer pointer in current context)
            
            // Iterate through list of caches to find one with appropriate slab size
            while((tempCache != NULL) && (size != tempCache->slab_size)) { 
                tempCache = tempCache->next;
            }
            Slab* temp = tempCache->head;                   // temp variable to iterate through Cache

            /* When a cache with an appropriate slab size is found, iterate through that cache until slab
            referenced by ptr is found for deallocation */ 
            while(temp != NULL && ((ptr < temp->addr) && (ptr > lAddr))) { 
                lAddr = N_OBJS_PER_SLAB * size + temp->addr; 
                temp = temp->next;
            }
            if(temp == NULL) return;                        // If slab is already deallocated/free

            for (int i = 0; i < N_OBJS_PER_SLAB; i++){
                lAddr = (i * size) + temp->addr;            // Increment last address occupied by slab
                if (ptr == lAddr){
                    switch (temp->bits[i]){
                        case 0:                             // If slab doesnt contain objects.
                            return;
                        case 1:                             // If slab is occupied then empty it.
                            temp->utilization -- ;   
                            temp->bits[i] = 0;              // Decrement bits to show objects not occupying that part of slab
                            tempCache->slabs_used -- ;
                            break;
                    }
                }
            }
            //If slab contains no objects it can be freed.
            if(temp->utilization <= 0) {
                free_slab(tempCache, temp);
            }
            break;
    }
}
