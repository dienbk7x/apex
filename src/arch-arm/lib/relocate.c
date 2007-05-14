/* relocate.c

   written by Marc Singer
   9 Nov 2004

   Copyright (C) 2004 Marc Singer

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

   Really, this is a relocator from NOR flash or some other directly
   mapped source.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <asm/cp15.h>

#include <debug_ll.h>

#if defined (CONFIG_DEBUG_LL)
//# define USE_LDR_COPY		/* Simpler copy loop, more free registers */
# define USE_SLOW_COPY		/* Simpler copy loop, more free registers */
# define USE_COPY_VERIFY
#else
# define USE_COPY_VERIFY	/* Verify all of the time */
#endif

//# define USE_LDR_COPY		/* Simpler copy loop, more free registers */

void relocate_apex_exit (void);

/* relocate_apex

   performs a memory move of the whole loader, presumed to be from NOR
   flash into SDRAM.  The LMA is determined at runtime.  The relocator
   will put the loader at the VMA and then return to the newly
   relocated return address.

   *** FIXME: it might be prudent to check for the VMA being greater
   *** than or less than the LMA.  As it stands, we depend on
   *** twos-complement arithmetic to make sure that offset works out
   *** when we jump to the relocated code.

   This function does not check for source overlapping destination.

   The value lr - offset is the source location of the return address.
   We should be able to jump to lr and continue with our work.

*/

void __naked __section (.apexrelocate) relocate_apex (unsigned long offset)
{
  unsigned long lr;

  PUTC_LL ('R');
  __asm volatile ("bl 0f\n\t"
               "0: str lr, [%0]\n\t"
		  :  "=&r" (lr)
		  :: "lr", "cc");

  PUTC_LL ('c');

  {
    unsigned long d = (unsigned long) &APEX_VMA_COPY_START;
    unsigned long s = (unsigned long) &APEX_VMA_COPY_START - offset;
#if defined USE_LDR_COPY
    unsigned long index
      = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 3 - 4) & ~3;
    unsigned long v;
    __asm volatile (
		    "0: ldr %3, [%0, %2]\n\t"
		       "str %3, [%1, %2]\n\t"
		       "subs %2, %2, #4\n\t"
		       "bpl 0b\n\t"
		       : "+r" (s),
			 "+r" (d),
			 "+r" (index),
			 "=&r" (v)
		       :: "cc"
		    );
#elif defined (USE_SLOW_COPY)
  __asm volatile (
	       "0: ldmia %0!, {r3}\n\t"
		  "stmia %1!, {r3}\n\t"
		  "cmp %1, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s),
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r3", "cc"
		  );
#else
  __asm volatile (
	       "0: ldmia %0!, {r3-r10}\n\t"
		  "stmia %1!, {r3-r10}\n\t"
		  "cmp %1, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s),
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "cc"
		  );
#endif
  }

  PUTC_LL ('\r');
  PUTC_LL ('\n');
  PUTHEX_LL (offset);
  PUTC_LL ('+');
  PUTHEX_LL (lr);
  PUTC_LL ('=');
  PUTC_LL ('>');
  PUTHEX_LL (*(unsigned long*) (lr - offset));	/* Original */
  PUTC_LL ('=');
  PUTC_LL ('=');
  PUTHEX_LL (*(unsigned long*) (lr));		/* Copy */
  PUTC_LL ('\r');
  PUTC_LL ('\n');

#if defined (USE_COPY_VERIFY)
  if (*(unsigned long*) lr != *(unsigned long*) (lr - offset)) {
    PUTC ('@');
    PUTC ('F');
    PUTC ('A');
    PUTC ('I');
    PUTC ('L');
    __asm volatile ("0: b 0b");
  }
#endif

  PUTC_LL ('j');

				/* Return to SDRAM */
  __asm volatile ("mov pc, %0" : : "r" (&relocate_apex_exit));
}

void __naked __section (.apexrelocate) relocate_apex_exit (void)
{
}
