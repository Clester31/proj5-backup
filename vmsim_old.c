// i should probably do this in C

// vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>
// only use r for NRU

// Physical memory: num frames * 2kb (2^11 bytes)
// Virtual memory: set at 2^32 bytes (32-bit addr space)

// page table: 1 entry for every page in virtual memory
// 2^21 page table entries in virtual memory

// Only evict when the page table fills up

// Page table should be 2^21 length array

// Trace file:
//     Split first letter
//     M is 2 of I->L->s
//     Dont need to worry about size
//     convert the hex into 32-bit binary (uint 32)
//     bottom 11 bits are the offset
//     other 21 bits are the page number (right shify by 11 bits to only get the page number)
//         convert hex into int and shift by 11 bits -> PTE[page #] virtual page

//     malloc(sizeof(PTE) * 2^21)
    
// Page table entry has...
//     (BOOL) Valid bit -> 0 = not mapped in physical memory, 1 means we are mapped in physical memory (init to 0)
//     (BOOL) reference bit -> 1 = page has been accesses at all since added (init to 0)
//         if anything happens (I L S M) at the addr, set it to 1
//     (BOOL) dirty -> 1 means we've done a save or modify on that page

// go through every line of the memory file and simulate that specific address
// I and L don't modify the state of the physical memory (just set ref to 1)
// S and M sets the dirty bit to 1
    // so when we have to evict, increment a rights to disc variable

// page# = VA/page size

// upper 21 bits are the index in the page table

// for each line
    // read operation (I L S M)
    // read memory address
    // calculate page number, and do something based on result
        // hit, page fault – no eviction, page fault – evict clean, page fault – evict dirty
// Print out statistics

// Numframes is the upper limit for physical pages
    // when you hit your limit, evict something
// frames how many pages have valid bits of 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

typedef struct _pte {
    int valid_bit;
    int ref_bit;
    int dirty_bit;
    uint32_t frame_num;
} pte;

// nodes for the clock algorithm
typedef struct _clock_node {
    pte *node_info;
    struct _clock_node *next;
    struct _clock_node *prev;
} clock_node;

typedef struct _frame_node {
    pte *node_info;
    struct _frame_node *next;
} frame_node;

// Global variables for statistics
int total_memory_accesses = 0;
int total_page_faults = 0;
int total_writes_to_disk = 0;
int num_pt_leaves = 0;

int numframes; int curframes = 0;
char* alg;
int refresh;
char* trace;

