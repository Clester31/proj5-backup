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
    long line_num;
} pte;

// I created three seperate linked lists for each different algorithm
// some of them have minor adjustments between eachother
typedef struct _clock_node {
    pte *node_info;
    struct _clock_node *next;
    struct _clock_node *prev;
} clock_node;

clock_node *clock_head = NULL;

typedef struct _opt_node {
    pte* node_info;
    struct _opt_node *next;
    struct _opt_node *prev;
} opt_node;

opt_node *opt_head = NULL;

typedef struct _unique_addr {
   uint32_t page_num;
   struct _unique_addr *next;
   struct _ua_info *info;
} unique_addr;

unique_addr *ua_head = NULL;

typedef struct _ua_info {
    long line_num;
    struct _ua_info *next;
} ua_info;

typedef struct _nru_node {
    pte *node_info;
    struct _nru_node *next;
    struct _nru_node *prev;
    int p;
} nru_node;

nru_node *nru_head = NULL;

// Global variables for statistics
int total_memory_accesses = 0;
int total_page_faults = 0;
int total_writes_to_disk = 0;

// arguments
int numframes; int curframes = 0;
char* alg;
int refresh;
char* trace;

// additional vars for later
int line_count = 0;
int full = 0;

void add_opt(pte*);
void add_clock(pte*);
void add_nru(pte*);
void preprocess_trace(FILE*, long*, int);

