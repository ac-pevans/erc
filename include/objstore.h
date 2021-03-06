#ifndef _OBJSTORE_H_
#define _OBJSTORE_H_

#include <stdbool.h>

#include "apple2/mem.h"
#include "vm_bits.h"

typedef struct {
    char header[4];
    vm_8bit apple2_sys_rom[APPLE2_SYSROM_SIZE];
    vm_8bit apple2_peripheral_rom[APPLE2_PERIPHERAL_SIZE];
    vm_8bit apple2_sysfont[APPLE2_SYSFONT_SIZE];
    vm_8bit apple2_invfont[APPLE2_SYSFONT_SIZE];
} objstore;

extern bool objstore_ready();
extern int objstore_init();
extern void objstore_clear();

#define OBJSTORE_DECL(x) \
    const vm_8bit *objstore_##x()

OBJSTORE_DECL(apple2_peripheral_rom);
OBJSTORE_DECL(apple2_sys_rom);
OBJSTORE_DECL(apple2_sysfont);
OBJSTORE_DECL(apple2_invfont);

#endif
