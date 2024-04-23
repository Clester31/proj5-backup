import argparse, math

# i should probably do this in C

# vmsim –n <numframes> -a <opt|clock|nru> [-r <refresh>] <tracefile>
# only use r for NRU

# Physical memory: num frames * 2kb (2^11 bytes)
# Virtual memory: set at 2^32 bytes (32-bit addr space)

# page table: 1 entry for every page in virtual memory
# 2^21 page table entries

# Only evict when the page table fills us

# Page table should be 2^21 length array

# Trace file:
#     Split first letter
#     M is 2 of I->L->s
#     Dont need to worry about size
#     convert the hex into 32-bit binary (uint 32)
#     bottom 11 bits are the offset
#     other 21 bits are the page number (right shify by 11 bits to only get the page number)
#         convert hex into int and shift by 11 bits -> PTE[page #]

#     malloc(sizeof(PTE) * 2^21)
    
# Page table entry has...
#     (INT) Frame number: points to a space in physical memory
#     (BOOL) Valid bit -> 0 = not mapped in physical memory (init to 0)
#     (BOOL) reference bit -> 1 = page has been accesses at all since added (init to 0)
#         if anything happens (I L S M) at the addr, set it to 1
#     (BOOL) dirty -> 1 means we've done a save or modify on that page

PAGE_SIZE = 2048 # 2KB per page
TOTAL_PAGES = math.pow(2, 20) # Number of pages necessary for 32-bit address space

class Page_Table_Entry:
    def __init__(self):
        self.pte_frame_num = pte_frame_num
        self.pte_valid_bit = pte_valid_bit
        self.pte_ref_bit = pte_valid_bit
        self.pte_ditry_bit = pte_dirty_bit


def main():
    # parse our arguments
    parser = argparse.ArgumentParser(description="VMSIM")

    parser.add_argument("-n", "--numframes", type=int, help="number of frames", required=True)
    parser.add_argument("-a", "--algorithm", type=str, choices=["opt", "clock", "nru"], help="algorithm type", required=True)
    parser.add_argument("-r", "--refresh", type=int, help="Refresh interval (for nru only)", default=0)
    parser.add_argument("tracefile", type=str, help="tracefile path")

    args = parser.parse_args()

    # Extract arguments
    numframes = args.numframes
    algorithm = args.algorithm
    refresh = args.refresh
    tracefile = args.tracefile

    print("Number of frames:", numframes)
    print("Algorithm:", algorithm)
    print("Refresh interval:", refresh)
    print("Tracefile:", tracefile)

    # set up the page table and things of that nature
    # page_table = [0] * TOTAL_PAGES

    # check which algorithm we specified and run it
    if algorithm == 'opt':
        opt_alg(numframes, tracefile)
    elif algorithm == 'clock':
        clock_alg(numframes, tracefile)
    elif algorithm == 'nru':
        nru_alg(numframes, refresh, tracefile)
    else:
        print("ERROR: invalid arguments")

def opt_alg(numframes, tracefile):
    print("Optimal")

def clock_alg(numframes, tracefile):
    print("Clock")

def nru_alg(numframes, refresh, tracefile):
    print("NUR")


if __name__ == "__main__":
    main()