int main(int argc, char *argv[]) {
    FILE* trace_file;
    // Make loop that iterates over argv
    // this makes it easier to handle a case where we do or don't have -r
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

    // open trace file
    trace_file = fopen(trace, "r");
    if(trace_file == NULL) {
        perror("error opening file");
        printf("Usage: vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>\n");
        exit(EXIT_FAILURE);
    }

    // allocate our page table as an array of page table entries
    // we have 2^21 page table entries
    pte* page_table = malloc(sizeof(pte) * pow(2, 21));

    // how opt will work
        // create a linked list of unique page numbers
        // each time we read a page number we don't have in our list, add it
            // include the line we found it on in its info
        // if the next page # we read is already in the list:
            // add a new node to that nodes ua_info that's the next line we found it at
            // update the list so that every new line entry is the head
    // once we run the actual opt algorithm, we can just grab the max sizes of each frame
    // and then compare then to evict the node with the longest distance of lines
    if(strcmp(alg, "opt") == 0) {
        // first part is just like the main loop
        char ua_buf[32];
        while (fgets(ua_buf, 32, trace_file) != NULL) {
        // use strtok to parse our string into the instructions and page number
            char *ua_inst = strtok(ua_buf, " ");
            char *ua_addr_str = strtok(NULL, ",");
            // check if our instruction is valid AND we aren't at the end of the file
            if((strcmp(ua_inst, "I") == 0 || strcmp(ua_inst, "L") || strcmp(ua_inst, "S") == 0 || strcmp(ua_inst, "M")) && ua_addr_str != NULL && (ua_buf[0] == 'I' || ua_buf[1] == 'L' || ua_buf[1] == 'S' || ua_buf[1] == 'M')) {
                uint32_t ua_addr = (uint32_t)strtoul(ua_addr_str, NULL, 16); 
                // get the page number by shifting by 11 bits
                uint32_t ua_page_num = (ua_addr>>11);
                //printf("%u\n", ua_page_num);

                // if head is null, that means we are at the first, address.
                // so just add it in
                if(ua_head == NULL) {
                    //printf("head null\n");
                    ua_head = (unique_addr*)malloc(sizeof(unique_addr));
                    // set the page num
                    ua_head->page_num = ua_page_num;
                    ua_head->info = (ua_info*)malloc(sizeof(ua_info));
                    // set up the line number info
                    ua_head->info->line_num = line_count;
                    ua_head->info->next = NULL;
                    // set next to NULl
                    ua_head->next == NULL;
                } else {
                // Walk the unique address list to find if the page number already exists
                unique_addr *cur_ua = ua_head;
                while (cur_ua != NULL) {
                    if (cur_ua->page_num == ua_page_num) {
                        // the page num is already in the list 
                        // add the line number to its info list
                        ua_info *new_info_node = (ua_info*)malloc(sizeof(ua_info));
                        // set the line number its at
                        new_info_node->line_num = line_count;
                        // move our current head down to be the next node of the new node
                        new_info_node->next = cur_ua->info;
                        // set the head of the list to the recently added line number
                        cur_ua->info = new_info_node;
                        // exit the loop since we found a match
                        break;
                    }
                    cur_ua = cur_ua->next;
                }
                // If the page number doesn't exist in the list, create a new unique address node and add it to the list
                // do an insert at head so we don't have to walk the list (order doesn't really matter here)
                if (cur_ua == NULL) {
                    // Create a new ua node for the unique address
                    unique_addr *new_ua_node = (unique_addr*)malloc(sizeof(unique_addr));
                    if (new_ua_node != NULL) {
                        new_ua_node->page_num = ua_page_num;
                        // do the same for its info
                        new_ua_node->info = (ua_info*)malloc(sizeof(ua_info));
                        // set line number
                        new_ua_node->info->line_num = line_count;
                        new_ua_node->info->next = NULL;
                        // move it to head of list
                        new_ua_node->next = ua_head;
                        // now set it as the head
                        ua_head = new_ua_node;
                    }
                }
            }
                    // printf("added %u\n", ua_page_num);
            }
        }
        // move back to the start of the file when we enter the main loop
        fseek(trace_file, 0, SEEK_SET);
    }

    // create a buffer to hold our instruction
    char buf[32];
    uint32_t line_buf[numframes];
    int max_dist = -1;
    // giant while loop that reads through the entire file
    line_count = 0;
    while (fgets(buf, 32, trace_file) != NULL)
    {
        // use strtok to parse our string into the instructions and page number
        line_count++;
        char *inst = strtok(buf, " ");
        char *addr_str = strtok(NULL, ",");
        // check if our instruction is valid AND we aren't at the end of the file
        if((strcmp(inst, "I") == 0 || strcmp(inst, "L") || strcmp(inst, "S") == 0 || strcmp(inst, "M")) && addr_str != NULL && (buf[0] == 'I' || buf[1] == 'L' || buf[1] == 'S' || buf[1] == 'M')) {
            // have a line count variable that will track where we currently are in the instructions
                // remove leading space on some entries
            if (addr_str[0] == ' ') {
                addr_str++;
            }

            // increment total number of memory accesses
            // since M is basically a load and store, we do 2 memory accesses
            if(strcmp(inst, "M") == 0) {
                total_memory_accesses+=2;
            } else {
                total_memory_accesses++;
            }

            // only used when we do nru
            if(strcmp(alg, "nru") == 0) {
                // do a refresh after -r number of lines 
                if(line_count % refresh == 0) {
                    // printf("refreshed\n\n");
                    // walk through our NRU list and set ref bits to 0
                    nru_node *cur = nru_head;
                    while(cur != NULL) {
                        page_table[cur->node_info->frame_num].ref_bit = 0;
                        cur = cur->next;
                    }
                    
                }
            }
            
            // get page number, first convert the address string into uint32
            uint32_t addr = (uint32_t)strtoul(addr_str, NULL, 16); 
            // get the page number by shifting by 11 bits
            uint32_t page_num = (addr>>11);

            //update the frame number field of our pte struct
            page_table[page_num].frame_num = page_num;

            if(page_table[page_num].valid_bit == 1) {
                // we have a hit. update ref to 1
                page_table[page_num].ref_bit = 1;
            } else {
                total_page_faults++;
                // we have a page fault. we will have to add this page
                // increase our total page faults
                // we first have to check if we have exaused our frame space
                if(curframes == numframes) {
                    if(strcmp(alg, "opt") == 0) {
                        //printf("entering opt\n");
                        // set our max distance to -1 so that it instantly gets changed 
                        // on the first evict
                        int max_dist = -1;
                        int evicted_page = 0;
                        // go through each frame
                        opt_node *cur_node = opt_head;
                        for(int i = 0; i < numframes; i++) {
                            unique_addr *ua_temp = ua_head;
                            // walk through the ua list
                            while(ua_temp != NULL) {
                                // if our current page number matches the one at the current
                                // spot of the ua list, lets get the furthest line
                                if(ua_temp->page_num == cur_node->node_info->frame_num) {
                                    ua_info *info_temp = ua_temp->info;
                                    int best_line = 0;
                                    while (info_temp != NULL) {
                                        if (info_temp->line_num != -1) { 
                                            // last valid line we can use at the end of the list
                                            best_line = info_temp->line_num;
                                            // we found a match. exit loop
                                            break;
                                        }
                                        info_temp = info_temp->next;
                                    }
                                    // Calculate the distance from the current line to the last line in the list
                                    int dist = line_count - best_line;
                                    //printf("dist: %d | max dist: %d\n", dist, max_dist);
                                    // if our current distance is greater than our max
                                    // update the max distance, set the page to be evicted, and
                                    // mark the ua_list's page number as no longer valid
                                    if(dist > max_dist) {
                                        max_dist = dist;
                                        evicted_page = cur_node->node_info->frame_num;
                                        info_temp->line_num = -1;
                                    }
                                }
                                ua_temp = ua_temp->next;
                            }
                            cur_node = cur_node->next;
                        }

                        // for(int i = 0; i < numframes; i++) {
                        //     printf("%u -> ", line_buf[1]);
                        // }
                        // printf("\n");

                        //printf("evicting\n");
                        // If dirty bit is 1, write to disk
                        if(page_table[evicted_page].dirty_bit == 1) {
                            total_writes_to_disk++;
                        }
                        // mark page as invalid
                        page_table[evicted_page].valid_bit = 0;

                        //printf("setting info\n");
                        //update info for the next node
                        opt_head->node_info = &page_table[page_num];
                        // move head to the next node in the list
                        opt_head = opt_head->next;

                        //printf("all done\n");
                    } else if(strcmp(alg, "clock") == 0) {
                        // int that tracks whether our list is full so we can stop
                        // using add_clock when we need it
                        full = 1;
                        clock_node *clock_cur = clock_head;
                        // simple int to exit loop when done
                        int in_loop = 1;
                        while(in_loop == 1) {
                            // if our reference is 0, we can evict that page
                            if(clock_cur->node_info->ref_bit == 0) {
                                // set the evicted page to the current node
                                int evicted_page = clock_cur->node_info->frame_num;

                                // if dirty bit is 0, write to disk
                                if(page_table[evicted_page].dirty_bit == 1) {
                                    total_writes_to_disk++;
                                }

                                // evict page. set valid bit to 0
                                page_table[evicted_page].valid_bit = 0;

                                // rather than removing then adding to the list again i decided another approach
                                // just modify the current node we are it so that it gets the info of the new page
                                // basically, just update its page num and set ref to 1
                                clock_cur->node_info = &page_table[page_num];
                                clock_cur->node_info->ref_bit = 1;

                                // exit loop
                                in_loop = 0;
                            } else {
                                // if ref is not 0, then set it to 0 and go to next node
                                clock_cur->node_info->ref_bit = 0;
                                clock_cur = clock_cur->next;
                            }
                        }
                    } else if (strcmp(alg, "nru") == 0) {
                        // do not set your refresh rate very low
                        // when we get a hit, set ref to 1
                        // once we run out of frames, evict the one that is neither referenced or dirty
                            // have some sort of priority system that goes as
                                // 1 - !ref !dirty
                                // 2 - !ref dirty
                                // 3 - ref !dirty
                                // 4 - ref dirty
                            // on refresh set every vaild page's ref to 0
                        nru_node *cur = nru_head;
                        // go through the list, set priority of page based on above priority system
                        while (cur != NULL) {
                            if (cur->node_info->ref_bit == 0 && cur->node_info->dirty_bit == 0) {
                                cur->p = 1;
                            } else if (cur->node_info->ref_bit == 0 && cur->node_info->dirty_bit == 1) {
                                cur->p = 2;
                            } else if (cur->node_info->ref_bit == 1 && cur->node_info->dirty_bit == 0) {
                                cur->p = 3;
                            } else {
                                cur->p = 4;
                            }
                            cur = cur->next;
                        }

                        // int to track if we found our node so we can exit the loop
                        int found = 0;
                        // for loop for each level of priority, should we not find anything at 
                        // level 1 or 2, etc. (rare that we go past 1)
                        for (int i = 1; i <= 4 && found == 0; i++) {
                            cur = nru_head;
                            // walk thru list, find first node at the lowest current priority (usually 1)
                            while (cur != NULL) {
                                if (cur->p == i) {
                                    // we found the page we want to evict
                                    found = 1;
                                    // create temp node to be used for deletion
                                    nru_node *temp = cur;
                                    int evicted_page = temp->node_info->frame_num;
                                    cur = cur->next;
                                    // if we're evicting the head...
                                    if (temp->prev == NULL) {
                                        // head gets set to it's next node
                                        nru_head = temp->next;
                                        // if the head is already null, just set its prev to null to 
                                        if (nru_head != NULL) {
                                            nru_head->prev = NULL;
                                        }
                                    // if our tail is NULL, set the next of the prev to be null
                                    // essentially make it the new tail
                                    } else if (temp->next == NULL) {
                                        temp->prev->next = NULL;
                                    // if not, cut out the node we want to evict by having
                                    // its previous and next point to eachother
                                    } else {
                                        temp->prev->next = temp->next;
                                        temp->next->prev = temp->prev;
                                    }
                                    // mark as invalid
                                    page_table[evicted_page].valid_bit = 0;
                                    // write to disk if neccesary
                                    if(page_table[evicted_page].dirty_bit == 1) {
                                        total_writes_to_disk++;
                                    }
                                    break;
                                    // free(temp->node_info);
                                    // free(temp);
                                } else {
                                    cur = cur->next;
                                }
                            }
                        }
                    } else {
                        // for if we input an algorithm that doesnt exist
                        perror("invalid algorithm");
                        free(page_table);
                        exit(EXIT_FAILURE);
                    }
                    curframes--;
                }
                //page_table[page_num].line_num = line_count;
                // after eviction (if we did so), we can add the new page
                //add_frame(&page_table[page_num]);

                //inserting into our linked lists
                if(strcmp(alg, "opt") == 0) {
                    add_opt(&page_table[page_num]);
                // clock is a little different
                } else if(strcmp(alg, "clock") == 0) {
                    // if the list is full, we no longer need to use add_clock
                    // just use the replacement method in the clock algorithm
                    if(full == 0) {
                        add_clock(&page_table[page_num]);
                    }
                } else if(strcmp(alg, "nru") == 0) {
                    add_nru(&page_table[page_num]);
                } else {
                    perror("invalid algorithm");
                    exit(EXIT_FAILURE);
                }

                // setting bits for instructions
                // fetch/load
                if(strcmp(inst, "I") == 0 || strcmp(inst, "L") == 0) { 
                    page_table[page_num].valid_bit = 1;
                    page_table[page_num].ref_bit = 1;
                    page_table[page_num].dirty_bit = 0;
                //store/modify (set dirty to 1)
                } else if (strcmp(inst, "S") == 0 || strcmp(inst, "M") == 0) { // Store
                    page_table[page_num].valid_bit = 1;
                    page_table[page_num].ref_bit = 1;
                    page_table[page_num].dirty_bit = 1;
                } 
                curframes++;
            }
        }
    }

    // stuff to print
    fclose(trace_file);

    printf("Algorithm: %s\n", alg);
    printf("Number of frames: %d\n", numframes);
    printf("Total memoroy accesses: %d\n", total_memory_accesses);
    printf("Total page faults: %d\n", total_page_faults);
    printf("Total writes to disk: %d\n", total_writes_to_disk);

    free(page_table);

    return 0;
}

