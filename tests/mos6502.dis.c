#include <criterion/criterion.h>
#include <unistd.h>

#include "mos6502.dis.h"
#include "mos6502.enums.h"

/*
 * BUFSIZ is the normal block-buffer size that a FILE stream would use
 * (possibly amongst other things).
 */
static char buf[BUFSIZ];

/*
 * This is the file stream we will be using to write our disassembly
 * code into.
 */
static FILE *stream = NULL;

static void
setup()
{
    // Ok, so...there's some...trickery going on here. As you might
    // guess by the file path being /dev/null.
    stream = fopen("/dev/null", "w");
    if (stream == NULL) {
        perror("Could not open temporary file for mos6502 disassembly tests");
    }

    // The C standard library allows us to set an arbitrary buffer for a
    // file stream. It also allows us to fully buffer the file stream,
    // which means nothing is written to file until an fflush() or an
    // fclose() is called (or something else I can't think of). So we
    // can use the FILE abstraction to write our disassembly results
    // into, but use an underlying string buffer that we can easily
    // check with Criterion. Uh, unless we blow out the buffer size...
    // don't do that :D
    setvbuf(stream, buf, _IOFBF, 256);
}

static void
teardown()
{
    fclose(stream);
}

static void
assert_buf(const char *str)
{
    // This will set the cursor position in the file back to the start
    // of the file stream.
    rewind(stream);

    // Our actual assertion. The downside to doing it this way is that
    // when Criterion flags an assertion failure, it'll highlight _this_
    // line in the file, not in the test. It might be worth macroifying
    // this code.
    cr_assert_str_eq(buf, str);

    // We're not sure what previous tests may have run, and where NUL
    // characters were set therein, so to be safe we wipe out the full
    // contents of the test buffer after every test.
    memset(buf, 0, BUFSIZ);
}

TestSuite(mos6502_dis, .init = setup, .fini = teardown);

Test(mos6502_dis, operand)
{
    mos6502_dis_operand(stream, 0, ABS, 0x1234);
    assert_buf("$1234");
    mos6502_dis_operand(stream, 0, ABX, 0x1234);
    assert_buf("$1234,X");
    mos6502_dis_operand(stream, 0, ABY, 0x1234);
    assert_buf("$1234,Y");
    mos6502_dis_operand(stream, 0, IMM, 0x12);
    assert_buf("#$12");

    // For JMPs and JSRs (and BRKs), this should be a label and not a
    // literal value. So we need to test both the literal and
    // jump-labeled version.
    mos6502_dis_operand(stream, 0, IND, 0x1234);
    assert_buf("($1234)");
    mos6502_dis_jump_label(0x1234, 0, IND);
    mos6502_dis_operand(stream, 0, IND, 0x1234);
    assert_buf("ADDR_4660"); // = 0x1234

    // Let's undo our label above...
    mos6502_dis_jump_unlabel(0x1234);

    mos6502_dis_operand(stream, 0, IDX, 0x12);
    assert_buf("($12,X)");
    mos6502_dis_operand(stream, 0, IDY, 0x34);
    assert_buf("($34),Y");
    mos6502_dis_operand(stream, 0, ZPG, 0x34);
    assert_buf("$34");
    mos6502_dis_operand(stream, 0, ZPX, 0x34);
    assert_buf("$34,X");
    mos6502_dis_operand(stream, 0, ZPY, 0x34);
    assert_buf("$34,Y");
    
    // These should both end up printing nothing
    mos6502_dis_operand(stream, 0, ACC, 0);
    assert_buf("");
    mos6502_dis_operand(stream, 0, IMP, 0);
    assert_buf("");

    // Test a forward jump (operand < 128)
    mos6502_dis_operand(stream, 500, REL, 52);
    assert_buf("ADDR_552");

    // Test a backward jump (operand >= 128)
    mos6502_dis_operand(stream, 500, REL, 152);
    assert_buf("ADDR_396");
}

