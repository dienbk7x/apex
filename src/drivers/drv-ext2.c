/* drv-ext2.c
     $Id$

   written by Marc Singer
   22 Feb 2005

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
   
   NOTES
   -----

   o Thanks to John,
     http://uranus.it.swin.edu.au/~jn/explore2fs/es2fs.htm, for
     compiling much of the information used to write this driver.
   o Thanks to the GRUB project for implementing an ext2fs reader.

   o Partitions.  The partition support is redundant here.  We may
     want to implement a translation driver to handle the partition
     stuff.  Later.

   o There is no inode 0, so no room is left for it in the on-disk
     data structures.  
   o The filesystem image starts with the superblock and is
     immediately followed by an array of block_group structures.
     These data are replicated in each block group.
   o The first superblock begins in the first sector, or in the third,
     depending on how the filesystem was initialized.  This skipped
     1KiB only occurs for the first superblock and is never accounted
     any place else.
   o Filesystem blocks are counted from zero starting at the beginning
     of the filesystem partition.
   o The bitmaps for the inodes and the used blocks are not
     replicated, but are specific to each block group.
   o As a simplification, we may elect not to follow an indirect block
     on a filesystem search...I don't know.  We have to do it to read
     a file any way.  We need a good way to handle this.
   o Since we know where we are going, it may be advantageous to only
     read the superblock in it's entirety.  The other small
     structures, e.g. block_group and inode, can be read individually,
     letting the underlying driver deal with edge conditions.
     Directories, probably need to be read in block sized chunks so
     that we don't have to deal with boundary conditions.
   o Thus, a file read is <DIR_MODE>inode = root; block_group; inode;
     <FILE_MODE> data_block[n]; search for path element; resolve; inode =
     new_inode; repeat; <DIR_MODE> if more path elements, <DATA_MODE>
     if file found. 
   o Symlinks only introduce a slight change to the system in that
     they may adjust the patch we're searching for.
   o Is it worth adding another layer to handle the path search logic?
     Can it even be done? 

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <spinner.h>
#include <linux/kernel.h>
#include <error.h>

#define TALK

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

#define DRIVER_NAME	"ext2fs"

#define MAGIC_EXT2	0xef53

#define SECTOR_SIZE	512
#define BLOCK_SIZE	1024	/* Of questionable value */

#define BLOCK_SIZE_MAX	4096	/* Largest block buffer we support */

#if 0
/* Defined in kernel headers */
typedef __signed__ char  __s8;
typedef unsigned   char  __u8;
typedef __signed__ short __s16;
typedef unsigned   short __u16;
typedef __signed__ int	 __s32;
typedef unsigned   int	 __u32;
#endif

struct partition {
  unsigned char boot;
  unsigned char begin_chs[3];
  unsigned char type;
  unsigned char end_chs[3];
  unsigned long start;
  unsigned long length;
};

struct superblock {
  __u32 s_inodes_count;		/* Total inodes */
  __u32 s_blocks_count;		/* Total blocks */
  __u32	s_r_blocks_count;	/* Count of reserved blocks */
  __u32 s_free_blocks_count;	/* Total free blocks */
  __u32 s_free_inodes_count;	/* Total of free inodes */
  __u32 s_first_data_block;	/* Index of first data block (0|1) */
  __u32 s_log_block_size;	/* 2^(10+n) count of bytes in a block */
  __s32 s_log_frag_size;	/* Size of the fragments ?? */
  __u32 s_blocks_per_group;	/* Number of blocks in a block group */
  __u32 s_frags_per_group;	/* Number of fragments in a block group */
  __u32 s_inodes_per_group;	/* Number of inodes inn a block group */
  __u32 s_mtime;		/* Last filesystem modification time */
  __u32 s_wtime;		/* Last filesystem write time */
  __u16 s_mnt_count;		/* Count of filesystem mounts */
  __s16 s_max_mnt_count;	/* Mount limit before forcing a check */
  __u16 s_magic;		/* EXT2 magic number (0xef53) */
  __u16 s_state;		/* Filesystem state */
  __u16 s_errors;		/* Error reporting mode */
  __u16 s_pad;			/* Padding */
  __u32 s_lastcheck;		/* Last filesystem check time */
  __u32 s_checkinterval;	/* Maximum interval between checks */
  __u32 s_creator_os;		/* OS that created filesystem */
  __u32 s_rev_level;		/* Filesystem revision number */
  __u16 s_def_resuid;		/* Default uid for reserved blocks */
  __u16 s_def_resgid;		/* Default gid for reserved blocks */
  __u32 s_reserved[235];	/* Padding */
};

