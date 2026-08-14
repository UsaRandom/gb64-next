#include <ultra64.h>
#include "sc64rtc.h"
#include "memory_map.h"
#include "../memory.h"

/* The SummerCart64 command mailbox: five registers behind the PI, protocol
 * from SummerCart64 sw/bootloader/src/sc64.c and sw/controller/src/cfg.c.
 * The menu locks the registers before booting a game, so this unlocks with
 * the same "_UNL","OCK_" key pair the SC64 bootloader uses and locks again
 * on every exit path, leaving the cart exactly as the menu left it.
 *
 * How the clock actually works, learned the hard way twice: the MBC3 counter
 * is game-owned, writable state -- games write it to set the clock and to
 * fold the day counter down -- and games diagnose a dead cart battery by the
 * counter moving backward or leaping. Version one of this file re-imposed a
 * wall-derived counter every boot and fought the game's writes; version two
 * stored counter-minus-wall, which was exact arithmetic built on an
 * assumption the hardware then broke: it trusted the cart clock to be
 * consistent between save and load, and a clock that has never been set (no
 * USB on this cart to set it over) can answer anything at all. So version
 * three trusts nothing: timer stays the plain absolute counter that works
 * everywhere, wallAtSave records what the cart clock said at save, and a
 * load applies the difference only when it is forward and under a year.
 * Every lie a clock can tell -- garbage, backward, unset, absent -- lands in
 * the same place: the counter resumes where it stopped, a frozen clock,
 * which is the one failure MBC3 games shrug at. */

#define SC64_REG_SR_CMD     0x1FFF0000
#define SC64_REG_DATA0      0x1FFF0004
#define SC64_REG_DATA1      0x1FFF0008
#define SC64_REG_IDENTIFIER 0x1FFF000C
#define SC64_REG_KEY        0x1FFF0010

#define SC64_V2_IDENTIFIER  0x53437632 /* "SCv2" */
#define SC64_KEY_UNLOCK_1   0x5F554E4C
#define SC64_KEY_UNLOCK_2   0x4F434B5F
#define SC64_KEY_LOCK       0xFFFFFFFF

#define SC64_SR_CMD_ERROR   (1 << 30)
#define SC64_SR_CPU_BUSY    (1u << 31)

#define SC64_CMD_TIME_GET   't'

/* An RTC read is microseconds of firmware work; this bound only exists so a
 * cart that wedges cannot wedge the boot with it. */
#define SC64_BUSY_SPIN_LIMIT 200000

static u32 regRead(u32 reg)
{
    u32 value;
    osPiReadIo(reg, &value);
    return value;
}

static void regWrite(u32 reg, u32 value)
{
    osPiWriteIo(reg, value);
}

static int fromBcd(u32 value)
{
    return ((value >> 4) & 0xF) * 10 + (value & 0xF);
}

/* Days since 2000-01-01 for a civil date; valid through 2099, which outlives
 * the SC64's own two-digit-year RTC. Only differences of this value are ever
 * used, so the epoch itself is arbitrary -- it just has to be the same one
 * at store and at load. */