int main(int argc, char *argv[]) {
    FILE* trace_file;

    // Make loop that iterates over argv
    // r is a value not a boolean

    int i;
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-n") == 0) {
            if(++i < argc) {
                numframes = atoi(argv[i]);
            } else {
                 printf("Usage: vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>\n");
            }
        } else if(strcmp(argv[i], "-a") == 0) {
            if(++i < argc) {
                alg = argv[i];
            } else {
                 printf("Usage: vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>\n");
            }
        } else if(strcmp(argv[i], "-r") == 0) {
            if(++i < argc) {
                refresh = atoi(argv[i]);
            } else {
                 printf("Usage: vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>\n");
            }
        } 
    }
    trace = argv[i - 1];

    trace_file = fopen(trace, "r");
    if(trace_file == NULL) {
        perror("error opening file");
        printf("Usage: vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>\n");
        exit(EXIT_FAILURE);
    }

    pte* page_table = malloc(sizeof(pte) * pow(2, 21)); 

    frame_node *fn_head = NULL;

    clock_node *head = NULL;
    clock_node *tail = NULL;
    //printf("page table:%ld\npage table entry:%ld\n", sizeof(page_table), sizeof(pte));

    char buf[32];
    while (fgets(buf, 32, trace_file) != NULL) {
        //sleep(1);
        if (buf[0] == '=') {
            continue;
        }
        char *inst = strtok(buf, " ");
        char *addr_str = strtok(NULL, ",");
        if (addr_str != NULL) {
            // Remove leading space if present
            if (addr_str[0] == ' ') {
                addr_str++;
            }
            uint32_t addr = (uint32_t)strtoul(addr_str, NULL, 16); 
            uint32_t page_num = (addr>>11);
            //printf("\ninstruction: %s | address: %u, page#: %u \n", inst, addr, page_num);
            if(page_table[page_num].valid_bit == 1) { // hit (page is present)
                page_table[page_num].ref_bit = 1;
            } else { // page fault (page is not present)
                if(page_num != 0) {
                    //printf("PAGE FAULT: page is not present at #%u\n", page_num);
                    total_page_faults++;
                    if(curframes != numframes) {
                        frame_node *fn_new = (frame_node*)(sizeof(frame_node));
                        fn_new->node_info->frame_num = page_num;
                        frame_node *cur = fn_head;
                        if(cur == NULL) {
                            fn_head = fn_new;
                        } else {
                            while(cur->next != NULL) {
                                cur = cur->next;
                            } 
                            cur->next = fn_new;
                        }
                        curframes++;
                    }
                }
                // if we are using clock, lets add to our circular linked list
                if(strcmp(alg, "clock") == 0) {
                    clock_node *cur = (clock_node*)malloc(sizeof(clock_node));
                    //cur->node_info->frame_num = frames[curframes];
                    cur->next = head;
                    // if head is null set the next of cur to be cur
                    if(head == NULL) {
                        cur->next = cur;
                        head = tail;
                    } else {
                        clock_node *temp = head;
                        while(temp->next != head) {
                            temp = temp->next;
                        }
                        temp->next = cur;
                    }
                    head = cur;
                }
            }
            if(curframes >= numframes) { // page fault (out of frames)
                // we've reached our frame limit. Lets evict
                //printf("frames are full. evicting...\n");
                total_page_faults++;
                if(strcmp(alg, "opt") == 0) {
                    
                } else if(strcmp(alg, "clock") == 0) {
                    // set vlaid and ref to 1 when we load instruction
                    // first node in our clock will point to first frame
                    // keep adding nodes to circular queue
                    // what happens when we run out of frames?
                        // check if the head has a ref of 1
                        // set ref to 0 and head to head.next
                        // if ref is now 0, then we evict the node, add the new one
                            // have some sort of temp to deal with eviction
                    if(head->node_info->ref_bit == 1) {
                        head->node_info->ref_bit = 0;
                        head = head->next;
                    } else if(head->node_info->ref_bit == 0) {
                        clock_node *temp_evict = head;
                        
                    }
                                        
                } else if (strcmp(alg, "nru") == 0) {
                    printf("not implemented\n");
                    // do not set your refresh rate very low
                    // when we get a hit, set ref to 1
                    // once we run out of frames, evict the one that is neither referenced or dirty
                        // have some sort of priority system that goes as
                            // 1 - !ref !dirty
                            // 2 - !ref dirty
                            // 3 - ref !dirty
                            // 4 - ref dirty
                        // on refresh set every vaild page's ref to 0
                } else {
                    perror("invalid algorithm specified\n");
                    exit(EXIT_FAILURE);
                }
                // eviction part (imagine it exists)
                curframes--;
            }
            // get page number
            // basically, we start with our valid at 0 -> increase frame count -> set valid to 1
                // then, when we hit that frame again, we get a hit

            if(strcmp(inst, "I") == 0 || strcmp(inst, "L") == 0) { // Instruction Fetch
                page_table[page_num].valid_bit = 1;
                page_table[page_num].ref_bit = 1;
                page_table[page_num].dirty_bit = 0;
            } else if (strcmp(inst, "S") == 0 || strcmp(inst, "M") == 0) { // Store
                page_table[page_num].valid_bit = 1;
                page_table[page_num].ref_bit = 1;
                page_table[page_num].dirty_bit = 1;
            } 

            total_memory_accesses++;
            
            //printf("page #%d inserted at address %d\n", total_memory_accesses, page_num);
        }
    }

    fclose(trace_file);

    printf("Algorithm: %s\n", alg);
    printf("Number of frames: %d\n", numframes);
    printf("Total memoroy accesses: %d\n", total_memory_accesses);
    printf("Total page faults: %d\n", total_page_faults);
    printf("Total writes to disk: %d\n", total_writes_to_disk);
    printf("Number of page table leaves: %d\n", num_pt_leaves);
    printf("Total size of page table: %f\n", pow(2, 32));

    free(page_table);
}