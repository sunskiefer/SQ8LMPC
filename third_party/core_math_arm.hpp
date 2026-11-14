// ARMv7 adaptation (SQ8LMPC) of the upstream MSVC shim; lets 32-bit ARM clang build third_party/core-math, force-included (/FI) into
// those files only; see SQ8L_CRMATH_SOURCES in CMakeLists.txt. The correctly rounded
// exp/log/sin/cos/tan are what make the port give identical results on every platform, so
// the sources themselves are left alone apart from the u128 typedef, which cannot be
// expressed any other way: MSVC has no 128-bit integer type on any architecture.
//
// Two things are provided here:
//   * the GCC/Clang builtins and attributes core-math uses, mapped to their MSVC or
//     standard-library equivalents;
//   * sq8l_u128, a 128-bit unsigned integer with the operators core-math needs.
//
// The files are compiled as C++20 under MSVC (they are C99 otherwise), because a type with
// operators is the only way to keep the arithmetic spelled as it is upstream.
#pragma once

#include <math.h>
#include <stdint.h>
#include <type_traits>

// core-math tests endianness with the GCC predefined macros, which MSVC does not define.
// It uses them for one thing only: the order in which the two 64-bit halves overlaying a
// u128 are declared, in the five unions that pun one against the other. That order is a
// property of the 128-bit type, and sq8l_u128 below stores its high half first, so these
// files must take their big-endian branch -- which also happens to be the one whose
// declaration order matches the designators in their tables, as C++20 requires and C
// does not. The host is of course still little-endian.
#undef __BYTE_ORDER__
#define __ORDER_LITTLE_ENDIAN__ 1234
#define __ORDER_BIG_ENDIAN__ 4321
#define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__   /* ARMv7 is little-endian; see note above */

// 128-bit unsigned integer. core-math reads the halves back by punning this against a
// struct of two uint64_t, so the member order here and the order those unions declare
// have to agree: high half first, which is what the __BYTE_ORDER__ choice above selects.
// Trivially default-constructible and standard-layout, so it is still a valid union member.
struct sq8l_u128 {
    uint64_t hi;
    uint64_t lo;

    sq8l_u128() = default;
    constexpr sq8l_u128(uint64_t v) noexcept : hi(0), lo(v) {}

    static constexpr sq8l_u128 make(uint64_t h, uint64_t l) noexcept {
        sq8l_u128 r;
        r.hi = h;
        r.lo = l;
        return r;
    }

    // Sign-extends, like a conversion from a signed integer to unsigned __int128.
    template <typename T>
    static constexpr sq8l_u128 widen(T v) noexcept {
        if constexpr (std::is_signed_v<T>) {
            if (v < 0) return make(~UINT64_C(0), static_cast<uint64_t>(static_cast<int64_t>(v)));
        }
        return sq8l_u128(static_cast<uint64_t>(v));
    }

    // Truncating, like a cast from unsigned __int128, and implicit for the same reason:
    // core-math assigns 128-bit expressions straight into uint64_t. This cannot silently
    // turn 128-bit arithmetic into 64-bit, because every operator below also has a
    // mixed-width form that is an exact match and therefore wins outright.
    constexpr operator uint64_t() const noexcept { return lo; }

    friend sq8l_u128 operator+(sq8l_u128 a, sq8l_u128 b) noexcept {
        const uint64_t lo = a.lo + b.lo;
        return make(a.hi + b.hi + (lo < a.lo), lo);
    }
    friend sq8l_u128 operator-(sq8l_u128 a, sq8l_u128 b) noexcept {
        return make(a.hi - b.hi - (a.lo < b.lo), a.lo - b.lo);
    }
    // Low 128 bits of the product, as a hardware 128-bit multiply gives.
    friend sq8l_u128 operator*(sq8l_u128 a, sq8l_u128 b) noexcept {
        // portable 64x64 -> 128 (32-bit ARM has no 128-bit type and no _umul128)
        const uint64_t a0 = a.lo & 0xffffffffu, a1 = a.lo >> 32, b0 = b.lo & 0xffffffffu, b1 = b.lo >> 32;
        const uint64_t p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
        const uint64_t mid = (p00 >> 32) + (p01 & 0xffffffffu) + (p10 & 0xffffffffu);
        const uint64_t low = (p00 & 0xffffffffu) | (mid << 32);
        const uint64_t high = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
        return make(high + a.hi * b.lo + a.lo * b.hi, low);
    }
    friend constexpr sq8l_u128 operator&(sq8l_u128 a, sq8l_u128 b) noexcept {
        return make(a.hi & b.hi, a.lo & b.lo);
    }
    friend constexpr sq8l_u128 operator|(sq8l_u128 a, sq8l_u128 b) noexcept {
        return make(a.hi | b.hi, a.lo | b.lo);
    }
    friend constexpr sq8l_u128 operator^(sq8l_u128 a, sq8l_u128 b) noexcept {
        return make(a.hi ^ b.hi, a.lo ^ b.lo);
    }
    constexpr sq8l_u128 operator~() const noexcept { return make(~hi, ~lo); }
    sq8l_u128 operator-() const noexcept { return sq8l_u128(0) - *this; }

