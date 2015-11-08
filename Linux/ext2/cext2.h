
#ifndef CEXT2_H
#define CEXT2_H

#include "common/Types.h"

/* Encoding of the file mode.  */

#define EXT2__S_IFMT        0170000 /* These bits determine file type.  */

/* File types.  */
#define EXT2__S_IFDIR       0040000 /* Directory.  */
#define EXT2__S_IFCHR       0020000 /* Character device.  */
#define EXT2__S_IFBLK       0060000 /* Block device.  */
#define EXT2__S_IFREG       0100000 /* Regular file.  */
#define EXT2__S_IFIFO       0010000 /* FIFO.  */
#define EXT2__S_IFLNK       0120000 /* Symbolic link.  */
#define EXT2__S_IFSOCK      0140000 /* Socket.  */

/* Protection bits.  */

#define EXT2__S_ISUID       04000   /* Set user ID on execution.  */
#define EXT2__S_ISGID       02000   /* Set group ID on execution.  */
#define EXT2__S_ISVTX       01000   /* Save swapped text after use (sticky).  */
#define EXT2__S_IREAD       0400    /* Read by owner.  */
#define EXT2__S_IWRITE      0200    /* Write by owner.  */
#define EXT2__S_IEXEC       0100    /* Execute by owner.  */

#define EXT2_NDIR_BLOCKS                12
#define EXT2_IND_BLOCK                  EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK                 (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK                 (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS                   (EXT2_TIND_BLOCK + 1)
#define EXT2_DEFAULT_PREALLOC_BLOCKS    8
#define EXT2_BAD_INO             1      /* Bad blocks inode */
#define EXT2_ROOT_INO            2      /* Root inode */
#define EXT2_BOOT_LOADER_INO     5      /* Boot loader inode */
#define EXT2_UNDEL_DIR_INO       6      /* Undelete directory inode */
#define EXT2_GOOD_OLD_FIRST_INO 11
#define EXT2_SUPER_MAGIC        0xEF53
#define EXT2_LINK_MAX           32000
#define EXT2_MIN_BLOCK_SIZE             1024
#define EXT2_MAX_BLOCK_SIZE             4096
#define EXT2_MIN_BLOCK_LOG_SIZE           10
#define EXT2_MIN_FRAG_SIZE              1024
#define EXT2_MAX_FRAG_SIZE              4096
#define EXT2_MIN_FRAG_LOG_SIZE            10
/*
 * Inode flags
 */
#define EXT2_SECRM_FL                   0x00000001 /* Secure deletion */
#define EXT2_UNRM_FL                    0x00000002 /* Undelete */
#define EXT2_COMPR_FL                   0x00000004 /* Compress file */
#define EXT2_SYNC_FL                    0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL               0x00000010 /* Immutable file */
#define EXT2_APPEND_FL                  0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL                  0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL                 0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT2_DIRTY_FL                   0x00000100
#define EXT2_COMPRBLK_FL                0x00000200 /* One or more compressed clusters */
#define EXT2_NOCOMP_FL                  0x00000400 /* Don't compress */
#define EXT2_ECOMPR_FL                  0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define EXT2_BTREE_FL                   0x00001000 /* btree format dir */
#define EXT2_INDEX_FL                   0x00001000 /* hash-indexed directory */
#define EXT2_IMAGIC_FL                  0x00002000 /* AFS directory */
#define EXT2_JOURNAL_DATA_FL            0x00004000 /* Reserved for ext3 */
#define EXT2_NOTAIL_FL                  0x00008000 /* file tail should not be merged */
#define EXT2_DIRSYNC_FL                 0x00010000 /* dirsync behaviour (directories only) */
#define EXT2_TOPDIR_FL                  0x00020000 /* Top of directory hierarchies*/
#define EXT2_RESERVED_FL                0x80000000 /* reserved for ext2 lib */

#define EXT2_FL_USER_VISIBLE            0x0003DFFF /* User visible flags */
#define EXT2_FL_USER_MODIFIABLE         0x000380FF /* User modifiable flags */

#define EXT2_VALID_FS                   0x0001  /* Unmounted cleanly */
#define EXT2_ERROR_FS                   0x0002  /* Errors detected */

