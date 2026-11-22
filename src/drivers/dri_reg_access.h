#ifndef DRI_REG_ACCESS_H
#define DRI_REG_ACCESS_H

#include <cstdint>
constexpr uintptr_t DRI_REG_XOR = 0x1000;
constexpr uintptr_t DRI_REG_SET = 0x2000;
constexpr uintptr_t DRI_REG_CLR = 0x3000;

constexpr uint32_t mask(uint8_t shift, uint8_t len) {
    return (((1u << len) - 1) << shift);
}
#endif /* DRI_REG_ACCESS_H */