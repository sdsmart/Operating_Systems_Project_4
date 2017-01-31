// === INCLUDES ===
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// === CONSTANTS ===
#define MAX_QUEUE_SIZE 2000
#define MEMORY_SIZE 2000
#define FREE_MEM -111
#define ALLOC_MEM -999
#define TRUE 1
#define FALSE 0

// === STRUCTURES ===
typedef struct {

    int type;
    int size;
    int begin_time;
    int duration;

} Event;

struct Node {

    int address;
    struct Node* next;
    struct Node* prev;

};

typedef struct {

    int now;
    int num_allocations;
    int num_releases;
    int num_free_blocks;
    int num_allocated_blocks;
    int total_size_free_blocks;
    int total_size_allocated_blocks;
    Event final_allocation;

} Statistics;

// === GLOBAL VARIABLES ===
Event event_queue[MAX_QUEUE_SIZE];
int queue_length = 0;
int memory[MEMORY_SIZE];
int free_start = 0;
struct Node* sentinal;
Statistics stats;

// === FUNCTION DECLARATIONS ===
void init();
void print_stats(int final);

// Queue Functions
void init_event_queue();
Event deq();
void enq(Event e);
void print_event_queue();

// Free List Functions
void init_free_list();
void print_free_list();
void update_free_list(int old_address, int new_address);
void add_to_free_list(int address);
void remove_from_free_list(int address);

// Memory Management Functions
void init_memory();
void print_memory();
int allocate(Event e);
void release(Event e);
void release_case_1(Event e, int ablock_size);
void release_case_2(Event e, int ablock_size, int block_at_end);
void release_case_3(Event e, int ablock_size);
void release_case_4(Event e, int ablock_size, int block_at_end);
int rng(int max);

// === PROGRAM BEGINS HERE ===
int main() {

    init();

    Event e;

    while (TRUE) {

        e = deq();
        stats.now = e.begin_time;

        if (e.type >= 0) {

            release(e);
            stats.num_releases++;

        } else {

            int alloc_address = allocate(e);
            stats.num_allocations++;

            Event release_event;
            release_event.type = alloc_address;
            release_event.size = 0;
            release_event.begin_time = stats.now + e.duration;
            release_event.duration = 0;

            enq(release_event);

            Event new_process;
            new_process.type = -1;
            new_process.size = rng(100);
            new_process.begin_time = stats.now + rng(60);
            new_process.duration = rng(250);

            enq(new_process);
        }

        if (stats.num_allocations > 4000) {
            stats.final_allocation = e;
            print_stats(TRUE);
            break;
        }

        if ((stats.num_allocations % 50) == 0) {
            print_stats(FALSE);
        }
    }

    return 0;
}

// === FUNCTION DEFINITIONS ===
// Initializes the program.
void init() {

    srand(time(NULL));

    init_event_queue();
    init_memory();
    init_free_list();

    stats.now = 0;
    stats.num_allocations = 0;
    stats.num_releases = 0;
    stats.num_free_blocks = 1;
    stats.num_allocated_blocks = 0;
    stats.total_size_free_blocks = 1996;
    stats.total_size_allocated_blocks = 0;
}