Test(mos6502_dis, instruction)
{
#define TEST_INST(x) \
    mos6502_dis_instruction(stream, x); \
    assert_buf(#x)

    TEST_INST(ADC);
    TEST_INST(AND);
    TEST_INST(ASL);
    TEST_INST(BCC);
    TEST_INST(BCS);
    TEST_INST(BEQ);
    TEST_INST(BIT);
    TEST_INST(BMI);
    TEST_INST(BNE);
    TEST_INST(BPL);
    TEST_INST(BRK);
    TEST_INST(BVC);
    TEST_INST(BVS);
    TEST_INST(CLC);
    TEST_INST(CLD);
    TEST_INST(CLI);
    TEST_INST(CLV);
    TEST_INST(CMP);
    TEST_INST(CPX);
    TEST_INST(CPY);
    TEST_INST(DEC);
    TEST_INST(DEX);
    TEST_INST(DEY);
    TEST_INST(EOR);
    TEST_INST(INC);
    TEST_INST(INX);
    TEST_INST(INY);
    TEST_INST(JMP);
    TEST_INST(JSR);
    TEST_INST(LDA);
    TEST_INST(LDX);
    TEST_INST(LDY);
    TEST_INST(LSR);
    TEST_INST(NOP);
    TEST_INST(ORA);
    TEST_INST(PHA);
    TEST_INST(PHP);
    TEST_INST(PLA);
    TEST_INST(PLP);
    TEST_INST(ROL);
    TEST_INST(ROR);
    TEST_INST(RTI);
    TEST_INST(RTS);
    TEST_INST(SBC);
    TEST_INST(SEC);
    TEST_INST(SED);
    TEST_INST(SEI);
    TEST_INST(STA);
    TEST_INST(STX);
    TEST_INST(STY);
    TEST_INST(TAX);
    TEST_INST(TAY);
    TEST_INST(TSX);
    TEST_INST(TXA);
    TEST_INST(TXS);
    TEST_INST(TYA);
}

Test(mos6502_dis, expected_bytes)
{
#define TEST_BYTES(x, y) \
    cr_assert_eq(mos6502_dis_expected_bytes(x), y)

    TEST_BYTES(ACC, 0);
    TEST_BYTES(ABS, 2);
    TEST_BYTES(ABX, 2);
    TEST_BYTES(ABY, 2);
    TEST_BYTES(IMM, 1);
    TEST_BYTES(IMP, 0);
    TEST_BYTES(IND, 2);
    TEST_BYTES(IDX, 1);
    TEST_BYTES(IDY, 1);
    TEST_BYTES(REL, 1);
    TEST_BYTES(ZPG, 1);
    TEST_BYTES(ZPX, 1);
    TEST_BYTES(ZPY, 1);
}

Test(mos6502_dis, opcode)
{
    vm_segment *segment;
    int bytes;

    segment = vm_segment_create(1000);

    vm_segment_set(segment, 0, 0x29);   // AND (imm)
    vm_segment_set(segment, 1, 0x38);

    bytes = mos6502_dis_opcode(stream, segment, 0);
    assert_buf("    AND     #$38\n");
    cr_assert_eq(bytes, 2);
}

Test(mos6502_dis, scan)
{
    vm_segment *segment;

    segment = vm_segment_create(1000);

    vm_segment_set(segment, 0, 0x29);   // AND (imm)
    vm_segment_set(segment, 1, 0x38);
    vm_segment_set(segment, 2, 0x88);   // DEY (imp)
    vm_segment_set(segment, 3, 0x6C);   // JMP (ind)
    vm_segment_set(segment, 4, 0x12);
    vm_segment_set(segment, 5, 0x34);

    mos6502_dis_scan(stream, segment, 0, 6);

    assert_buf("    AND     #$38\n"
               "    DEY\n"
               "    JMP     ($1234)\n"
               );
}

Test(mos6502_dis, jump_label)
{
    cr_assert_eq(mos6502_dis_is_jump_label(123), false);

    mos6502_dis_jump_label(123, 0, IND);
    cr_assert_eq(mos6502_dis_is_jump_label(123), true);

    mos6502_dis_jump_unlabel(123);
    cr_assert_eq(mos6502_dis_is_jump_label(123), false);
}

Test(mos6502_dis, label)
{
    mos6502_dis_label(stream, 123);
    assert_buf("ADDR_123");
}