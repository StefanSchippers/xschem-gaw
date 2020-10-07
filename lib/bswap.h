#ifndef B_SWAP_H
#define B_SWAP_H

/*
 * bswap.h - interface to byte swap macros
 * 
 * include LICENSE
 */

/*
 * File to be suppressed when all dists will define le16toh, ...
 */
#if defined(HAVE_ENDIAN_H) && defined(HAVE_BYTESWAP_H)

#include <endian.h>
#include <byteswap.h>

/* Definitions for Microsoft WAVE format */

#if __BYTE_ORDER == __LITTLE_ENDIAN

#  define cpu_to_be16(x) __bswap_16 (x)
#  define cpu_to_le16(x) (x)
#  define be16_to_cpu(x) __bswap_16 (x)
#  define le16_to_cpu(x) (x)

#  define cpu_to_be32(x) __bswap_32 (x)
#  define cpu_to_le32(x) (x)
#  define be32_to_cpu(x) __bswap_32 (x)
#  define le32_to_cpu(x) (x)

#  define cpu_to_be64(x) __bswap_64 (x)
#  define cpu_to_le64(x) (x)
#  define be64_to_cpu(x) __bswap_64 (x)
#  define le64_to_cpu(x) (x)

# elif __BYTE_ORDER == __BIG_ENDIAN

#  define cpu_to_be16(x) (x)
#  define cpu_to_le16(x) __bswap_16 (x)
#  define be16_to_cpu(x) (x)
#  define le16_to_cpu(x) __bswap_16 (x)

#  define cpu_to_be32(x) (x)
#  define cpu_to_le32(x) __bswap_32 (x)
#  define be32_to_cpu(x) (x)
#  define le32_to_cpu(x) __bswap_32 (x)

#  define cpu_to_be64(x) (x)
#  define cpu_to_le64(x) __bswap_64 (x)
#  define be64_to_cpu(x) (x)
#  define le64_to_cpu(x) __bswap_64 (x)

#else  /* __BYTE_ORDER == __LITTLE_ENDIAN */
#error "Wrong endian"
#endif /* __BYTE_ORDER == __LITTLE_ENDIAN */

#elif defined(HAVE_LIBKERN_OSBYTEORDER_H)

#include <libkern/OSByteOrder.h>

/* Definitions for Microsoft WAVE format */
#  define cpu_to_be16(x) OSSwapHostToBigInt16 (x)
#  define cpu_to_le16(x) OSSwapHostToLittleInt16 (x)
#  define be16_to_cpu(x) OSSwapBigToHostInt16 (x)
#  define le16_to_cpu(x) OSSwapLittleToHostInt16 (x)

#  define cpu_to_be32(x) OSSwapHostToBigInt32 (x)
#  define cpu_to_le32(x) OSSwapHostToLittleInt32 (x)
#  define be32_to_cpu(x) OSSwapBigToHostInt32 (x)
#  define le32_to_cpu(x) OSSwapLittleToHostInt32 (x)

#  define cpu_to_be64(x) OSSwapHostToBigInt64 (x)
#  define cpu_to_le64(x) OSSwapHostToLittleInt64 (x)
#  define be64_to_cpu(x) OSSwapBigToHostInt64 (x)
#  define le64_to_cpu(x) OSSwapLittleToHostInt64 (x)

#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>

/* Definitions for Microsoft WAVE format */
#  define cpu_to_be16(x) htobe16(x)
#  define cpu_to_le16(x) htole16 (x)
#  define be16_to_cpu(x) be16toh (x)
#  define le16_to_cpu(x) le16toh (x)

#  define cpu_to_be32(x) htobe32 (x)
#  define cpu_to_le32(x) htole32 (x)
#  define be32_to_cpu(x) be32toh (x)
#  define le32_to_cpu(x) le32toh (x)

#  define cpu_to_be64(x) htobe64 (x)
#  define cpu_to_le64(x) htole64 (x)
#  define be64_to_cpu(x) be64toh (x)
#  define le64_to_cpu(x) le64toh (x)

#else  /* defined(HAVE_ENDIAN_H) && defined(HAVE_BYTESWAP_H) */
#error "Failed to define byte swapping macros"
/* TODO: Implement fallback functions to detect endianness and implement byte swapping.*/
#endif /* defined(HAVE_ENDIAN_H) && defined(HAVE_BYTESWAP_H) */

#endif	/* B_SWAP_H */