// Prints the relevant memory management statistics.
void print_stats(final) {

    if (final) {
        printf("\x1b[31mfinal statistics\x1b[0m:\n\n");
    } else {
        printf("\x1b[34mstatistics\x1b[0m:\n\n");
    }

    printf("simulation time: \x1b[36m%d\x1b[0m\n", stats.now);
    printf("number of allocations: \x1b[36m%d\x1b[0m\n", stats.num_allocations);
    printf("number of releases: \x1b[36m%d\x1b[0m\n", stats.num_releases);
    printf("total number of free blocks: \x1b[36m%d\x1b[0m\n", stats.num_free_blocks);
    printf("total number of allocated blocks: \x1b[36m%d\x1b[0m\n", stats.num_allocated_blocks);
    printf("total size of free blocks: \x1b[36m%d\x1b[0m\n", stats.total_size_free_blocks);
    printf("total size of allocated blocks: \x1b[36m%d\x1b[0m\n", stats.total_size_allocated_blocks);

    int average_free_block_size = 0;
    if (stats.num_free_blocks == 0) {
        average_free_block_size = 0;
    } else {
        average_free_block_size = stats.total_size_free_blocks / stats.num_free_blocks;
    }
    printf("average free block size: \x1b[36m%d\x1b[0m\n", average_free_block_size);

    int average_size_allocated_block = 0;
    if (stats.num_allocated_blocks == 0) {
        average_size_allocated_block = 0;
    } else {
        average_size_allocated_block = (stats.total_size_allocated_blocks / stats.num_allocated_blocks);
    }    
    printf("average allocated block size: \x1b[36m%d\x1b[0m\n", average_size_allocated_block);

    struct Node* n = sentinal;
    int free_block_size = 0;    
    int num_fb_greater_asa = 0;
    while (TRUE) {
        n = n->next;

        if (n->address == 0) {
            break;
        }

        free_block_size = (memory[n->address] / 4);

        if (free_block_size >= average_size_allocated_block) {
            num_fb_greater_asa++;
        }
    }

    printf("number of requests greater than the average allocated block size that could be met: \x1b[36m%d\x1b[0m\n", num_fb_greater_asa);

    double percent_free = 0.0;
    if (stats.total_size_free_blocks == 0) {
        percent_free = 0.0;
    } else {
        percent_free = (100 * (((double)num_fb_greater_asa * average_size_allocated_block) / stats.total_size_free_blocks));
    }

    printf("percentage of free memory that could not hold a request greater than the average allocated block size: \x1b[36m%.2f%c\x1b[0m\n",
          percent_free, 37);

    if (final) {
        printf("\nsize of each free block:\n\n");

        n = sentinal;
        while (TRUE) {
            n = n->next;

            if (n->address == 0) {
                break;
            }

            printf("address: \x1b[35m%d\x1b[0m size: \x1b[35m%d\x1b[0m\n", n->address, (memory[n->address] / 4));
        }

        printf("\nsize of allocation that could not be honored: \x1b[36m%d\x1b[0m\n", stats.final_allocation.size);
    }

    printf("\n");
}

// ---------- Queue Functions ----------
// Initializes the event_queue.
void init_event_queue() {

    Event initial_event;
    initial_event.type = -1;
    initial_event.size = rng(100);
    initial_event.begin_time = 0;
    initial_event.duration = rng(250);

    enq(initial_event);
}

// Dequeues the next Event from the event_queue.
Event deq() {

    if (queue_length == 0) {
        printf("warning: dequeueing an empty queue\n\n");

        Event empty;
        empty.type = -99;

        return empty;
    }

    Event temp = event_queue[0];

    int i;
    for (i = 1; i < queue_length; i++) {
        event_queue[i-1] = event_queue[i];
    }

    queue_length--;

    return temp;
}

// Enqueues the given Event into the event_queue
// based on the priority of the given event.
//
// The priority of a given event is dependent on the arrival_time
// field of the event. The later the arrival_time value, the lower
// the priority.
void enq(Event e) {

    if (queue_length == 0) {
        event_queue[0] = e;
        queue_length++;
        return;
    }

    int i;
    for (i = (queue_length - 1); i >= 0; i--) {
        if (event_queue[i].begin_time > e.begin_time) {
            event_queue[i + 1] = event_queue[i];
        } else {
            break;
        }
    }
    event_queue[i + 1] = e;

    queue_length++;
}

// Prints the contents of the event_queue.
void print_event_queue() {

    printf("event_queue contents:\n\n");

    if (queue_length == 0) {
        printf("empty\n\n");
        return;
    }

    int i;
    for (i = 0; i < queue_length; i++) {
        printf("[%d]: type: %d size: %d begin_time: %d duration: %d\n",
                    i, event_queue[i].type, event_queue[i].size, 
                    event_queue[i].begin_time, event_queue[i].duration);
    }

    printf("\n");
}
// ---------- End Queue Functions ----------

