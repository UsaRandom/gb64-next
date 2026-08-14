
.global mbc3WriteTimer
.align 4
mbc3WriteTimer:
    lw TMP2, MEMORY_MISC_RAM_BANK(Memory)
    slti $at, TMP2, 0x8
    bnez $at, mbc3WriteTimer_error
    slti $at, TMP2, 0xD
    beqz $at, mbc3WriteTimer_error
    nop
    add TMP2, TMP2, Memory
    sb VAL, (REG_RTC_S+MEMORY_MISC_START-MM_REGISTER_START-0x8)(TMP2)

    # set tmp2 to 0
    dsubu TMP2, TMP2, TMP2
    # load upper day bits
    read_register_direct TMP2, REG_RTC_DH
    andi TMP2, TMP2, 0x81
    sll TMP2, TMP2, 8
    # load lower day bits
    read_register_direct $at, REG_RTC_DL
    or TMP2, TMP2, $at
    # days to hours
    li $at, 24
    multu TMP2, $at
    # hours to minutes
    read_register_direct $at, REG_RTC_H
    mflo TMP2
    add TMP2, TMP2, $at
    li $at, 60
    multu TMP2, $at
    # minutes to second 
    read_register_direct $at, REG_RTC_M
    mflo TMP2
    add TMP2, TMP2, $at
    li $at, 60
    multu TMP2, $at
    # the minutes-to-seconds product was never read back (and the seconds
    # register never added), so a clock set landed 60x short of where the
    # game put it -- upstream bug, invisible until wall-clock support made
    # the counter's absolute value matter.
    read_register_direct $at, REG_RTC_S
    mflo TMP2
    add TMP2, TMP2, $at
    # seconds to ticks
    dsll TMP2, TMP2, 20
    # Two explicit word stores, never `sd`: under -mabi=32 this toolchain's
    # gas assembles `sd` as the o32 REGISTER-PAIR macro -- sw TMP2 followed
    # by sw of the NEXT register -- which wrote whatever that register held
    # into the counter's low word. That is the corrupt-clock bug the M64
    # kept reporting as a dead cart battery. Big-endian: high word first.
    dsrl32 $at, TMP2, 0
    sw $at, MEMORY_MISC_TIMER(Memory)
    sw TMP2, (MEMORY_MISC_TIMER+4)(Memory)
    jr $ra
    nop

mbc3WriteTimer_error:
    jr $ra
    nop


.global mbc3ReadTimer
.align 4
mbc3ReadTimer:
    lw TMP2, MEMORY_MISC_RAM_BANK(Memory)
    slti $at, TMP2, 0x8
    bnez $at, mbc3ReadTimer_error
    slti $at, TMP2, 0xD
    beqz $at, mbc3ReadTimer_error
    nop
    add TMP2, TMP2, Memory
    jr $ra
    lbu $v0, (REG_RTC_S+MEMORY_MISC_START-MM_REGISTER_START-0x8)(TMP2)

mbc3ReadTimer_error:
    jr $ra
    li $v0, 0xFF
    
.global HUC1_READ_IR
.align 4
HUC1_READ_IR:
    jr $ra
    li $v0, 0xC0
