#include <criterion/criterion.h>

#include "mos6502/mos6502.h"
#include "mos6502/enums.h"
#include "mos6502/tests.h"

TestSuite(mos6502_branch, .init = setup, .fini = teardown);

Test(mos6502_branch, bcc)
{
    cpu->eff_addr = 123;
    mos6502_handle_bcc(cpu, 0);
    cr_assert_eq(cpu->PC, 2);

    cpu->P &= ~MOS_CARRY;
    cpu->eff_addr = 128;
    mos6502_handle_bcc(cpu, 3);
    cr_assert_eq(cpu->PC, 128);
}

Test(mos6502_branch, bcs)
{
    cpu->eff_addr = 123;
    mos6502_handle_bcs(cpu, 0);
    cr_assert_eq(cpu->PC, 123);

    cpu->P &= ~MOS_CARRY;
    cpu->eff_addr = 200;
    mos6502_handle_bcs(cpu, 0);
    cr_assert_eq(cpu->PC, 125);
}

Test(mos6502_branch, beq)
{
    cpu->eff_addr = 123;
    mos6502_handle_beq(cpu, 0);
    cr_assert_eq(cpu->PC, 123);

    cpu->P &= ~MOS_ZERO;
    cpu->eff_addr = 150;
    mos6502_handle_beq(cpu, 0);
    cr_assert_neq(cpu->PC, 150);
}

Test(mos6502_branch, bmi)
{
    cpu->eff_addr = 123;
    mos6502_handle_bmi(cpu, 0);
    cr_assert_eq(cpu->PC, 123);

    cpu->P &= ~MOS_NEGATIVE;
    cpu->eff_addr = 150;
    mos6502_handle_bmi(cpu, 0);
    cr_assert_neq(cpu->PC, 150);
}

Test(mos6502_branch, bne)
{
    cpu->eff_addr = 123;
    mos6502_handle_bne(cpu, 0);
    cr_assert_neq(cpu->PC, 123);

    cpu->P &= ~MOS_ZERO;
    cpu->eff_addr = 125;
    mos6502_handle_bne(cpu, 0);
    cr_assert_eq(cpu->PC, 125);
}

Test(mos6502_branch, bpl)
{
    cpu->eff_addr = 123;
    mos6502_handle_bpl(cpu, 0);
    cr_assert_neq(cpu->PC, 123);

    cpu->P &= ~MOS_NEGATIVE;
    cpu->eff_addr = 125;
    mos6502_handle_bpl(cpu, 0);
    cr_assert_eq(cpu->PC, 125);
}

Test(mos6502_branch, bra)
{
    cr_assert_eq(cpu->PC, 0);
    cpu->eff_addr = 123;
    mos6502_handle_bra(cpu, 0);
    cr_assert_eq(cpu->PC, 123);
}

Test(mos6502_branch, bvc)
{
    cpu->eff_addr = 123;
    mos6502_handle_bvc(cpu, 0);
    cr_assert_neq(cpu->PC, 123);

    cpu->P &= ~MOS_OVERFLOW;
    cpu->eff_addr = 125;
    mos6502_handle_bvc(cpu, 0);
    cr_assert_eq(cpu->PC, 125);
}

Test(mos6502_branch, bvs)
{
    cpu->eff_addr = 123;
    mos6502_handle_bvs(cpu, 0);
    cr_assert_eq(cpu->PC, 123);

    cpu->P &= ~MOS_OVERFLOW;
    cpu->eff_addr = 125;
    mos6502_handle_bvs(cpu, 0);
    cr_assert_eq(cpu->PC, 125);
}
