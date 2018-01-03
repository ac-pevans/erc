/*
 * apple2.mem.c
 */

#include "apple2.h"

/*
 * Return a byte of memory from a bank-switchable address. This may be
 * from ROM, from main memory, or from the "extra" 4k bank of RAM.
 */
vm_8bit
apple2_mem_read_bank(vm_segment *segment, size_t address, void *_mach)
{
    apple2 *mach;

    mach = (apple2 *)_mach;
    
    switch (mach->memory_mode) {
        // Return memory from the rom bank
        case MEMORY_BANK_ROM:
            // We need to account for the difference in address location
            // before we can successfully get any data from ROM.
            return vm_segment_get(mach->rom, address - APPLE2_BANK_OFFSET);

        // If the address is $D000..$DFFF, then we need to get the byte
        // from the ram2 bank. Otherwise, we break to use default
        // behavior.
        case MEMORY_BANK_RAM2:
            if (address < 0xE000) {
                // The same caution holds for getting data from the
                // second RAM bank.
                return vm_segment_get(mach->ram2, 
                                      address - APPLE2_BANK_OFFSET);
            }
            
            break;

        case MEMORY_BANK_RAM1:
        default:
            break;
    }

    // The "default" behavior as referred-to above is simply to return
    // the value as held in our primary memory bank.
    return segment->memory[address];
}

/*
 * Write a byte into bank-switchable memory. Many of the same cautions,
 * notes, etc. written for the read function apply here as well.
 */
void
apple2_mem_write_bank(vm_segment *segment, 
                      size_t address, vm_8bit value, void *_mach)
{
    apple2 *mach;

    mach = (apple2 *)_mach;

    switch (mach->memory_mode) {
        case MEMORY_BANK_ROM:
            vm_segment_set(mach->rom,
                           address - APPLE2_BANK_OFFSET, value);
            return;

        case MEMORY_BANK_RAM2:
            if (address < 0xE000) {
                vm_segment_set(mach->ram2,
                               address - APPLE2_BANK_OFFSET, value);
                return;
            }

        case MEMORY_BANK_RAM1:
        default:
            break;
    }

    // Just set the value in main memory
    segment->memory[address] = value;
}

/*
 * Set the memory map functions for main memory in an apple2 machine
 */
void
apple2_mem_map(apple2 *mach)
{
    size_t addr;

    for (addr = APPLE2_BANK_OFFSET; addr < MOS6502_MEMSIZE; addr++) {
        vm_segment_read_map(mach->memory, addr, apple2_mem_read_bank);
        vm_segment_write_map(mach->memory, addr, apple2_mem_write_bank);
    }
}