struct block_group {
  __u32 bg_block_bitmap;	/* Group's block bitmap block address */
  __u32 bg_inode_bitmap;	/* Group's inode bitmap block address */
  __u32 bg_inode_table;		/* Group's inode table block address */
  __u16 bg_free_blocks_count;	/* Group's free block total */
  __u16 bg_free_inodes_count;	/* Group's free inode total */
  __u16 bg_used_dirs_count;	/* Group's used directory total */
  __u16 bg_pad;			/* Padding */
  __u32 bg_reserved[3];		/* Padding */
}; 

struct inode {
  __u16 i_mode;			/* File mode */
  __u16 i_uid;			/* Owner Uid */
  __u32 i_size;			/* Size in bytes */
  __u32 i_atime;		/* Access time */
  __u32 i_ctime;		/* Creation time */
  __u32 i_mtime;		/* Modification time */
  __u32 i_dtime;		/* Deletion Time */
  __u16 i_gid;			/* Group Id */
  __u16 i_links_count;		/* Links count */
  __u32 i_blocks;		/* Blocks count */
  __u32 i_flags;		/* File flags */
  union {
    struct {
      __u32 l_i_reserved1;
    } linux1;
    struct {
      __u32 h_i_translator;
    } hurd1;
    struct {
      __u32 m_i_reserved1;
    } masix1;
  } osd1;			/* OS dependent 1 */
  __u32 i_block[15];		/* Pointers to blocks */
  __u32 i_version;		/* File version (for NFS) */
  __u32 i_file_acl;		/* File ACL */
  __u32 i_dir_acl;		/* Directory ACL */
  __u32 i_faddr;		/* Fragment address */

  union {
    struct {
      __u8 l_i_frag;		/* Fragment number */
      __u8 l_i_fsize;		/* Fragment size */
      __u16 i_pad1;
      __u32 l_i_reserved2[2];
    } linux2;
    struct {
      __u8 h_i_frag;		/* Fragment number */
      __u8 h_i_fsize;		/* Fragment size */
      __u16 h_i_mode_high;
      __u16 h_i_uid_high;
      __u16 h_i_gid_high;
      __u32 h_i_author;
    } hurd2;
    struct {
      __u8 m_i_frag;		/* Fragment number */
      __u8 m_i_fsize;		/* Fragment size */
      __u16 m_pad1;
      __u32 m_i_reserved2[2];
    } masix2;
  } osd2;			/* OS dependent 2 */
  //  __u8   i_frag;		/* Fragment number */
  //  __u8   i_fsize;		/* Fragment size */
  //  __u16 i_pad1;		/* Padding */
  //  __u32  i_reserved2[2];	/* Padding */
};

enum {
  EXT2_BAD_INO		= 1,	/* Bad blocks inode */
  EXT2_ROOT_INO		= 2,	/* Root inode */
  EXT2_ACL_IDX_INO 	= 3,	/* ACL inode */
  EXT2_ACL_DATA_INO	= 4, 	/* ACL inode */
  EXT2_BOOT_LOADER_INO	= 5, 	/* Boot loader inode */
  EXT2_UNDEL_DIR_INO    = 6, 	/* Undelete directory inode */
  EXT2_FIRST_INO	= 11,	/* First non reserved inode */
};

#define EXT2_FILENAME_LENGTH_MAX 255

struct directory {
  __u32 inode;			/* File inode */
  __u16 rec_len;		/* Size of whole directory record */
  __u8  name_len;		/* Size of filename */
  __u8  file_type;
  char name[EXT2_FILENAME_LENGTH_MAX];
};

struct ext2_info {
  struct descriptor_d d;	/* Descriptor for underlying driver */

  int fOK;
  struct partition partition[4];
  struct superblock superblock;
//  struct block_group block_group[200];	/* Cached block group info */
  int block_size;

//  char rgb[BLOCK_SIZE_MAX];	/* I/O buffer -- may not be needed */

  int inode;			/* Current inode, a file or a directory */ 
  

};

static struct ext2_info ext2;
static const char szBlockDriver[] = "cf"; /* Underlying block driver */

inline int block_groups (struct ext2_info* ext2)
{
  return (ext2->superblock.s_blocks_count
	  + ext2->superblock.s_blocks_per_group - 1)
    /ext2->superblock.s_blocks_per_group;
}

inline int group_from_inode (struct ext2_info* ext2, int inode)
{
  return (inode - 1)/ext2->superblock.s_inodes_per_group;
}


/* ext2_identify

   performs an initial read on the device to get partition
   information.

*/

