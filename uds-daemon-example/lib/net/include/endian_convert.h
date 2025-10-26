/*
 * This file is part of the EMBTOM project
 * Copyright (c) ...
 */

#ifndef ENDIAN_CONVERT_H
#define ENDIAN_CONVERT_H

#include <byteswap.h>
#include <concepts>
#include <cstdint>
#include <endian.h> // __BYTE_ORDER, __LITTLE_ENDIAN
#include <type_traits>

namespace net {

//*****************************************************************************
//! \brief Concept for supported arithmetic types
//*****************************************************************************
template <typename T>
concept EndianConvertible = std::is_arithmetic_v<T> &&
                            (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

//*****************************************************************************
//! \brief host_to_network
//*****************************************************************************
template <EndianConvertible T>
constexpr T host_to_network(T value) noexcept
{
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(value)));
#else
        return value;
#endif
    } else if constexpr (sizeof(T) == 4) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(value)));
#else
        return value;
#endif
    } else if constexpr (sizeof(T) == 8) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(value)));
#else
        return value;
#endif
    }
}

//*****************************************************************************
//! \brief network_to_host (symmetric)
//*****************************************************************************
template <EndianConvertible T>
constexpr T network_to_host(T value) noexcept
{
    return host_to_network(value);
}

} // namespace net

#endif /* ENDIAN_CONVERT_H */