#define EXT2_MOUNT_CHECK                0x000001  /* Do mount-time checks */
#define EXT2_MOUNT_OLDALLOC             0x000002  /* Don't use the new Orlov allocator */
#define EXT2_MOUNT_GRPID                0x000004  /* Create files with directory's group */
#define EXT2_MOUNT_DEBUG                0x000008  /* Some debugging messages */
#define EXT2_MOUNT_ERRORS_CONT          0x000010  /* Continue on errors */
#define EXT2_MOUNT_ERRORS_RO            0x000020  /* Remount fs ro on errors */
#define EXT2_MOUNT_ERRORS_PANIC         0x000040  /* Panic on errors */
#define EXT2_MOUNT_MINIX_DF             0x000080  /* Mimics the Minix statfs */
#define EXT2_MOUNT_NOBH                 0x000100  /* No buffer_heads */
#define EXT2_MOUNT_NO_UID32             0x000200  /* Disable 32-bit UIDs */
#define EXT2_MOUNT_XATTR_USER           0x004000  /* Extended user attributes */
#define EXT2_MOUNT_POSIX_ACL            0x008000  /* POSIX Access Control Lists */
#define EXT2_MOUNT_XIP                  0x010000  /* Execute in place */

#define EXT2_DFL_MAX_MNT_COUNT          20      /* Allow 20 mounts */
#define EXT2_DFL_CHECKINTERVAL          0       /* Don't use interval check */

#define EXT2_ERRORS_CONTINUE            1       /* Continue execution */
#define EXT2_ERRORS_RO                  2       /* Remount fs read-only */
#define EXT2_ERRORS_PANIC               3       /* Panic */
#define EXT2_ERRORS_DEFAULT             EXT2_ERRORS_CONTINUE

#define EXT2_OS_LINUX           0
#define EXT2_OS_HURD            1
#define EXT2_OS_MASIX           2
#define EXT2_OS_FREEBSD         3
#define EXT2_OS_LITES           4

#define EXT2_GOOD_OLD_REV       0       /* The good old (original) format */
#define EXT2_DYNAMIC_REV        1       /* V2 format w/ dynamic inode sizes */

#define EXT2_CURRENT_REV        EXT2_GOOD_OLD_REV
#define EXT2_MAX_SUPP_REV       EXT2_DYNAMIC_REV

#define EXT2_GOOD_OLD_INODE_SIZE 128
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC        0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES       0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL         0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR            0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INO          0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX           0x0020
#define EXT2_FEATURE_COMPAT_ANY                 0xffffffff

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER     0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE       0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR        0x0004
#define EXT2_FEATURE_RO_COMPAT_ANY              0xffffffff

#define EXT2_FEATURE_INCOMPAT_COMPRESSION       0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE          0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER           0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV       0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG           0x0010
#define EXT2_FEATURE_INCOMPAT_ANY               0xffffffff

#define EXT2_FEATURE_COMPAT_SUPP        EXT2_FEATURE_COMPAT_EXT_ATTR
#define EXT2_FEATURE_INCOMPAT_SUPP      (EXT2_FEATURE_INCOMPAT_FILETYPE| \
                                         EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_RO_COMPAT_SUPP     (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
                                         EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
                                         EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT2_FEATURE_RO_COMPAT_UNSUPPORTED      ~EXT2_FEATURE_RO_COMPAT_SUPP
#define EXT2_FEATURE_INCOMPAT_UNSUPPORTED       ~EXT2_FEATURE_INCOMPAT_SUPP

/*
 * Default values for user and/or group using reserved blocks
 */
#define EXT2_DEF_RESUID         0
#define EXT2_DEF_RESGID         0

/*
 * Default mount options
 */
#define EXT2_DEFM_DEBUG         0x0001
#define EXT2_DEFM_BSDGROUPS     0x0002
#define EXT2_DEFM_XATTR_USER    0x0004
#define EXT2_DEFM_ACL           0x0008
#define EXT2_DEFM_UID16         0x0010
    /* Not used by ext2, but reserved for use by ext3 */
#define EXT3_DEFM_JMODE         0x0060
#define EXT3_DEFM_JMODE_DATA    0x0020
#define EXT3_DEFM_JMODE_ORDERED 0x0040
#define EXT3_DEFM_JMODE_WBACK   0x0060

#define EXT2_NAME_LEN 255

#define EXT2_DIR_PAD                    4
#define EXT2_DIR_ROUND                  (EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(name_len)      (((name_len) + 8 + EXT2_DIR_ROUND) & \
                                         ~EXT2_DIR_ROUND)

