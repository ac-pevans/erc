/*
 * mos6502.addr.c
 *
 * Here we have support for the address modes that are built into the
 * MOS 6502 chip. In general, these address modes help the instruction
 * figure out _what_ it is working with, which is either a value from a
 * register, or from some place in memory.
 */

#include <stdlib.h>

#include "mos6502.h"
#include "mos6502.enums.h"

/*
 * This is a table of all the possible opcodes the 6502 understands,
 * mapped to the correct address mode. (Well -- I _hope_ it's the
 * correct address mode!)
 */
static int addr_modes[] = {
//   00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F
    IMP, IDX, NOA, NOA, NOA, ZPG, ZPG, NOA, IMP, IMM, ACC, NOA, NOA, ABS, ABS, NOA, // 0x
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // 1x
    ABS, IDX, NOA, NOA, ZPG, ZPG, ZPG, NOA, IMP, IMM, ACC, NOA, ABS, ABS, ABS, NOA, // 2x
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // 3x
    IMP, IDX, NOA, NOA, NOA, ZPG, ZPG, NOA, IMP, IMM, ACC, NOA, ABS, ABS, ABS, NOA, // 4x
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // 5x
    IMP, IDX, NOA, NOA, NOA, ZPG, ZPG, NOA, IMP, IMM, ACC, NOA, IND, ABS, ABS, NOA, // 6x
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // 7x
    NOA, IDX, NOA, NOA, ZPG, ZPG, ZPG, NOA, IMP, NOA, IMP, NOA, ABS, ABS, ABS, NOA, // 8x
    REL, IDY, NOA, NOA, ZPX, ZPX, ZPY, NOA, IMP, ABY, IMP, NOA, NOA, ABX, NOA, NOA, // 9x
    IMM, IDX, IMM, NOA, ZPG, ZPG, ZPG, NOA, IMP, IMM, IMP, NOA, ABS, ABS, ABS, NOA, // Ax
    REL, IDY, NOA, NOA, ZPX, ZPX, ZPY, NOA, IMP, ABY, IMP, NOA, ABX, ABX, ABY, NOA, // Bx
    IMM, IDX, NOA, NOA, ZPG, ZPG, ZPG, NOA, IMP, IMM, IMP, NOA, ABS, ABS, ABS, NOA, // Cx
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // Dx
    IMM, IDX, NOA, NOA, ZPG, ZPG, ZPG, NOA, IMP, IMM, IMP, NOA, ABS, ABS, ABS, NOA, // Ex
    REL, IDY, NOA, NOA, NOA, ZPX, ZPX, NOA, IMP, ABY, NOA, NOA, NOA, ABX, ABX, NOA, // Fx
};

/*
 * This is a _kind_ of factory method, except we're obviously not
 * instantiating an object. Given an address mode, we return the
 * resolver function which will give you the right value (for a given
 * cpu) that an instruction will use.
 */
mos6502_address_resolver
mos6502_get_address_resolver(vm_8bit opcode)
{
    switch (mos6502_addr_mode(opcode)) {
        case ACC: return mos6502_resolve_acc;
        case ABS: return mos6502_resolve_abs;
        case ABX: return mos6502_resolve_abx;
        case ABY: return mos6502_resolve_aby;
        case IMM: return mos6502_resolve_imm;
        case IND: return mos6502_resolve_ind;
        case IDX: return mos6502_resolve_idx;
        case IDY: return mos6502_resolve_idy;
        case REL: return mos6502_resolve_rel;
        case ZPG: return mos6502_resolve_zpg;
        case ZPX: return mos6502_resolve_zpx;
        case ZPY: return mos6502_resolve_zpy;
        case IMP:   // FALLTHRU
        default: break;
    }

    return NULL;
}

/*
 * Just a little macro to help us figure out what the address is for
 * for 16-bit values
 */
#define ADDR_HILO(cpu) \
    vm_16bit addr; \
    vm_8bit hi, lo; \
    hi = mos6502_next_byte(cpu); \
    lo = mos6502_next_byte(cpu); \
    addr = (hi << 8) | lo

/*
 * In contrast to the ADDR_HILO macro, here we want just one byte from
 * the current program counter, and it is the (only) significant byte.
 */
#define ADDR_LO(cpu) \
    vm_16bit addr; \
    addr = mos6502_next_byte(cpu)

#define EFF_ADDR(addr) \
    vm_16bit eff_addr = addr; \
    cpu->last_addr = eff_addr

int
mos6502_addr_mode(vm_8bit opcode)
{
    return addr_modes[opcode];
}

/*
 * In the ACC address mode, the instruction will consider just the A
 * register. (It's probably the simplest resolution mode for us to
 * execute.)
 */
vm_8bit
mos6502_resolve_acc(mos6502 *cpu)
{
    EFF_ADDR(0);
    return cpu->A;
}

/*
 * This is the absolute address mode. The next two bytes are the address
 * in memory at which our looked-for value resides, so we consume those
 * bytes and return the value located therein.
 */