// add_clock will use a citcular linked list implementation
void add_clock(pte *page_table_entry) {
    // malloc node and set info 
    clock_node *new_clock_node = (clock_node*)malloc(sizeof(clock_node));
    new_clock_node->node_info = page_table_entry;

    if(clock_head == NULL) {
        // if head is null, set the next & prev of the new node to point to the same node
        new_clock_node->next = new_clock_node;
        new_clock_node->prev = new_clock_node;
        // then set head to the new node
        clock_head = new_clock_node;
    } else {
        // have the new nodes next point to the clock head's next
        // now they're next nodes are pointing to the same NULL
        new_clock_node->next = clock_head->next;
        // have the new nodes prev point to the head
        new_clock_node->prev = clock_head;
        // point the new node back to the head
        clock_head->next->prev = new_clock_node;
        // next of the head is our new node
        clock_head->next = new_clock_node;
    }

}

// add_nru and add_clock are the same algorithm

void add_nru(pte *page_table_entry) {
    // allocate info for our new node
    nru_node *new_nru_node = (nru_node*)malloc(sizeof(nru_node));
    // include info
    new_nru_node->node_info = page_table_entry;

    // set cur pointer
    nru_node *cur = nru_head;

    if(cur == NULL) {
        // if the head is null just set next and prev to null
        // and also set head as well
        nru_head = new_nru_node;
        nru_head->prev = NULL;
        nru_head->next = NULL;
    // if head is not null...
    } else {
        // walk the list
        while(cur->next != NULL) {
            cur = cur->next;
        }
        // add the new node and update its prev and next pointers
        cur->next = new_nru_node;
        new_nru_node->prev = cur;
        new_nru_node->next = NULL;
    }
}

// same as nru
void add_opt(pte *page_table_entry) {
    opt_node *new_opt_node = (opt_node*)malloc(sizeof(opt_node));
    new_opt_node->node_info = page_table_entry;

    opt_node *cur = opt_head;

    if(cur == NULL) {
        opt_head = new_opt_node;
        opt_head->prev = NULL;
        opt_head->next = NULL;
    } else {
        while(cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new_opt_node;
        new_opt_node->prev = cur;
        new_opt_node->next = NULL;
    }
}