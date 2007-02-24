/* mmu.h

   written by Marc Singer
   19 Dec 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

   The macros are here mainly for the MMU and cache control specific
   code.  Functions within the body of the program should use function
   calls instead of the macros, but this may not be feasible depending
   on the nature of thos functions.

   Also keep in mind that some of these macros are *not* available on
   some architectures.  The specific mmu-*.h should be written to
   undefine those macros that are unavailable.

*/

#if !defined (__MMU_H__)
#    define   __MMU_H__

/* ----- Includes */

#include <linux/types.h>

	/* ---- Cache control */

#define INVALIDATE_ICACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 0\n\t" :: "r" (0))
#define INVALIDATE_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (a))
#define INVALIDATE_ICACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 2\n\t" :: "r" (i))

#define INVALIDATE_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0\n\t" :: "r" (0))
#define INVALIDATE_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 1\n\t" :: "r" (a))
#define INVALIDATE_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 2\n\t" :: "r" (i))

#define INVALIDATE_CACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 0\n\t" :: "r" (0))
#define INVALIDATE_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 1\n\t" :: "r" (a))
#define INVALIDATE_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 2\n\t" :: "r" (i))

#define CLEAN_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t" :: "r" (a))
#define CLEAN_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 2\n\t" :: "r" (i))
#define DRAIN_WRITE_BUFFER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4\n\t" :: "r" (0))

#define CLEAN_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c11, 1\n\t" :: "r" (a))
#define CLEAN_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c11, 2\n\t" :: "r" (i))

#define PREFETCH_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c13, 1\n\t" :: "r" (a))

#define CLEAN_INV_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 1\n\t" :: "r" (a))
#define CLEAN_INV_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 2\n\t" :: "r" (i))

#define CLEAN_INV_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c15, 1\n\t" :: "r" (a))
#define CLEAN_INV_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c15, 2\n\t" :: "r" (i))

	/* ---- Cache lockdown */

#define UNLOCK_CACHE\
  __asm volatile ("mcr p15, 0, %0, c9, c1, 1\n\t"\
		  "mcr p15, 0, %0, c9, c2, 1\n\t" :: "r" (0))

	/* ---- TLB control */

#define INVALIDATE_TLB\
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))
#define INVALIDATE_ITLB_VA(a)\
    __asm volatile ("mcr p15, 0, %0, c8, c5, 1\n\t" :: "r" (a))
#define INVALIDATE_DTLB_VA(a)\
    __asm volatile ("mcr p15, 0, %0, c8, c6, 1\n\t" :: "r" (a))


	/* ----- Architecture specific functions */

#if defined (CONFIG_CPU_ARMV4)
# include <asm/mmu-armv4.h>
#endif

#if defined (CONFIG_CPU_XSCALE)
# include <asm/mmu-xscale.h>
#endif

#if !defined (CP15_WAIT)
# define CP15_WAIT
#endif

void* alloc_uncached (size_t cb, size_t alignment);
void* alloc_uncached_top_retain (size_t cb, size_t alignment);
void mmu_protsegment (void* pv, int cacheable, int bufferable);

#endif  /* __MMU_H__ */
