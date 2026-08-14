#ifndef _SC64RTC_H
#define _SC64RTC_H

#include "gameboy.h"

/**
 * If this cartridge is a SummerCart64, read its battery-backed RTC and seed
 * the emulated MBC3 clock from wall time, so Crystal's day/night survives the
 * console being off without anyone visiting the clock menu.
 *
 * Safe to call anywhere: it probes the SC64 identifier register first and
 * leaves the clock alone on any other cart, under any emulator, or if the
 * command interface misbehaves -- the settings.timer seed already applied by
 * initGameboy() stays in effect. Only carts whose MBC carries a timer are
 * touched at all.
 */
void sc64RtcSeedClock(struct GameBoy* gameboy);

#endif