// ---------- Free List Functions ----------
// Initializes the free list to hold the sentinal and the first free block.
void init_free_list() {

    sentinal = (struct Node*) malloc(sizeof(struct Node));
    sentinal->address = 0;

    struct Node* first_block = (struct Node*) malloc(sizeof(struct Node));
    first_block->address = 4;

    sentinal->next = first_block;
    sentinal->prev = first_block;

    first_block->next = sentinal;
    first_block->prev = sentinal;
}

// Prints the contents of the free list
void print_free_list() {

    int list_printed = FALSE;
    struct Node* current = sentinal;
    int i = 0;

    printf("free_list contents:\n\n");

    while (!list_printed) {

        printf("[%d]: address: %d next: %d prev: %d\n", i, current->address, current->next->address,
                    current->prev->address);

        current = current->next;

        if (current->address == 0) {
            list_printed = TRUE;
        }

        i++;
    }

    printf("\n");
}

// Updates a node in the free list to reflect the changed address.
void update_free_list(int old_address, int new_address) {

    struct Node* n = sentinal;

    // Finding the node of interest
    while (TRUE) {
        if (n->address == old_address) {
            break;
        }

        n = n->next;

        if (n->address == sentinal->address) {
            printf("error in update_free_list: address not found\n\n");
            exit(-1);
        }
    }

    n->address = new_address;
    memory[n->address + 1] = n->next->address;
    memory[n->address + 2] = n->prev->address;

    memory[n->next->address + 2] = n->address;
    memory[n->prev->address + 1] = n->address;
}

// Adds a new node at the end of the free list.
void add_to_free_list(int address) {

    struct Node* new = (struct Node*) malloc(sizeof(struct Node));
    new->address = address;
    new->next = sentinal;
    new->prev = sentinal->prev;
    memory[new->address + 1] = sentinal->address;
    memory[new->address + 2] = sentinal->prev->address;

    sentinal->prev->next = new;
    memory[sentinal->prev->address + 1] = new->address;

    sentinal->prev = new;
    memory[sentinal->address + 2] = new->address;

    stats.num_free_blocks++;
}

// Removes the node with the given address from the free list.
void remove_from_free_list(int address) {

    struct Node* n = sentinal;

    // Finding the node of interest
    while (TRUE) {
        if (n->address == address) {
            break;
        }

        n = n->next;

        if (n->address == sentinal->address) {
            printf("error in remove_from_free_list: address not found\n\n");
            exit(-1);
        }
    }

    n->next->prev = n->prev;
    memory[n->next->address + 2] = n->prev->address;

    n->prev->next = n->next;
    memory[n->prev->address + 1] = n->next->address;

    int size = memory[n->address] / 4;
    memory[n->address] = FREE_MEM;
    memory[n->address + 1] = FREE_MEM;
    memory[n->address + 2] = FREE_MEM;
    memory[n->address + size - 1] = FREE_MEM;

    stats.num_free_blocks--;
}
// ---------- End Free List Functions ----------

// ---------- Memory Management Functions ----------
// Initializes the memory block to have a sentinal and one large
// free block.
void init_memory() {

    // Filling memory with FREE_MEM
    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = FREE_MEM;
    }

    // Initializing sentinal
    // size = 0 | pre-use = 0 | use = 1
    memory[0] = 1;
    // Forward-link to next free block
    memory[1] = 4;
    // Back-link to previous free block
    memory[2] = 4;
    // size = 0
    memory[3] = 0;

    // Initializing first (and only) free block
    // size = MEMORY_SIZE - 5 | pre-use = 1 | use = 0
    memory[4] = 4 * (MEMORY_SIZE - 4) + 2 * 1;
    // Forward-link to next free block
    memory[5] = 0;
    // Back-link to previous free block
    memory[6] = 0;
    // size = MEMORY_SIZE - 5
    memory[MEMORY_SIZE - 1] = (MEMORY_SIZE - 4);
}

