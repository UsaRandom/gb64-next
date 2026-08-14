#ifndef _SC64RTC_H
#define _SC64RTC_H

#include "gameboy.h"

/**
 * Real time for MBC3 timer carts, from two clocks with one job each.
 *
 * While the console runs, the N64's own CPU counter (osGetTime) is the time
 * source: at every latch the emulated counter is recomputed from an anchor
 * (counter value + osGetTime captured together) rather than trusted as a
 * running variable, so nothing that happens to misc.time between latches can
 * survive a read. The game moving the clock is the one exception: the asm
 * write handler raises misc.timerWrittenByGame and the next latch re-bases
 * the anchor on the game's value instead.
 *
 * Across power-offs, the SummerCart64 RTC supplies the elapsed time:
 * settings.timer stays the plain absolute counter and settings.wallAtSave
 * records the SC64 clock at save; loading applies the difference only when
 * it is forward and under a year. Without an SC64 (ares, other carts) the
 * clock is exact within a session and frozen across sessions -- the one
 * failure mode MBC3 games accept silently.
 */

/* After initGameboy has applied settings.timer to the counter: clamp
 * impossible counters, add trustworthy off-time, and set the session anchor. */
void sc64RtcApplyLoadedTimer(struct GameBoy* gameboy);

/* Recompute misc.time from the session anchor (or fold a game write into
 * it). Called at every MBC3 latch and before every persist. */
void sc64RtcSyncTime(struct Memory* memory);

/* Bring the counter up to date, then capture it and the SC64 wall clock
 * into settings.timer / settings.wallAtSave for persisting. */
void sc64RtcStoreTimer(struct GameBoy* gameboy);

#endif
