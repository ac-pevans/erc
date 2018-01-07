#include <criterion/criterion.h>

#include "mos6502.h"
#include "mos6502.enums.h"
#include "mos6502.tests.h"

TestSuite(mos6502, .init = setup, .fini = teardown);

/* Test(mos6502, free) */

Test(mos6502, create)
{
    cr_assert_neq(cpu, NULL);

    cr_assert_eq(cpu->memory->size, MOS6502_MEMSIZE);

    cr_assert_eq(cpu->PC, 0);
    cr_assert_eq(cpu->A, 0);
    cr_assert_eq(cpu->X, 0);
    cr_assert_eq(cpu->Y, 0);
    cr_assert_eq(cpu->P, 0);
    cr_assert_eq(cpu->S, 0);
}

Test(mos6502, next_byte)
{
    cpu->PC = 128;
    vm_segment_set(cpu->memory, cpu->PC, 123);
    vm_segment_set(cpu->memory, cpu->PC + 1, 234);
    vm_segment_set(cpu->memory, cpu->PC + 2, 12);

    cr_assert_eq(mos6502_next_byte(cpu), 123);
    cr_assert_eq(mos6502_next_byte(cpu), 234);
    cr_assert_eq(mos6502_next_byte(cpu), 12);
}

Test(mos6502, push_stack)
{
    mos6502_push_stack(cpu, 0x1234);
    cr_assert_eq(vm_segment_get(cpu->memory, 0x0100), 0x12);
    cr_assert_eq(vm_segment_get(cpu->memory, 0x0101), 0x34);
}

Test(mos6502, pop_stack)
{
    mos6502_push_stack(cpu, 0x1234);
    cr_assert_eq(mos6502_pop_stack(cpu), 0x1234);
}

Test(mos6502, modify_status)
{
    mos6502_modify_status(cpu, MOS_NEGATIVE, 130);
    cr_assert_eq(cpu->P & MOS_NEGATIVE, MOS_NEGATIVE);
    mos6502_modify_status(cpu, MOS_NEGATIVE, 123);
    cr_assert_neq(cpu->P & MOS_NEGATIVE, MOS_NEGATIVE);

    mos6502_modify_status(cpu, MOS_OVERFLOW, 123);
    cr_assert_eq(cpu->P & MOS_OVERFLOW, MOS_OVERFLOW);
    mos6502_modify_status(cpu, MOS_OVERFLOW, 44);
    cr_assert_neq(cpu->P & MOS_OVERFLOW, MOS_OVERFLOW);

    mos6502_modify_status(cpu, MOS_CARRY, 23);
    cr_assert_eq(cpu->P & MOS_CARRY, MOS_CARRY);
    mos6502_modify_status(cpu, MOS_CARRY, 0);
    cr_assert_neq(cpu->P & MOS_CARRY, MOS_CARRY);

    mos6502_modify_status(cpu, MOS_ZERO, 0);
    cr_assert_eq(cpu->P & MOS_ZERO, MOS_ZERO);
    mos6502_modify_status(cpu, MOS_ZERO, 1);
    cr_assert_neq(cpu->P & MOS_ZERO, MOS_ZERO);
}

Test(mos6502, set_status)
{
    mos6502_set_status(cpu, MOS_BREAK | MOS_INTERRUPT | MOS_DECIMAL);
    cr_assert_eq(cpu->P & (MOS_BREAK | MOS_INTERRUPT | MOS_DECIMAL), MOS_BREAK | MOS_INTERRUPT | MOS_DECIMAL);
}

Test(mos6502, instruction)
{
    cr_assert_eq(mos6502_instruction(0x1D), ORA);
    cr_assert_eq(mos6502_instruction(0xD8), CLD);
    cr_assert_eq(mos6502_instruction(0x98), TYA);
}

Test(mos6502, cycles)
{
    cr_assert_eq(mos6502_cycles(cpu, 0x76), 6);
    cr_assert_eq(mos6502_cycles(cpu, 0xBA), 2);

    // In this case, we aren't cross a page boundary, and the number of
    // cycles should stay at 4
    cpu->last_addr = 0x5070;
    cpu->X = 23;
    cr_assert_eq(mos6502_cycles(cpu, 0x1D), 4);

    // Testing that crossing a page boundary adds one to the number of
    // cycles
    cpu->X = 200;
    cr_assert_eq(mos6502_cycles(cpu, 0x1D), 5);
}

Test(mos6502, get_instruction_handler)
{
    cr_assert_eq(mos6502_get_instruction_handler(0x00), mos6502_handle_brk);
    cr_assert_eq(mos6502_get_instruction_handler(0x1D), mos6502_handle_ora);
    cr_assert_eq(mos6502_get_instruction_handler(0x20), mos6502_handle_jsr);
}

Test(mos6502, execute)
{
    vm_segment_set(cpu->memory, 0, 34);
    mos6502_execute(cpu, 0x69);
    cr_assert_eq(cpu->A, 34);
}

Test(mos6502, read_byte)
{
    vm_segment_set(cpu->memory, 0, 0x54);
    cr_assert_eq(mos6502_read_byte(cpu), 0x54);
}

Test(mos6502, would_jump)
{
    bool expect;
    for (int inst = 0; inst <= TYA; inst++) {
        switch (inst) {
            case BCC:
            case BCS:
            case BEQ:
            case BMI:
            case BNE:
            case BPL:
            case BRK:
            case BVC:
            case BVS:
            case JMP:
            case JSR:
                expect = true;
                break;

            default:
                expect = false;
                break;
        }

        cr_assert_eq(mos6502_would_jump(inst), expect);
    }
}

Test(mos6502, flash_memory)
{
    vm_segment *segment;

    segment = vm_segment_create(MOS6502_MEMSIZE);
    vm_segment_set(segment, 0, 123);
    vm_segment_set(segment, 1, 124);
    mos6502_flash_memory(cpu, segment);

    cr_assert_eq(vm_segment_get(cpu->memory, 0), 123);
    cr_assert_eq(vm_segment_get(cpu->memory, 1), 124);
}

Test(mos6502, get_address_resolver)
{
    cr_assert_eq(mos6502_get_address_resolver(0x0A), mos6502_resolve_acc);
    cr_assert_eq(mos6502_get_address_resolver(0x20), mos6502_resolve_abs);
    cr_assert_eq(mos6502_get_address_resolver(0xBC), mos6502_resolve_abx);
    cr_assert_eq(mos6502_get_address_resolver(0x19), mos6502_resolve_aby);
    cr_assert_eq(mos6502_get_address_resolver(0xA0), mos6502_resolve_imm);
    cr_assert_eq(mos6502_get_address_resolver(0x6C), mos6502_resolve_ind);
    cr_assert_eq(mos6502_get_address_resolver(0x01), mos6502_resolve_idx);
    cr_assert_eq(mos6502_get_address_resolver(0x11), mos6502_resolve_idy);
    cr_assert_eq(mos6502_get_address_resolver(0x10), mos6502_resolve_rel);
    cr_assert_eq(mos6502_get_address_resolver(0x05), mos6502_resolve_zpg);
    cr_assert_eq(mos6502_get_address_resolver(0x15), mos6502_resolve_zpx);
    cr_assert_eq(mos6502_get_address_resolver(0x96), mos6502_resolve_zpy);
}