// Prints the contents of memory.
void print_memory() {

    int printed_A_F = FALSE;

    printf("memory contents:\n\n");
    int i;
    for (i = 0; i < MEMORY_SIZE; i++) {
        if (memory[i] == FREE_MEM && !printed_A_F) {
            printf("[%d]: FREE BLOCK\n", i);
            printed_A_F = TRUE;
        } else if (memory[i] == ALLOC_MEM && !printed_A_F) {
            printf("[%d]: ALLOCATED BLOCK\n", i);
            printed_A_F = TRUE;
        } else if (memory[i] != FREE_MEM && memory[i] != ALLOC_MEM) {
            printf("[%d]: %d\n", i, memory[i]);
            printed_A_F = FALSE;
        }
    }
    printf("\n");
}

// Allocates the given event to memory, if possible, and returns the address
// at which the process was allocated.
int allocate(Event e) {

    if (e.type != -1) {
        printf("error during allocation: event is of incorrect type\nexiting due to error\n\n");
        exit(-1);
    }

    if (stats.num_releases <= 40) {
        printf("attempting to \x1b[34mallocate\x1b[0m process with the following information:\n\n");
        printf("type: \x1b[36m%d\x1b[0m\nsize: \x1b[36m%d\x1b[0m\nbegin_time: \x1b[36m%d\x1b[0m\nduration: \x1b[36m%d\x1b[0m\n\n",
                e.type, e.size, e.begin_time, e.duration);
    }

    int fblock_size = 0;
    int new_fblock_size = 0;
    int initial_free_start = free_start;
    int ablock_size = 0;
    int next_free_start = 0;
    int ablock_address = 0;
    int i = 0;

    if (e.size < 3) {
        e.size = 3;
    }

    while (TRUE) {

        fblock_size = memory[free_start] / 4;

        if (fblock_size >= (e.size + 1)) {

            if (fblock_size <= (e.size + 5)) {

                ablock_size = fblock_size;
                if ((free_start + ablock_size) < MEMORY_SIZE) {
                    memory[free_start + ablock_size] = memory[free_start + ablock_size] + 2;
                }
                next_free_start = memory[free_start + 1];
                remove_from_free_list(free_start);

            } else {

                ablock_size = e.size + 1;
                new_fblock_size = fblock_size - ablock_size;
                memory[free_start + ablock_size] = (4 * new_fblock_size) + (2 * 1) + 0;
                memory[free_start + fblock_size - 1] = new_fblock_size;
                update_free_list(free_start, (free_start + ablock_size));
                next_free_start = free_start + ablock_size;
            }

            memory[free_start] = (4 * ablock_size) + (2 * 1) + 1;
            ablock_address = free_start;

            for (i = (ablock_address + 1); i < (ablock_address + ablock_size); i++) {
                memory[i] = ALLOC_MEM;
            }

            free_start = next_free_start;

            if (stats.num_releases <= 40) {
                printf("\x1b[33mallocation successful\x1b[0m\n\n");
            }

            stats.num_allocated_blocks++;
            stats.total_size_free_blocks -= ablock_size;
            stats.total_size_allocated_blocks += ablock_size;

            return ablock_address;
        }

        free_start = memory[free_start + 1];

        if (free_start == initial_free_start) {
            printf("no free blocks were large enough to allocate process\nending simulation...\n\n");
            exit(0);
        }
    }
}

// Releases a given event from memory and adjusts free blocks appropriately
void release(Event e) {

    if (e.type < 0) {
        printf("error during release: event is of incorrect type\nexiting due to error\n\n");
        exit(-1);
    }

    if (stats.num_releases <= 40) {
        printf("attempting to \x1b[31mrelease\x1b[0m process with the following information:\n\n");
        printf("address: \x1b[36m%d\x1b[0m\nsize: \x1b[36m%d\x1b[0m\nbegin_time: \x1b[36m%d\x1b[0m\nduration: \x1b[36m%d\x1b[0m\n\n",
                e.type, e.size, e.begin_time, e.duration);
    }

    int ablock_size = (memory[e.type] / 4);

    int block_at_end = FALSE;

    int prev_block_allocated = ((memory[e.type] % 4) / 2);
    int next_block_allocated = 0;
    if ((e.type + ablock_size) >= MEMORY_SIZE) {
        next_block_allocated = 1;
        block_at_end = TRUE;
    } else {
        next_block_allocated = (memory[e.type + ablock_size] % 2);
    }

    if (prev_block_allocated && !next_block_allocated) {

        release_case_1(e, ablock_size);

    } else if (!prev_block_allocated && next_block_allocated) {

        release_case_2(e, ablock_size, block_at_end);

    } else if (!prev_block_allocated && !next_block_allocated) {

        release_case_3(e, ablock_size);

    } else {

        release_case_4(e, ablock_size, block_at_end);
    }

    if (stats.num_releases <= 40) {
        printf("\x1b[33mrelease successful\x1b[0m\n\n");
    }

    stats.total_size_free_blocks += ablock_size;
    stats.total_size_allocated_blocks -= ablock_size;
    stats.num_allocated_blocks--;
}

