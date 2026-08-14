#ifndef _SC64RTC_H
#define _SC64RTC_H

#include "gameboy.h"

/**
 * SummerCart64 real-time clock support for MBC3 timer carts.
 *
 * The emulated counter is game-owned, writable state; the SC64 RTC is used
 * only to account for time that passes while the console is off. Saves store
 * counter-minus-wall in settings.timer (marked by
 * GB_SETTINGS_FLAGS_RTC_OFFSET); loading adds the wall clock back. On any
 * cart that is not an SC64 both calls degrade to the original absolute-
 * counter behaviour.
 */

/* After initGameboy has applied settings.timer to the counter: convert an
 * offset-flagged timer into a live counter using the SC64 clock. */
void sc64RtcApplyLoadedTimer(struct GameBoy* gameboy);

/* In place of `settings.timer = misc.time` when persisting: store the offset
 * (and flag it) when an SC64 clock is present, the absolute counter when not. */
void sc64RtcStoreTimer(struct GameBoy* gameboy);

#endif