// BEGIN CEXT2 namespace
namespace CEXT2 {
////////////////////////////////////////////////

// use these macros with the structure members i.e
// CEXT2_32(super.s_magic) to avoid endianness problems
#define CEXT2_16(x)			(llei16((void*)(&x)))
#define CEXT2_32(x)			(llei32((void*)(&x)))
#define CEXT2_64(x)			(llei64((void*)(&x)))

static char *levs[] = {
	"PANIC   ",	"CRITICAL",	"ERROR   ",	"WARNING ",
	"INFO    ",	"DEBUG   ",	"        ",
};

class reader
{
public:
	enum CommentLevel {		// 0 is most severe, 5 is least
		C_PANIC=0,
		C_CRITICAL=1,
		C_ERROR=2,
		C_WARNING=3,
		C_INFO=4,
		C_DEBUG=5,
	};
public:	// initializer and deinitializer functions
	int init();
	int freeMe();
public:	// overrideable methods for reading the "disk"
	/* int blocksize()
	   returns:		the size in bytes of one block (usually 512) */
	virtual int				blocksize()=0;
	/* int blockread(sector,N,buffer)
	   purpose:		reads N sectors starting at sector "sector" into buffer "buffer"
	   returns:		number of sectors read */
	virtual int				blockread(uint64 sector,int N,unsigned char *buffer)=0;
	/* uint64 disksize()
	   purpose:		returns the size of the "disk" in sectors */
	virtual uint64			disksize()=0;
public:	// overrideable methods for getting "comments" from the reader
	virtual void			comment(int level,char *str,...)=0;
public:	// methods for mounting, unmounting
	/* int mount()
	   purpose:		to "mount" the filesystem
	   returns:		1 if successful, 0 if not (e.g. filesystem error or already mounted) */
	int						mount();
	/* int umount()
	   purpose:		to "unmount" the filesystem
	   returns:		1 if successful, 0 if not (internal error, already unmounted) */
	int						umount();
public:
	// to map sectors to ext2 blocks
	int						e2blockread(uint64 sector,int N,unsigned char *buffer);
	int						e2blockread1k(uint64 sector,int N,unsigned char *buffer);
	unsigned char*			read_group_descriptor_sector(uint64 sector);
public:
	int						mounted;
	uint64					esuperblockN;
	uint64					esuperblockN1k;
	int						sectsize;
	int						e2blocksize;
	unsigned char*			tmpe2block;
public:
	int						GetSuperBlock(unsigned char *tmp,int sz,int sb);
public:
/*
 * Ext2 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
	enum {
		EXT2_FT_UNKNOWN,
		EXT2_FT_REG_FILE,
		EXT2_FT_DIR,
		EXT2_FT_CHRDEV,
		EXT2_FT_BLKDEV,
		EXT2_FT_FIFO,
		EXT2_FT_SOCK,
		EXT2_FT_SYMLINK,
		EXT2_FT_MAX
	};
public:
// MSVC hack: MSVC will by default pad things to DWORD alignment.
// to use direct casting of stuff to memory we must override that here.
//
// GCC in my experience will not do this unless asked to. I recommend however
// that you tell it explicitly not do in the CFLAGS env. variable
#ifdef WIN32
#pragma pack(push,1)
#endif
	typedef struct dir_entry_2 {
		uint32				inode;					/* Inode number */
		unsigned short		rec_len;				/* Directory entry length */
		unsigned char		name_len;				/* Name length */
		unsigned char		file_type;
		char				name[EXT2_NAME_LEN];	/* File name */
// not really part of struct but present to ensure that there is a trailing \0
		char				null_end;
	};
	typedef struct dir_entry {
		uint32				inode;					/* Inode number */
		unsigned short		rec_len;				/* Directory entry length */
		unsigned short		name_len;				/* Name length */
		char				name[EXT2_NAME_LEN];	/* File name */
// not really part of struct but present to ensure that there is a trailing \0
		char				null_end;
	};
	typedef struct group_desc {
        uint32				bg_block_bitmap;		/* Blocks bitmap block */
        uint32				bg_inode_bitmap;		/* Inodes bitmap block */
        uint32				bg_inode_table;			/* Inodes table block */
        unsigned short		bg_free_blocks_count;	/* Free blocks count */
        unsigned short		bg_free_inodes_count;	/* Free inodes count */
        unsigned short		bg_used_dirs_count;		/* Directories count */
        unsigned short		bg_pad;
        uint32				bg_reserved[3];
	};
	typedef struct inode {
        unsigned short		i_mode;					/* File mode */
        unsigned short		i_uid;					/* Low 16 bits of Owner Uid */
        uint32				i_size;					/* Size in bytes */
        uint32				i_atime;				/* Access time */
        uint32				i_ctime;				/* Creation time */
        uint32				i_mtime;				/* Modification time */
        uint32				i_dtime;				/* Deletion Time */
        unsigned short		i_gid;					/* Low 16 bits of Group Id */
        unsigned short		i_links_count;			/* Links count */
        uint32				i_blocks;				/* Blocks count */
        uint32				i_flags;				/* File flags */
        union {
				struct {
						uint32	l_i_reserved1;
                } linux1;
                struct {
                        uint32	h_i_translator;
                } hurd1;
                struct {
                        uint32	m_i_reserved1;
                } masix1;
        } osd1;										/* OS dependent 1 */
        uint32				i_block[EXT2_N_BLOCKS];	/* Pointers to blocks */
        uint32				i_generation;			/* File version (for NFS) */
        uint32				i_file_acl;				/* File ACL */
        uint32				i_dir_acl;				/* Directory ACL, or for regular files "i_size_high" (upper 32 bits of file size) */
        uint32				i_faddr;				/* Fragment address */
        union {
                struct {
                        unsigned char		l_i_frag;       /* Fragment number */
                        unsigned char		l_i_fsize;      /* Fragment size */
                        unsigned short		i_pad1;
                        unsigned short		l_i_uid_high;   /* these 2 fields    */
                        unsigned short		l_i_gid_high;   /* were reserved2[0] */
                        uint32				l_i_reserved2;
                } linux2;
                struct {
                        unsigned char		h_i_frag;       /* Fragment number */
                        unsigned char		h_i_fsize;      /* Fragment size */
                        unsigned short		h_i_mode_high;
                        unsigned short		h_i_uid_high;
                        unsigned short		h_i_gid_high;
                        uint32				h_i_author;
                } hurd2;
                struct {
                        unsigned char		m_i_frag;       /* Fragment number */
                        unsigned char		m_i_fsize;      /* Fragment size */
                        unsigned short		m_pad1;
                        uint32				m_i_reserved2[2];
                } masix2;
        } osd2;                         /* OS dependent 2 */
	};
	typedef struct super_block {
        uint32					s_inodes_count;         /* Inodes count */
        uint32					s_blocks_count;         /* Blocks count */
        uint32					s_r_blocks_count;       /* Reserved blocks count */
        uint32					s_free_blocks_count;    /* Free blocks count */
        uint32					s_free_inodes_count;    /* Free inodes count */
        uint32					s_first_data_block;     /* First Data Block */
        uint32					s_log_block_size;       /* Block size */
        uint32					s_log_frag_size;        /* Fragment size */
        uint32					s_blocks_per_group;     /* # Blocks per group */
        uint32					s_frags_per_group;      /* # Fragments per group */
        uint32					s_inodes_per_group;     /* # Inodes per group */
        uint32					s_mtime;                /* Mount time */
        uint32					s_wtime;                /* Write time */
        unsigned short			s_mnt_count;            /* Mount count */
        unsigned short			s_max_mnt_count;        /* Maximal mount count */
        unsigned short			s_magic;                /* Magic signature */
        unsigned short			s_state;                /* File system state */
        unsigned short			s_errors;               /* Behaviour when detecting errors */
        unsigned short			s_minor_rev_level;      /* minor revision level */
        uint32					s_lastcheck;            /* time of last check */
        uint32					s_checkinterval;        /* max. time between checks */
        uint32					s_creator_os;           /* OS */
        uint32					s_rev_level;            /* Revision level */
        unsigned short			s_def_resuid;           /* Default uid for reserved blocks */
        unsigned short			s_def_resgid;           /* Default gid for reserved blocks */
        /*
         * These fields are for EXT2_DYNAMIC_REV superblocks only.
         *
         * Note: the difference between the compatible feature set and
         * the incompatible feature set is that if there is a bit set
         * in the incompatible feature set that the kernel doesn't
         * know about, it should refuse to mount the filesystem.
         *
         * e2fsck's requirements are more strict; if it doesn't know
         * about a feature in either the compatible or incompatible
         * feature set, it must abort and not try to meddle with
         * things it doesn't understand...
         */
        uint32					s_first_ino;            /* First non-reserved inode */
        unsigned short			s_inode_size;          /* size of inode structure */
        unsigned short			s_block_group_nr;       /* block group # of this superblock */
        uint32					s_feature_compat;       /* compatible feature set */
        uint32					s_feature_incompat;     /* incompatible feature set */
        uint32					s_feature_ro_compat;    /* readonly-compatible feature set */
        unsigned char			s_uuid[16];             /* 128-bit uuid for volume */
        char					s_volume_name[16];      /* volume name */
        char					s_last_mounted[64];     /* directory where last mounted */
        uint32					s_algorithm_usage_bitmap; /* For compression */
        /*
         * Performance hints.  Directory preallocation should only
         * happen if the EXT2_COMPAT_PREALLOC flag is on.
         */
        unsigned char			s_prealloc_blocks;      /* Nr of blocks to try to preallocate*/
        unsigned char			s_prealloc_dir_blocks;  /* Nr to preallocate for dirs */
        unsigned short			s_padding1;
        /*
         * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
		 */
        unsigned char			s_journal_uuid[16];     /* uuid of journal superblock */
        uint32					s_journal_inum;         /* inode number of journal file */
        uint32					s_journal_dev;          /* device number of journal file */
        uint32					s_last_orphan;          /* start of list of inodes to delete */
        uint32					s_hash_seed[4];         /* HTREE hash seed */
        unsigned char			s_def_hash_version;     /* Default hash version to use */
        unsigned char			s_reserved_char_pad;
        unsigned short			s_reserved_word_pad;
        uint32					s_default_mount_opts;
        uint32					s_first_meta_bg;        /* First metablock block group */
        uint32					s_reserved[190];        /* Padding to the end of the block */
	};
#ifdef WIN32
#pragma pack(pop)
#endif
public:
	class file {
	public:
		file(reader *r);
		~file();
		int						get_inode(uint64 in);
		int						reset();
		int						read(unsigned char *buf,int len);
		uint64					seek(uint64 ofs);
		int						nextblock();
	public:
		reader					*mom;
		inode					the_inode;
		uint64					the_inode_N;
		char					the_inode_yes;
		int						block_n;
		int						pblock_n;
		uint64					cur_block;
		uint64					buffer_block;
		unsigned char*			buffer;
		unsigned char*			bufferfence;
		unsigned char*			ptr;
		uint64					file_pointer;
		uint64					file_pointer_max;
		// used for keeping track when handling {double-,triple-}indirect references
		unsigned char*			indirect_map;
		unsigned char*			indirect_map_ptr;
		unsigned char*			indirect_map_fence;
		unsigned char*			dindirect_map;
		unsigned char*			dindirect_map_ptr;
		unsigned char*			dindirect_map_fence;
		unsigned char*			tindirect_map;
		unsigned char*			tindirect_map_ptr;
		unsigned char*			tindirect_map_fence;
	};
	class readdir {
	public:
		readdir(reader *r);
		~readdir();
		int						get_inode(uint64 in);
		int						reset();
		int						read();
	public:
		reader					*mom;
		inode					the_inode;
		uint64					the_inode_N;
		char					the_inode_yes;
		int						block_n;
		int						pblock_n;
		uint64					cur_block;
		unsigned char			ent_raw[4096];
		int						ent_raw_n;
		int						ent_raw_max;
		unsigned char*			buffer;
		unsigned char*			bufferfence;
		unsigned char*			ptr;
		dir_entry_2				cur_dirent;
		// used for keeping track when handling {double-,triple-}indirect references
		unsigned char*			indirect_map;
		unsigned char*			indirect_map_ptr;
		unsigned char*			indirect_map_fence;
		unsigned char*			dindirect_map;
		unsigned char*			dindirect_map_ptr;
		unsigned char*			dindirect_map_fence;
		unsigned char*			tindirect_map;
		unsigned char*			tindirect_map_ptr;
		unsigned char*			tindirect_map_fence;
	};
public:
	super_block					theSuperBlock;
	int							e2fsfragsize;
	int							e2fsblocksize;
	int							e2fs_inode_size;
	int							e2fs_first_inode;
	int							e2fs_frags_per_block;
	int							e2fs_inodes_per_block;
	int							e2fs_itb_per_group;
	int							e2fs_desc_per_block;
	int							e2fs_addr_per_block_bits;
	int							e2fs_desc_per_block_bits;
	int							e2fs_groups_count;
	int							e2fs_db_count;
	int							e2fs_first_data_block;
	readdir*					root_dir;
	readdir*					GetRoot();
	uint64						GetFileSize(inode *i);
public:
	int							ext2_bg_has_super(int group);
	int							GetInode(uint64 N,inode *i);
	int							GetGroupDesc(uint64 N,group_desc *g);
};

////////////////////////////////////////////////
};
// END CEXT2 namespace

#endif //CEXT2_H
