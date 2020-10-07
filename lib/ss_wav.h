#ifndef SS_WAV_H
#define SS_WAV_H

/*
 * ss_wav.h - routines for SpiceStream that handle the ".wav" file format
 * 
 * include LICENSE
 */

#include <sys/types.h> /* Required by BSD derivates for u_short, u_int, etc. */
#include <bswap.h>

/* Definitions for Microsoft WAVE format */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#else
#error "Wrong endian"
#endif

#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')

/* WAVE fmt block constants from Microsoft mmreg.h header */
#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe

/* Used with WAV_FMT_EXTENSIBLE format */
#define WAV_GUID_TAG		"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71"

/*
 * it's in chunks like .voc and AMIGA iff, but my source say there
 * are in only in this combination, so I combined them in one header;
 * it works on all WAVE-file I have
 */
typedef struct {
   u_int magic;		/* 'RIFF' */
   u_int length;	/* filelen */
   u_int type;		/* 'WAVE' */
} WaveHeader;

typedef struct {
   u_short format;	/* see WAV_FMT_* */
   u_short channels;
   u_int sample_fq;	/* frequence of sample */
   u_int byte_p_sec;
   u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
   u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
   WaveFmtBody format;
   u_short ext_size;
   u_short bit_p_spl;
   u_int channel_mask;
   u_short guid_format;	/* WAV_FMT_* */
   u_char guid_tag[14];	/* WAV_GUID_TAG */
} WaveFmtExtensibleBody;

typedef struct {
   u_int type;		/* 'data' */
   u_int length;	/* samplecount */
} WaveChunkHeader;


#endif	/* SS_WAV_H */
