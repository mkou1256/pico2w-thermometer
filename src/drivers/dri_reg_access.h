#ifndef DRI_REG_ACCESS_H
#define DRI_REG_ACCESS_H

#include <cstdint>
constexpr uintptr_t DRI_REG_XOR = 0x1000;
constexpr uintptr_t DRI_REG_SET = 0x2000;
constexpr uintptr_t DRI_REG_CLR = 0x3000;

inline void reg_set(volatile uint32_t* addr, uint32_t mask) {
    volatile uint32_t* reg = (volatile uint32_t*) ((uintptr_t) addr + DRI_REG_SET);
    *reg = mask;
}

inline void reg_clr(volatile uint32_t* addr, uint32_t mask) {
    volatile uint32_t* reg = (volatile uint32_t*) ((uintptr_t) addr + DRI_REG_CLR);
    *reg = mask;
}

inline void reg_xor(volatile uint32_t* addr, uint32_t mask) {
    volatile uint32_t* reg = (volatile uint32_t*) ((uintptr_t) addr + DRI_REG_XOR);
    *reg = mask;
}

inline void reg_write_masked(volatile uint32_t* addr, uint32_t values, uint32_t mask) {
    reg_xor(addr, (*addr ^ values) & mask);
}

constexpr uint32_t make_mask(uint8_t shift, uint8_t len) {
    if (len >= 32) return 0xFFFFFFFF;
    return (((1u << len) - 1) << shift);
}
#endif /* DRI_REG_ACCESS_H */