static u32 daysSince2000(int year, int month, int day)
{
    static const u16 daysBeforeMonth[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    u32 days = (u32)year * 365 + ((year + 3) / 4);
    days += daysBeforeMonth[month - 1];
    if ((year % 4) == 0 && month > 2) {
        days += 1;
    }
    return days + (day - 1);
}

/* Read the SC64 RTC as CPU_TICKS_PER_SECOND ticks since the 2000-01-01
 * epoch. Returns 0 -- leaving *ticksOut alone -- on any cart that is not an
 * SC64, any command failure, or nonsense BCD. */
static int sc64RtcReadTicks(u64 *ticksOut)
{
    u32 sr;
    u32 timeWord;
    u32 dateWord;
    int spins;

    regWrite(SC64_REG_KEY, SC64_KEY_UNLOCK_1);
    regWrite(SC64_REG_KEY, SC64_KEY_UNLOCK_2);

    if (regRead(SC64_REG_IDENTIFIER) != SC64_V2_IDENTIFIER)
    {
        /* Not a SummerCart64 -- ares, a different cart, anything. The two key
         * writes above were writes to plain cart address space and no other
         * hardware assigns them meaning. */
        return 0;
    }

    regWrite(SC64_REG_SR_CMD, SC64_CMD_TIME_GET);

    spins = 0;
    do {
        sr = regRead(SC64_REG_SR_CMD);
        if (++spins > SC64_BUSY_SPIN_LIMIT)
        {
            regWrite(SC64_REG_KEY, SC64_KEY_LOCK);
            return 0;
        }
    } while (sr & SC64_SR_CPU_BUSY);

    if (sr & SC64_SR_CMD_ERROR)
    {
        regWrite(SC64_REG_KEY, SC64_KEY_LOCK);
        return 0;
    }

    /* rsp0 = weekday|hour|minute|second, rsp1 = century|year|month|day,
     * every field BCD (cfg_get_time passes the RTC chip's encoding through). */
    timeWord = regRead(SC64_REG_DATA0);
    dateWord = regRead(SC64_REG_DATA1);
    regWrite(SC64_REG_KEY, SC64_KEY_LOCK);

    {
        int second = fromBcd(timeWord & 0xFF);
        int minute = fromBcd((timeWord >> 8) & 0xFF);
        int hour = fromBcd((timeWord >> 16) & 0xFF);
        int day = fromBcd(dateWord & 0xFF);
        int month = fromBcd((dateWord >> 8) & 0xFF);
        int year = fromBcd((dateWord >> 16) & 0xFF);

        if (second > 59 || minute > 59 || hour > 23 ||
            day < 1 || day > 31 || month < 1 || month > 12)
        {
            return 0;
        }

        *ticksOut = ((u64)daysSince2000(year, month, day) * 86400
                   + (u64)hour * 3600 + (u64)minute * 60 + (u64)second)
                   * CPU_TICKS_PER_SECOND;
        return 1;
    }
}

static int isTimerCart(struct GameBoy* gameboy)
{
    return gameboy->memory.mbc && (gameboy->memory.mbc->flags & MBC_FLAGS_TIMER);
}

/* The most off-time a load will believe, in ticks: 365 days. Past this --
 * and past any backward step, however small -- the clock is assumed to be
 * lying and the counter simply resumes where the save left it. A frozen
 * clock is the one failure mode MBC3 games treat as normal; a counter that
 * moves backward or leaps is the one they call a dead battery. The bound
 * also stays clear of the counter's own 512-day wrap. */
#define SC64_MAX_OFF_TICKS ((u64)365 * 86400 * CPU_TICKS_PER_SECOND)

void sc64RtcApplyLoadedTimer(struct GameBoy* gameboy)
{
    u64 now;
    u64 elapsed;

    if (!isTimerCart(gameboy))
    {
        return;
    }

    /* A counter at or past the MBC3's own 512-day carry can never be
     * legitimate stored state -- games fold their day counter far below
     * that -- only an artifact of the version-2 offset experiment or a
     * corrupt save. Left alone it latches the carry bit at every boot,
     * which games diagnose as a dead cart battery and answer with a
     * set-the-clock prompt, forever, no matter how many times the clock is
     * set and saved. Restarting at zero costs one honest prompt instead. */
    if (gameboy->memory.misc.time >= (u64)512 * 86400 * CPU_TICKS_PER_SECOND)
    {
        gameboy->memory.misc.time = 0;
    }

    /* initGameboy already applied the absolute counter; everything below
     * only ever adds trustworthy off-time on top of it. */
    if (gameboy->settings.wallAtSave == 0)
    {
        return;
    }
    if (!sc64RtcReadTicks(&now))
    {
        return;
    }
    if (now < gameboy->settings.wallAtSave)
    {
        return;
    }
    elapsed = now - gameboy->settings.wallAtSave;
    if (elapsed > SC64_MAX_OFF_TICKS)
    {
        return;
    }

    gameboy->memory.misc.time = gameboy->settings.timer + elapsed;
}

void sc64RtcStoreTimer(struct GameBoy* gameboy)
{
    u64 now;

    gameboy->settings.timer = gameboy->memory.misc.time;
    gameboy->settings.wallAtSave =
        (isTimerCart(gameboy) && sc64RtcReadTicks(&now)) ? now : 0;
}
