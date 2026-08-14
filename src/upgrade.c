
#include "upgrade.h"
#include "save.h"

void updateToLatestVersion(struct GameBoy* gameboy)
{
    while (gameboy->settings.version < GB_SETTINGS_CURRENT_VERSION) 
    {
        switch (gameboy->settings.version)
        {
            case 0:
                if (gameboy->memory.mbc->bankSwitch == handleMBC3Write)
                {
                    gameboy->memory.misc.ramRomSelect = gameboy->memory.misc.romBankUpper;
                    gameboy->memory.misc.romBankUpper = 0;
                }
                break;
            case 1:
                gameboy->settings.compressedSize = 0;
                gameboy->settings.storedType = getDeprecatedStoredInfoType(gameboy);
                break;
            case 2:
                /* The version-2 RTC experiment stored counter-minus-wall in
                 * timer behind this flag. The wall base is unrecoverable, so
                 * the counter restarts and the game asks for the clock once
                 * more -- the last time it ever should. */
                if (gameboy->settings.flags & GB_SETTINGS_FLAGS_RTC_OFFSET)
                {
                    gameboy->settings.timer = 0;
                    gameboy->settings.flags &= (u16)~GB_SETTINGS_FLAGS_RTC_OFFSET;
                }
                gameboy->settings.wallAtSave = 0;
                break;
        }

        ++gameboy->settings.version;
    }
}