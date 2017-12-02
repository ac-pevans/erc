#ifndef _MOS6502_ENUMS_H_
#define _MOS6502_ENUMS_H_

/*
 * mos6502.enums.h
 *   Enums and other symbols for use with the mos 6502
 *
 * We have separated the definitions of address mode types, instruction
 * types, etc. into their own file so that we can include it in our main
 * source file, as well as from our unit test suite, without necessarily
 * adding them to the global namespace throughout the application.
 */

enum status_flags {
    CARRY = 1,
    ZERO = 2,
    INTERRUPT = 4,
    DECIMAL = 8,
    BREAK = 16,
    OVERFLOW = 64,
    NEGATIVE = 128,
};

enum addr_mode {
    NOA,    // no address mode
    ACC,    // accumulator
    ABS,    // absolute
    ABX,    // absolute x-index
    ABY,    // absolute y-index
    IMM,    // immediate
    IMP,    // implied
    IND,    // indirect
    IDX,    // x-index indirect
    IDY,    // indirect y-index
    REL,    // relative
    ZPG,    // zero page
    ZPX,    // zero page x-index
    ZPY,    // zero page y-index
};

enum instruction {
    ADC,    // ADd with Carry
    AND,    // bitwise AND
    ASL,    // Arithmetic Shift Left
    BCC,    // Branch on Carry Clear
    BCS,    // Branch on Carry Set
    BEQ,    // Branch on EQual to zero
    BIT,    // BIT test
    BMI,    // Branch on MInus 
    BNE,    // Branch on Not Equal to zero
    BPL,    // Branch on PLus
    BRK,    // BReaK (interrupt)
    BVC,    // Branch on oVerflow Clear
    BVS,    // Branch on oVerflow Set
    CLC,    // CLear Carry
    CLD,    // CLear Decimal
    CLI,    // CLear Interrupt disable
    CLV,    // CLear oVerflow
    CMP,    // CoMPare
    CPX,    // ComPare with X register
    CPY,    // ComPare with Y register
    DEC,    // DECrement
    DEX,    // DEcrement X
    DEY,    // DEcrement Y
    EOR,    // Exclusive OR
    INC,    // INCrement
    INX,    // INcrement X
    INY,    // INcrement Y
    JMP,    // JuMP
    JSR,    // Jump to SubRoutine
    LDA,    // LoaD Accumulator
    LDX,    // LoaD X
    LDY,    // LoaD Y
    LSR,    // Logical Shift Right
    NOP,    // NO oPeration
    ORA,    // OR with Accumulator
    PHA,    // PusH Accumulator
    PHP,    // PusH Predicate register
    PLA,    // PulL Accumulator
    PLP,    // PulL Predicate register
    ROL,    // ROtate Left
    ROR,    // ROtate Right
    RTI,    // ReTurn from Interrupt
    RTS,    // ReTurn from Subroutine
    SBC,    // SuBtract with Carry
    SEC,    // SEt Carry
    SED,    // SEt Decimal
    SEI,    // SEt Interrupt disable
    STA,    // STore Accumulator
    STX,    // STore X
    STY,    // STore Y
    TAX,    // Transfer Accumulator to X
    TAY,    // Transfer Accumulator to Y
    TSX,    // Transfer Stack register to X
    TXA,    // Transfer X to Accumulator
    TXS,    // Transfer X to Stack register
    TYA,    // Transfer Y to Accumulator
};

#endif
