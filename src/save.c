/* save.c — T56: Slot tabanlı binary kayıt sistemi
 * SaveRaw/LoadRaw/CheckSlot: format = [magic][version][size][data] */

#include "save.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#  include <direct.h>
#  define MAKE_DIR(p) _mkdir(p)
#else
#  include <sys/stat.h>
#  define MAKE_DIR(p) mkdir((p), 0755)
#endif

static void slot_path(char *buf, int bufSize, int slot) {
    snprintf(buf, bufSize, "%s/slot_%d.sav", SAVE_DIR, slot);
}

void EnsureSaveDir(void) {
    MAKE_DIR(SAVE_DIR);
}

bool SaveRaw(const void *data, size_t size, int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;
    EnsureSaveDir();

    char path[128];
    slot_path(path, sizeof(path), slot);

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint32_t magic   = SAVE_MAGIC;
    uint32_t version = SAVE_VERSION;
    uint32_t bytes   = (uint32_t)size;

    bool ok =
        fwrite(&magic,   sizeof(magic),   1, f) == 1 &&
        fwrite(&version, sizeof(version), 1, f) == 1 &&
        fwrite(&bytes,   sizeof(bytes),   1, f) == 1 &&
        fwrite(data,     1,         size, f) == size;

    fclose(f);
    if (!ok) remove(path);
    return ok;
}

bool LoadRaw(void *data, size_t size, int slot, uint32_t magic, uint32_t version) {
    if (slot < 0 || slot >= SAVE_SLOTS) return false;

    char path[128];
    slot_path(path, sizeof(path), slot);

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint32_t fMagic, fVersion, fBytes;
    bool ok =
        fread(&fMagic,   sizeof(fMagic),   1, f) == 1 &&
        fread(&fVersion, sizeof(fVersion), 1, f) == 1 &&
        fread(&fBytes,   sizeof(fBytes),   1, f) == 1;

    if (!ok || fMagic != magic || fVersion != version || fBytes != (uint32_t)size) {
        fclose(f);
        return false;
    }

    ok = fread(data, 1, size, f) == size;
    fclose(f);
    return ok;
}

SlotStatus CheckSlot(int slot, uint32_t magic, uint32_t version) {
    if (slot < 0 || slot >= SAVE_SLOTS) return SLOT_EMPTY;

    char path[128];
    slot_path(path, sizeof(path), slot);

    FILE *f = fopen(path, "rb");
    if (!f) return SLOT_EMPTY;

    uint32_t fMagic, fVersion, fBytes;
    bool ok =
        fread(&fMagic,   sizeof(fMagic),   1, f) == 1 &&
        fread(&fVersion, sizeof(fVersion), 1, f) == 1 &&
        fread(&fBytes,   sizeof(fBytes),   1, f) == 1;
    fclose(f);

    if (!ok || fMagic != magic || fVersion != version) return SLOT_CORRUPT;
    return SLOT_VALID;
}
