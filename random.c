#include <random.h>
#include <stdlib.h>

#ifndef RAND_MAX
#  ifndef INT_MAX
#    define INT_MAX ((int)((1l << (8 * sizeof(int) - 1) - 1)))
#  endif
#  define RAND_MAX INT_MAX
#endif

#define MY_RANDOM 1

#if MY_RANDOM

#undef RAND_MAX
#define RAND_MAX 32767

static unsigned long seed = 1;

#ifdef __PUREC__
static unsigned long swap(unsigned long val) 0x4840; /* swap d0 */
#else
#define swap(val) ((val) >> 16)
#endif

#define rand() \
	(seed = seed * 22695477l + 1, (int)(swap(seed) & RAND_MAX))
#define srand(_seed) seed = _seed

#endif /* MY_RANDOM */

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID new_rand(_UWORD x)
{
	srand(x);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD get_rand(_WORD n)
{
	return rand() % n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD dice_roll(_WORD count, _WORD dice)
{
	_WORD value = 0;
	
	while (count-- > 0)
	{
		value += get_rand(dice) + 1;
	}
	return value;
}
