/* lpd79524.h
     $Id$

   written by Marc Singer
   12 Nov 2004

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

   The CPLD registers changed during development.  The ifdef disabled
   versions of the constants document those changes.  Most of them are
   unimportant for the Linux kernel and drivers that we support.

*/

#if !defined (__LPD79524_H__)
#    define   __LPD79524_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#define CPLD_INTR		(0x4c600000)
#define CPLD_CONTROL		(0x4c100000)
#define CPLD_SPI		(0x4c500000)
#define CPLD_INTERRUPT		(0x4c600000)
#define CPLD_FLASH		__REG16 (0x4c800000)
#define CPLD_REVISION		(0x4ca00000)
#define CPLD_EXGPIO		(0x4cb00000)
#define CPLD_GPIODATA		(0x4cc00000)
#define CPLD_GPIODIR		(0x4cd00000)

#define CPLD_FLASH_NANDSPD	(1<<6)
#define CPLD_FLASH_RDYnBSY	(1<<2)
#define CPLD_FLASH_FL_VPEN	(1<<0)
#define CPLD_FLASH_STS1		(1<<1)

#define CPLD_CONTROL_WRLAN_ENABLE (1<<0)

#define CPLD_SPI_CS_NCODEC	(1<<5)
#define CPLD_SPI_CS_MAC		(1<<4)
#define CPLD_SPI_CS_EEPROM	(1<<3)
#define CPLD_SPI_SCLK		(1<<2)
#define CPLD_SPI_TX_SHIFT	(1)
#define CPLD_SPI_TX		(1<<CPLD_SPI_TX_SHIFT)
#define CPLD_SPI_RX_SHIFT	(0)
#define CPLD_SPI_RX		(1<<CPLD_SPI_RX_SHIFT)

#define CPLD_INTR_NWRLAN	(1<<0)

#if 1 				/* New bits */
#define CPLD_INTR_IRQDMSK	(1<<1)
#define CPLD_INTR_WLANMSK	(1<<2)
#define CPLD_INTR_WLANMSK	(1<<2)
#define CPLD_INTR_USBMSK	(1<<3)
#define CPLD_INTR_USB_VBUS	(1<<4)
#define CPLD_INTR_NIRQD		(1<<5)
#define CPLD_INTR_NIRQC		(1<<6)
#define CPLD_INTR_IRQCMSK	(1<<7)
#else				/* Old bits */
#define CPLD_INTR_USB_VBUS	(1<<1)
#define CPLD_INTR_NIRQC		(1<<2)
#define CPLD_INTR_NIRQD		(1<<3)
#define CPLD_INTR_NSUSPEND	(1<<4)
#define CPLD_INTR_NSTANDBY	(1<<5)
#define CPLD_INTR_NRESET	(1<<6)
#define CPLD_INTR_NSWINT	(1<<7)
#endif

#if 1
#define CPLD_EXGPIO_GPIO1	(1<<0)
#define CPLD_EXGPIO_LED2	(1<<1)
#define CPLD_EXGPIO_LED1	(1<<2)
#else
#define CPLD_EXGPIO_LED1	(1<<1)
#define CPLD_EXGPIO_LED2	(1<<2)
#endif

//#define NOR_0_PHYS	(0x44000000)
//#define NOR_0_LENGTH	(8*1024*1024)
//#define NOR_1_PHYS	(0x45000000)
//#define NOR_1_LENGTH	(8*1024*1024)
//#define WIDTH		(16)	/* Device width in bits */

#if 1
# define CF_PHYS	0x48200000
#else
# define CF_PHYS	0x48100000
#endif

#endif  /* __LPD79524_H__ */