    friend constexpr sq8l_u128 operator<<(sq8l_u128 a, int n) noexcept {
        if (n == 0) return a;
        if (n >= 128) return sq8l_u128(0);
        if (n >= 64) return make(a.lo << (n - 64), 0);
        return make((a.hi << n) | (a.lo >> (64 - n)), a.lo << n);
    }
    friend constexpr sq8l_u128 operator>>(sq8l_u128 a, int n) noexcept {
        if (n == 0) return a;
        if (n >= 128) return sq8l_u128(0);
        if (n >= 64) return sq8l_u128(a.hi >> (n - 64));
        return make(a.hi >> n, (a.lo >> n) | (a.hi << (64 - n)));
    }

    friend constexpr bool operator==(sq8l_u128 a, sq8l_u128 b) noexcept { return a.hi == b.hi && a.lo == b.lo; }
    friend constexpr bool operator!=(sq8l_u128 a, sq8l_u128 b) noexcept { return !(a == b); }
    friend constexpr bool operator<(sq8l_u128 a, sq8l_u128 b) noexcept {
        return a.hi != b.hi ? a.hi < b.hi : a.lo < b.lo;
    }
    friend constexpr bool operator>(sq8l_u128 a, sq8l_u128 b) noexcept { return b < a; }
    friend constexpr bool operator<=(sq8l_u128 a, sq8l_u128 b) noexcept { return !(b < a); }
    friend constexpr bool operator>=(sq8l_u128 a, sq8l_u128 b) noexcept { return !(a < b); }

    // Mixed-width forms. Without these, "u128 <op> uint64_t" is ambiguous: reaching the
    // operator above needs a conversion into sq8l_u128, reaching the built-in 64-bit
    // operator needs the conversion out of it, and the two rank equally. These are an
    // exact match on both operands, so they win and the arithmetic stays 128-bit.
#define SQ8L_U128_MIXED(op)                                                                        \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>                      \
    friend sq8l_u128 operator op(sq8l_u128 a, T b) noexcept {                                      \
        return a op widen(b);                                                                      \
    }                                                                                              \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>                      \
    friend sq8l_u128 operator op(T a, sq8l_u128 b) noexcept {                                       \
        return widen(a) op b;                                                                      \
    }
    SQ8L_U128_MIXED(+)
    SQ8L_U128_MIXED(-)
    SQ8L_U128_MIXED(*)
    SQ8L_U128_MIXED(&)
    SQ8L_U128_MIXED(|)
    SQ8L_U128_MIXED(^)
#undef SQ8L_U128_MIXED

#define SQ8L_U128_MIXED_CMP(op)                                                                    \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>                      \
    friend bool operator op(sq8l_u128 a, T b) noexcept {                                            \
        return a op widen(b);                                                                      \
    }                                                                                              \
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>                      \
    friend bool operator op(T a, sq8l_u128 b) noexcept {                                            \
        return widen(a) op b;                                                                      \
    }
    SQ8L_U128_MIXED_CMP(==)
    SQ8L_U128_MIXED_CMP(!=)
    SQ8L_U128_MIXED_CMP(<)
    SQ8L_U128_MIXED_CMP(>)
    SQ8L_U128_MIXED_CMP(<=)
    SQ8L_U128_MIXED_CMP(>=)
#undef SQ8L_U128_MIXED_CMP

    // Shift counts stay integers; only the shifted value is 128-bit.
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    friend sq8l_u128 operator<<(sq8l_u128 a, T n) noexcept {
        return a << static_cast<int>(n);
    }
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    friend sq8l_u128 operator>>(sq8l_u128 a, T n) noexcept {
        return a >> static_cast<int>(n);
    }

    sq8l_u128& operator+=(sq8l_u128 b) noexcept { return *this = *this + b; }
    sq8l_u128& operator-=(sq8l_u128 b) noexcept { return *this = *this - b; }
    sq8l_u128& operator*=(sq8l_u128 b) noexcept { return *this = *this * b; }
    sq8l_u128& operator&=(sq8l_u128 b) noexcept { return *this = *this & b; }
    sq8l_u128& operator|=(sq8l_u128 b) noexcept { return *this = *this | b; }
    sq8l_u128& operator^=(sq8l_u128 b) noexcept { return *this = *this ^ b; }
    sq8l_u128& operator<<=(int n) noexcept { return *this = *this << n; }
    sq8l_u128& operator>>=(int n) noexcept { return *this = *this >> n; }
};

static_assert(sizeof(sq8l_u128) == 16, "u128 must be 16 bytes to alias the union halves");
static_assert(std::is_standard_layout_v<sq8l_u128>, "u128 is punned against a struct of two uint64_t");
static_assert(std::is_trivially_default_constructible_v<sq8l_u128>, "u128 is used as a union member");


// clang's own builtins are used (clz/ctz/expect/fma/roundeven); only the u128 operand forms
// need forwarding because core-math narrows values such as (C >> 64) in the call itself.
static inline int sq8l_clzll(uint64_t x) { return __builtin_clzll(x); }
static inline int sq8l_clzll(sq8l_u128 x) { return __builtin_clzll(static_cast<uint64_t>(x)); }
static inline int sq8l_ctzll(uint64_t x) { return __builtin_ctzll(x); }
static inline int sq8l_ctzll(sq8l_u128 x) { return __builtin_ctzll(static_cast<uint64_t>(x)); }
#define __builtin_clzll sq8l_clzll
#define __builtin_ctzll sq8l_ctzll
