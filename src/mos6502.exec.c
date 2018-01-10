/*
 * mos6502.exec.c
 *
 * These instructions concern program execution; things like JMP, JSR,
 * BRK, and so forth.
 */

#include "mos6502.h"
#include "mos6502.enums.h"

/*
 * The BRK instruction will set the interrupt bit; will push the current
 * PC address to the stack; and will advance the counter by 2 positions.
 */
DEFINE_INST(brk)
{
    mos6502_push_stack(cpu, cpu->PC);
    mos6502_push_stack(cpu, cpu->P);
    cpu->P |= MOS_INTERRUPT;
    cpu->PC += 2;
}

/*
 * A jump is straight forward; whatever the effective address is, that
 * is now the new value of the PC register.
 */
DEFINE_INST(jmp)
{
    cpu->PC = cpu->last_addr;
}

/*
 * Meanwhile, a JSR (or jump to subroutine) is a little more nuanced. We
 * record our current position, plus two, to the stack, and jump the
 * effective address.
 */
DEFINE_INST(jsr)
{
    mos6502_push_stack(cpu, cpu->PC + 3);
    cpu->PC = cpu->last_addr;
}

/*
 * The NOP instruction is short for no-operation. It does nothing except
 * waste cycles (which happens elsewhere).
 */
DEFINE_INST(nop)
{
    // do nothing
}

/*
 * Here we return from an interrupt, which effectively resets the PC
 * register to the last value on the stack.
 */
DEFINE_INST(rti)
{
    cpu->P = mos6502_pop_stack(cpu);
    cpu->PC = mos6502_pop_stack(cpu);
}

/*
 * The RTS instruction (return from subroutine) works the same as the
 * RTI instruction, which may or may not be a misconception on my part.
 */
DEFINE_INST(rts)
{
    cpu->PC = mos6502_pop_stack(cpu);
}