vm_8bit
mos6502_resolve_abs(mos6502 *cpu)
{
    ADDR_HILO(cpu);
    EFF_ADDR(addr);
    return vm_segment_get(cpu->memory, addr);
}

/*
 * The absolute x-indexed address mode is a slight modification of the
 * absolute mode. Here, we consume two bytes, but add the X register
 * value to what we read -- plus one if we have the carry bit set. This
 * is a mode you would use if you were scanning a table, for instance.
 */
vm_8bit
mos6502_resolve_abx(mos6502 *cpu)
{
    ADDR_HILO(cpu);
    CARRY_BIT();
    EFF_ADDR(addr + cpu->X + carry);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * Very much the mirror opposite of the ABX address mode; the only
 * difference is we use the Y register, not the X.
 */
vm_8bit
mos6502_resolve_aby(mos6502 *cpu)
{
    ADDR_HILO(cpu);
    CARRY_BIT();
    EFF_ADDR(addr + cpu->Y + carry);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * In immediate mode, the very next byte is the literal value to be used
 * in the instruction. This is a mode you would use if, for instance,
 * you wanted to say "foo + 5"; 5 would be the operand we return from
 * here.
 */
vm_8bit
mos6502_resolve_imm(mos6502 *cpu)
{
    EFF_ADDR(0);
    return mos6502_next_byte(cpu);
}

/*
 * In indirect mode, we presume that the next two bytes are an address
 * at which _another_ pointer can be found. So we dereference these next
 * two bytes, then dereference the two bytes found at that point, and
 * _that_ is what our value will be.
 */
vm_8bit
mos6502_resolve_ind(mos6502 *cpu)
{
    vm_8bit ind_hi, ind_lo;

    ADDR_HILO(cpu);

    ind_hi = vm_segment_get(cpu->memory, addr);
    ind_lo = vm_segment_get(cpu->memory, addr + 1);
    EFF_ADDR((ind_hi << 8) | ind_lo);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * The indirect x-indexed address mode, as well as the y-indexed mode,
 * are a bit complicated. The single, next byte we read is a zero-page
 * address to the base of _another_ zero-page address in memory; we add
 * X to it, which is the address of what we next dereference. Carry does
 * not factor into the arithmetic.
 */
vm_8bit
mos6502_resolve_idx(mos6502 *cpu)
{
    ADDR_LO(cpu);
    EFF_ADDR(addr + cpu->X);

    return vm_segment_get(
        cpu->memory, 
        vm_segment_get(cpu->memory, eff_addr));
}

/*
 * In significant contrast, the y-indexed indirect mode will read a
 * zero-page address from the next byte, and dereference it immediately.
 * The ensuing address will then have Y added to it, and then
 * dereferenced for the final time. Carry _is_ factored in here.
 */
vm_8bit
mos6502_resolve_idy(mos6502 *cpu)
{
    ADDR_LO(cpu);
    CARRY_BIT();
    EFF_ADDR(vm_segment_get(cpu->memory, addr) + cpu->Y + carry);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * The relative mode means we want to return an address in
 * memory which is relative to PC. If bit 7 is 1, which
 * means if addr > 127, then we treat the operand as though it
 * were negative. 
 */
vm_8bit
mos6502_resolve_rel(mos6502 *cpu)
{
    vm_16bit orig_pc;

    orig_pc = cpu->PC;
    ADDR_LO(cpu);

    if (addr > 127) {
        // e.g. if lo == 128, then cpu->PC + 127 - lo is the
        // same as subtracting 1 from PC.
        EFF_ADDR(orig_pc + 127 - addr);
        return 0;
    }

    // Otherwise lo is a positive offset from PC
    EFF_ADDR(orig_pc + addr);
    return 0;
}

/*
 * Zero page mode is very straightforward. It's very much the same as
 * absolute mode, except we consider just the next byte, and dereference
 * that (which is, by convention, always going to be an address in the
 * zero page of memory).
 */
vm_8bit
mos6502_resolve_zpg(mos6502 *cpu)
{
    ADDR_LO(cpu);
    EFF_ADDR(addr);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * In zero-page x-indexed mode, we read the next byte; add X to that;
 * and dereference the result. Carry is not a factor here.
 */
vm_8bit
mos6502_resolve_zpx(mos6502 *cpu)
{
    ADDR_LO(cpu);
    EFF_ADDR(addr + cpu->X);

    return vm_segment_get(cpu->memory, eff_addr);
}

/*
 * This is, as with absolute y-indexed mode, the mirror opposite of the
 * zero-page x-indexed mode. We simply use the Y register and not the X,
 * and here as well, we do not factor in the carry bit.
 */
vm_8bit
mos6502_resolve_zpy(mos6502 *cpu)
{
    ADDR_LO(cpu);
    EFF_ADDR(addr + cpu->Y);

    return vm_segment_get(cpu->memory, eff_addr);
}