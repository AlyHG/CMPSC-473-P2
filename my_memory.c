#include "my_memory.h"

// Memory allocator implementation
// Implement all other functions here...

/***********************************************************************************************************
*
*  File           : my_memory.c
*  Description    : This is the implementation of helper functions to be used
*                   by my_malloc() and my_free() for the allocation and freeing
*                   of memory.
*
*   Author         *** Aly Ghallab ***
***********************************************************************************************************/

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : malloc_buddy
// Description  : function allocates a portion of memory into a buddy node then removes the buddy out of
//                the free list. It iterates through the free list until it finds the first element that
//                the allocation size (First Fit). It is also used by slab allocator to allocate a new
//                slab into memory.
//
// Inputs       : size  - # of bytes of memory that need to be allocated.
// Outputs      : void* - pointer to buddy node after header where writing in memory can begin.
void* malloc_buddy(int size){
    int nSize = size + HEADER_SIZE;
    int index;
    int min_found_flag = 0; 
    int i = MIN_MEM_CHUNK_SIZE_EXP;
    void* ret_addr = NULL;
    
    Buddy* temp_prev = NULL;
    Buddy* temp = NULL;
    Buddy* temp2 = NULL;

    while (i <= MEMORY_SIZE_EXP){
        // If a free list with holes of the appropriate size is found
        // First Fit Allocation
        if (buddy_list[i].size >= nSize){
            
            // Flag used to indicate First Fit
            if (min_found_flag == 0){
                min_found_flag ++;
                index = i;
            }

            // If that free list has available "holes" in memory
            if (buddy_list[i].num_available > 0){
                temp = buddy_list[i].head;
                temp_prev = buddy_list[i].head;
                /* Intialize addr to last point in Memory; addr is the 
                lowest address of a 'hole' that can be allocated to */
                void* addr = allocator->start_of_memory + allocator->mem_size; 

                /* Iterate through free list of a specific 'hole' size that is 
                bigger than size parameter */ 
                while(temp != NULL){
                    // If node in Free List is available
                    // And node address is less than current address (addr)
                    if((temp->status == AVAILABLE) && (addr > temp->addr)){
                        // set current adress to the node's address. 
                        addr = temp->addr;
                        temp_prev = temp;
                    }
                    // Otherwise keep iterating.
                    temp = temp->next;
                }
                /* Removing the Free node from Free List */
                /* If the free node is the head of the list, make the head of 
                the list equal to the next node */
                if (buddy_list[i].head == temp_prev){
                    buddy_list[i].head = buddy_list[i].head->next;   // Remove temp from Linked List by reconnecting the pointers.
                }
                else{
                // Removing the node (if it isnt the head) from the free-list as it will now be occupied
                    temp2 = buddy_list[i].head;
                    while((temp2->next != NULL) && (temp_prev != temp2->next)){
                        temp2 = temp2->next;
                    }
                    if (temp_prev == temp2->next) temp2->next = temp2->next->next;
                }
                // Make a buddy node at the address.
                Buddy* buddy = (Buddy*)malloc(sizeof(Buddy));
                buddy->status = OCCUPIED;
                buddy->addr = addr;
                
                buddy_list[i].num_available --;
                buddy->next = buddy_list[index].head;
                buddy_list[index].head = buddy;
                
                ret_addr = addr + HEADER_SIZE;          // address to be returned by malloc_buddy
                addr += buddy_list[index].size;         // reassign addr to start at end of free-list
                int temp_index = index;
                allocate_rmdr(temp_index, i, addr);     // Create a sibiling node to the buddy
                break;  
            }
            // If availability == 0 then go to the next element in array.
        }
        i++;
    }
    return ret_addr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : malloc_cache
// Description  : function allocates a linked lists of slabs (aka a Cache).  
//
// Inputs       : slab_size  - # of bytes of memory occupied by each slab.
// Outputs      : void* - pointer to cache struct from which it can be accessed.
Cache* malloc_cache(int slab_size){
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->slabs_used = 0;
    cache->num_slabs = 0;
    cache->size = slab_size * N_OBJS_PER_SLAB;
    cache->head = NULL;
    cache->next = c1;
    cache->slab_size = slab_size; 
    return cache;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : free_slab
// Description  : function deallocates a given slab in a given cache.  
//
// Inputs       : *cache  - pointer to the cache of where a slab is located.
//              : *slab   - pointer to slab that needs to be deallocated. 
// Outputs      : void
void free_slab(Cache* cache, Slab* slab) {
    Slab* temp = cache->head;
    //If slab to be removed is at the head of the cache
    if(temp == slab) {
        cache->head = temp->next;
    }
    //If the slab to be removed is not the head of the cache
    else{
        //Iterate through cache until slab is found
        while(temp->next != slab){
            temp = temp->next;
        }
        //When slab is found remove it from list
        temp->next = slab->next;
        // slab->next = NULL;
    }
    cache->num_slabs--;
    free_buddy(slab->addr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : free_buddy
// Description  : function deallocates a given buddy, and coalesces if possible 
//
// Inputs       : *ptr  - pointer to the address of where the buddy is located + Header Size offset.
// Outputs      : void
void free_buddy(void* ptr){
    
    ptr = ptr - HEADER_SIZE;                    // Pointer Arithmetic to account for header
    int exponent = MIN_MEM_CHUNK_SIZE_EXP;
    int index;                                  // Index of list buddy is located at.

    /*Add future Comments here regarding each of the temp iterators*/
    Buddy* target;
    Buddy* remove;
    Buddy* temp;
    Buddy* temp2;
    int flag_x = 0;
    int flag_y = 0;
    int flag_z = 0;


    //Iterate through lists of available buddies to find the target.
    while((MEMORY_SIZE_EXP >= exponent) && (0 == flag_x)) { 
        temp = buddy_list[exponent].head;
        while((temp != NULL) && (0 == flag_x)) {
            if(temp->addr == ptr) {
                index = exponent;
                target = temp;
                target->status = AVAILABLE;
                flag_x ++;
            }else{
                temp = temp->next;
            }
        }
        exponent ++;
    }

    remove = buddy_list[index].head;  //Set temp node to head of list buddy is in.  
    remove_buddy(target, remove, index);
    while((index <= MEMORY_SIZE_EXP) && (0 == flag_y)) {
        void* addr;
        int size;
        unsigned long rmdr; // Remainder

    /*Note to self: Gather visual notes for presentation*/
        size = buddy_list[index].size; 
        rmdr = (ptr - allocator->start_of_memory) % (size * 2);    
    
        (rmdr == size)?(addr = ptr-size):(addr = ptr + size);
        
    
        temp2 = buddy_list[index].head;
        temp2 = find_buddy(temp2, addr);

        while ((temp2 != NULL) && (0 == flag_z)){
            if(addr == temp2->addr) flag_z ++;
            else temp2 = temp2->next;
        }

        if(temp2 == NULL || temp2->status == OCCUPIED){ 
            flag_y ++;
            target->next = buddy_list[index].head;
            target->addr = ptr;
            buddy_list[index].num_available ++;
            buddy_list[index].head = target;
            target->status = AVAILABLE;
        }

        else{
            remove = buddy_list[index].head;
            if(temp2 == buddy_list[index].head){
                buddy_list[index].head = buddy_list[index].head->next;
            } 
            else if(temp2 != remove->next){
                while(temp2 != remove->next){
                    remove = remove->next;
                }
            }else{
                remove->next = remove->next->next;        
            }
            if(ptr > addr) {
                ptr = addr;
            }
            buddy_list[index].num_available --;
        }
        index ++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_buddy
// Description  : function finds buddy (node) within a free-list (linked list).  
//
// Inputs       : *buddy    - pointer to node that is being seeked.
//              : *addr     - pointer to free-list to be iterated through.
// Outputs      :           *buddy pointer to buddy if it was found in the list.
//                           Otherwise, return NULL
Buddy* find_buddy(Buddy* buddy, void* addr) {
    int flag = 0;
    while(buddy != NULL && flag == 0) {
        (addr==buddy->addr)?(flag ++):(buddy = buddy->next);
    }
    return buddy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : remove_buddy
// Description  : function finds buddy (node) within a free-list (linked list) for removal typically after a
//                buddy is allocated and is no longer free.  
//
// Inputs       : *buddy_1     - pointer to node (buddy) that is being seeked for removal.
//              : *buddy_2     - pointer to a different node in the list that is being used as a temp
//                               iterator.
//              : index        - integer representing index of 2 with offset of MIN_MEM_CHUNK_SIZE_EXP
//                               that determines which list to iterate through.
// Outputs      : void
void remove_buddy(Buddy* buddy_1, Buddy* buddy_2, int index) {
    //If node to be removed is the head of the list then make the next node the head.
    if(buddy_1 == buddy_list[index].head) {
        buddy_list[index].head = buddy_list[index].head->next;
        return;
    }
    //If the node is not the head
    else if(buddy_1 != buddy_list[index].head) {
        //Iterate through the list
        while(buddy_2->next != buddy_1 && buddy_2->next != NULL) {
            buddy_2 = buddy_2->next;
        }
        //If buddy 1 is found then set next pointer to the node after and disconnect buddy 1 from the list.
        buddy_2->next = buddy_2->next->next;
    }
}
    
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function     : allocate_rmdr
// Description  : function called by malloc_buddy after a buddy node is allocated, this function creates
//                a free sister node (like in a binary tree) that is available to be allocated 
//                and inserts the nodes of the free list in an index in ascending addresses
//                hence the head is an available buddy with the lowest address for that given index
//                which represents a index of 2. The function is supposed to mark the gap in memory
//                of where the free 'sibiling'/leaf buddy belongs which is between index 1 and index 2
//                starting at addr. 
//
// Inputs       : int index 1: index of free list where now occupied buddy node was just allocated.
//                int index 2: The next next occupied index
//                void* addr: address of where to begin declaring free node.
// Outputs      : void
void allocate_rmdr(int index1, int index2, void* addr){
    while(index2 > index1) {
        Buddy* free_buddy = (Buddy*)malloc(sizeof(Buddy));

        free_buddy->addr = addr;
        free_buddy->next = buddy_list[index1].head;
        free_buddy->status = AVAILABLE;
        
        buddy_list[index1].head = free_buddy;
        buddy_list[index1].num_available += 1;
    
        addr += buddy_list[index1].size;
        index1++;
    } 
}

        