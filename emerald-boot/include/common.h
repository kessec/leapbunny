#ifndef __BOOT_COMMON_H__
#define __BOOT_COMMON_H__

#include <stdint.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

#define REG8(addr)  *((volatile uint8_t *)(addr))
#define REG16(addr) *((volatile uint16_t *)(addr))
#define REG32(addr) *((volatile uint32_t *)(addr))

/* bit masking */
#define BIT_SET(v,b)    (v |= (1<<(b)))
#define BIT_CLR(v,b)    (v &= ~(1<<(b)))
#define IS_SET(v,b)     (v & (1<<(b)))
#define IS_CLR(v,b)     !(v & (1<<(b)))
#define BIT_MASK_ONES(b) ((1<<(b))-1)

#define NULL	0

#endif /* __BOOT_COMMON_H__ */
