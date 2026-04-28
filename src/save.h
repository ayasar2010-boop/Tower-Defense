/* save.h — T56: Slot tabanlı kayıt/yükleme sistemi
 * Her slot save/slot_N.sav dosyasına binary olarak yazılır.
 * Magic + version header ile format doğrulaması yapılır. */

#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>
#include <stdint.h>

#define SAVE_MAGIC   0x52554C45u /* "RULE" */
#define SAVE_VERSION 2u
#define SAVE_SLOTS   3
#define SAVE_DIR     "saves"

/* Slotun boş/dolu/bozuk durumu */
typedef enum {
    SLOT_EMPTY = 0,
    SLOT_VALID,
    SLOT_CORRUPT
} SlotStatus;

/* Ham dosya I/O — gerçek struct Game ve tüm tipler main.c'de tanımlı olduğu için
 * bu fonksiyonlar void* üzerinden çalışır ve boyutu main.c'den alır. */
bool SaveRaw(const void *data, size_t size, int slot);
bool LoadRaw(void *data, size_t size, int slot, uint32_t magic, uint32_t version);
SlotStatus CheckSlot(int slot, uint32_t magic, uint32_t version);
void EnsureSaveDir(void);

#endif
