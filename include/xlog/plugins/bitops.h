#ifndef __BITOPS_H
#define __BITOPS_H

#include <xlog/xlog_config.h>

/** bit operations for xlog development */
#ifndef BITS_PER_LONG
#error BITS_PER_LONG must be defined.
#endif

#define DIV_ROUND_UP(n,d)		(((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_LONG)
#define BIT_WORD(nr)			((nr) / BITS_PER_LONG)

/** bit mask */
#define BIT_MASK(nr)			(1UL << ((nr) & (BITS_PER_LONG - 1)))
#define BITS_MASK(lo, hi)		((BIT_MASK((hi) - (lo)) - 1) << ((lo) & (BITS_PER_LONG - 1)))
#define BITS_MASK_K(lo, hi, k)	(((BIT_MASK((hi) - (lo)) - 1) & (k)) << ((lo) & (BITS_PER_LONG - 1)))

/** bit(s) operation */
#define SET_BIT(v, nr)			(v |= BIT_MASK(nr))
#define CLR_BIT(v, nr)			(v &= ~BIT_MASK(nr))
#define REV_BIT(v, nr)			(v ^= ~BIT_MASK(nr))
#define CHK_BIT(v, nr)			(v & BIT_MASK(nr))
#define GET_BIT(v, nr)			(!!CHK_BIT(v, nr))
#define SET_BITS(v, lo, hi)		(v |= BITS_MASK(lo, hi))
#define CLR_BITS(v, lo, hi)		(v &= ~BITS_MASK(lo, hi))
#define REV_BITS(v, lo, hi)		(v ^= ~BITS_MASK(lo, hi))
#define CHK_BITS(v, lo, hi)		(v & BITS_MASK(lo, hi))
#define GET_BITS(v, lo, hi)		(CHK_BITS(v, lo, hi) >> ((lo) & (BITS_PER_LONG - 1)))
#define SETV_BITS(v, lo, hi, k)	(CLR_BITS(v, lo, hi), v |= BITS_MASK_K(lo, hi, k))

/** bit algorithms */
#define CLR_LoBIT(v)			((v) & ((v) - 1))
#define CLR_LoBITS(v)			((v) & ((v) + 1))
#define SET_LoBIT(v)			((v) | ((v) + 1))
#define SET_LoBITS(v)			((v) | ((v) - 1))
#define GET_LoBIT(v)			(v) & (~(v) + 1)

#define IS_POWER_OF_2(v)		(CLR_LoBIT(v) == 0)
#define IS_POWER_OF_4(v)		(CLR_LoBIT(v) == 0 && ((v) & 0x55555555))

#define AVERAGE(x, y)			(((x) & (y)) + (((x) ^ (y)) >> 1))

/** count how many bits set */
static inline unsigned int BITS_COUNT32(uint32_t x)
{
	x = ( x & 0x55555555 ) + ( ( x & 0xaaaaaaaa ) >> 1 );
	x = ( x & 0x33333333 ) + ( ( x & 0xcccccccc ) >> 2 );
	x = ( x & 0x0f0f0f0f ) + ( ( x & 0xf0f0f0f0 ) >> 4 );
	x = ( x & 0x00ff00ff ) + ( ( x & 0xff00ff00 ) >> 8 );
	x = ( x & 0x0000ffff ) + ( ( x & 0xffff0000 ) >> 16 );
	
	return x;
}

static inline unsigned int BITS_COUNT( unsigned long x )
{
	#if BITS_PER_LONG == 32
	return BITS_COUNT32( (uint32_t)x );
	#elif BITS_PER_LONG == 64
	return BITS_COUNT32( (uint32_t)x ) + BITS_COUNT32( (uint32_t)( x >> 32 ) );
	#endif
}

/** get position of first clear bit */
static inline int BITS_FLS( unsigned long v )
{
	int rv = BITS_PER_LONG;
	if( 0 == v ) {
		return 0;
	}
	
	#define __X(rv, x, mask, bits) if(0 == ((x) & (mask))) {(x) <<= (bits); rv -= (bits); }
	#if BITS_PER_LONG == 64
	__X( rv, v, 0xFFFFFFFF00000000u, 32 );
	#endif
	__X( rv, v, 0xFFFF0000u, 16 );
	__X( rv, v, 0xFF000000u,  8 );
	__X( rv, v, 0xF0000000u,  4 );
	__X( rv, v, 0xc0000000u,  2 );
	__X( rv, v, 0x80000000u,  1 );
	#undef __X
	
	return rv;
}

/** get position of first set bit */
static inline int BITS_FFS( unsigned long v )
{
	int rv = 1;
	if( 0 == v ) {
		return 0;
	}
	#define __X(rv, x, mask, bits) if(0 == ((x) & (mask))) {(x) >>= (bits); rv += (bits); }
	#if BITS_PER_LONG == 64
	__X( rv, v, 0xFFFFFFFF, 32 );
	#endif
	__X( rv, v, 0x0000FFFF, 16 );
	__X( rv, v, 0x000000FF,  8 );
	__X( rv, v, 0x0000000F,  4 );
	__X( rv, v, 0x00000003,  2 );
	__X( rv, v, 0x00000001,  1 );
	#undef __X
	
	return rv;
}

#endif