static int ext2_identify (void)
{
  int result;
  char sz[128];
  struct descriptor_d d;

  snprintf (sz, sizeof (sz), "%s:+1s", szBlockDriver);
  if (   (result = parse_descriptor (sz, &d))
      || (result = open_descriptor (&d))) 
    return result;


  /* Check for signature */
  {
    unsigned char rgb[2];

    d.driver->seek (&d, SECTOR_SIZE - 2, SEEK_SET);
    if (d.driver->read (&d, &rgb, 2) != 2
	|| rgb[0] != 0x55
	|| rgb[1] != 0xaa) {
      close_descriptor (&d);
      return -1;
    }
  }

  d.driver->seek (&d, SECTOR_SIZE - 66, SEEK_SET);
  d.driver->read (&d, ext2.partition, 16*4);

  close_descriptor (&d);

  return 0;
}

static int ext2_open (struct descriptor_d* d)
{
  int result = 0;
  char sz[128];

  ENTRY (0);

  ext2.fOK = 0;

  if ((result = ext2_identify ()))
    return result;
  ext2.fOK = 1;

#if defined (TALK)
  PRINTF ("descript %d %d\n", d->c, d->iRoot);
  {
    int i;
    for (i = 0; i < d->c; ++i)
      PRINTF ("  %d: (%s)\n", i, d->pb[i]);
  }
#endif

  {
    int partition = 0;
    if (d->iRoot > 0 && d->c)
      partition = simple_strtoul (d->pb[0], NULL, 10) - 1;
    if (partition < 0 || partition > 3 
		      || ext2.partition[partition].length == 0)
      ERROR_RETURN (ERROR_BADPARTITION, "invalid partition"); 

    snprintf (sz, sizeof (sz), "%s:%lds+%lds", 
	      szBlockDriver, 
	      ext2.partition[partition].start, 
	      ext2.partition[partition].length);
  }

  PRINTF ("  opening %s\n", sz);

	/* Open descriptor for the partition */
  if (   (result = parse_descriptor (sz, &ext2.d))
      || (result = open_descriptor (&ext2.d))) 
    return result;

	/* Default for superblock is 1 1KiB block into the partition */
  ext2.d.driver->seek (&ext2.d, BLOCK_SIZE, SEEK_SET);
  if (ext2.d.driver->read (&ext2.d, &ext2.superblock, 
			   sizeof (ext2.superblock)) 
      != sizeof (ext2.superblock)
      || ext2.superblock.s_magic != MAGIC_EXT2) {
    close_descriptor (&ext2.d); 
    return -1;
  }
  
  ext2.block_size = 1 << (ext2.superblock.s_log_block_size + 10);

#if 0
	/* Read block group control structures */
  ext2.d.driver->seek (&ext2.d, 2*BLOCK_SIZE, SEEK_SET);
  if (ext2.d.driver->read (&ext2.d, &ext2.block_group, 
			   sizeof (ext2.block_group)) 
      != sizeof (ext2.block_group)) {
    close_descriptor (&ext2.d); 
    return -1;
  }    
#endif

  return 0;
}

static void ext2_close (struct descriptor_d* d)
{
}

static ssize_t ext2_read (struct descriptor_d* d, void* pv, size_t cb)
{
  return 0;
}

static void ext2_report (void)
{
  int i;

  if (!ext2.fOK) {
    if (ext2_identify ())
      return;
    ext2.fOK = 1;
  }

  printf ("  ext2:\n"); 
  for (i = 0; i < 4; ++i)
    if (ext2.partition[i].type)
      printf ("          partition %d: %c %02x 0x%08lx 0x%08lx\n", i, 
	      ext2.partition[i].boot ? '*' : ' ',
	      ext2.partition[i].type,
	      ext2.partition[i].start,
	      ext2.partition[i].length);

  printf ("          total (i/b) %d/%d  free %d/%d  group %d/%d\n", 
	  ext2.superblock.s_inodes_count,
	  ext2.superblock.s_blocks_count,
	  ext2.superblock.s_free_inodes_count,
	  ext2.superblock.s_free_blocks_count,
	  ext2.superblock.s_inodes_per_group,
	  ext2.superblock.s_blocks_per_group
	  );
  printf ("          first_data %d  block_size %d  groups %d\n", 
	  ext2.superblock.s_first_data_block, 
	  ext2.block_size,
	  block_groups (&ext2));
#if 0
  printf ("          group 0: inode block %d  free %d/%d\n", 
	  ext2.block_group[0].bg_inode_table,
	  ext2.block_group[0].bg_free_inodes_count,
	  ext2.block_group[0].bg_free_blocks_count);
#endif
}

static __driver_6 struct driver_d ext2_driver = {
  .name = DRIVER_NAME,
  .description = "Ext2 filesystem driver",
  .flags = DRIVER_DESCRIP_FS,
  .open = ext2_open,
  .close = ext2_close,
  .read = ext2_read,
//  .write = cf_write,
//  .erase = cf_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d ext2_service = {
#if !defined (CONFIG_SMALL)
  .report = ext2_report,
#endif
};