// Releases a given event when the previous block is allocated
// and the next block is free.
void release_case_1(Event e, int ablock_size) {

    if (free_start == (e.type + ablock_size)) {
        free_start = 0;
    }

    int fblock_size = (memory[e.type + ablock_size] / 4);
    int new_fblock_size = fblock_size + ablock_size;

    memory[e.type] = (4 * new_fblock_size) + (2 * 1) + 0;
    memory[e.type + new_fblock_size - 1] = new_fblock_size;

    update_free_list((e.type + ablock_size), e.type);

    int i;
    for (i = (e.type + 3); i < (e.type + new_fblock_size - 1); i++) {
        memory[i] = FREE_MEM;
    }
}

// Releases a given event when the previous block is free
// and the next block is allocated.
void release_case_2(Event e, int ablock_size, int block_at_end) {

    int new_fblock_size = (memory[e.type - 1] + ablock_size);

    int location_of_prev_fblock = (e.type - memory[e.type - 1]);

    memory[location_of_prev_fblock] = (4 * new_fblock_size) + (2 * 1) + 0;
    memory[location_of_prev_fblock + new_fblock_size - 1] = new_fblock_size;

    if (!block_at_end) {
        memory[e.type + ablock_size] = memory[e.type + ablock_size] - 2;
    }

    int i;
    for (i = (location_of_prev_fblock + 3); i < (location_of_prev_fblock + new_fblock_size - 1); i++) {
        memory[i] = FREE_MEM;
    }
}

// Releases a given event when both the previous and next
// block are free.
void release_case_3(Event e, int ablock_size) {

    if (free_start == (e.type + ablock_size)) {
        free_start = 0;
    }

    int size_of_next_fblock = (memory[e.type + ablock_size] / 4);

    remove_from_free_list(e.type + ablock_size);

    int address_of_prev_fblock = (e.type - memory[e.type -1]);

    int new_fblock_size = (memory[address_of_prev_fblock] / 4) + ablock_size
                            + size_of_next_fblock;

    memory[address_of_prev_fblock] = (4 * new_fblock_size) + (2 * 1) + 0;
    memory[address_of_prev_fblock + new_fblock_size - 1] = new_fblock_size;

    int i;
    for (i = (address_of_prev_fblock + 3); i < (address_of_prev_fblock + new_fblock_size - 1); i++) {
        memory[i] = FREE_MEM;
    }
}

// Releases a given event when both the previous and next
// blocks are allocated.
void release_case_4(Event e, int ablock_size, int block_at_end) {

    int new_fblock_size = ablock_size;

    memory[e.type] = (4 * new_fblock_size) + (2 * 1) + 0;
    memory[e.type + new_fblock_size - 1] = new_fblock_size;

    if (!block_at_end) {
        memory[e.type + new_fblock_size] = memory[e.type + new_fblock_size] - 2;
    }

    add_to_free_list(e.type);

    int i;
    for (i = (e.type + 3); i < (e.type + new_fblock_size - 1); i++) {
        memory[i] = FREE_MEM;
    }
}

// Generates a random number between 1 and max.
int rng(int max) {

    int random_num = ((rand() % max) + 1);
    return random_num;
}
// ---------- End Memory Management Functions ----------
