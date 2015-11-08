
#ifndef CMCOFS_H
#define CMCOFS_H

#include "common/Types.h"

// BEGIN CMCOFS namespace
namespace CMCOFS {
////////////////////////////////////////////////

/* these are designed to retrieve WORDs and DWORDs in Big Endian format! */
#define CMCOFS_GETWORD(x)		(lbei16(x))
#define CMCOFS_GETDWORD(x)		(lbei32(x))

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
	enum FileTypes {		// for use as the "type" parameter in file()
		FT_TYPE_FILE=0,
		FT_TYPE_DIR=1,
	};
public:
	class entity {
	public:
		/* attribute masks for the directory entry */
		enum {
			ATTR_DIRTY=1,
			ATTR_SUBDIRECTORY=2,
			ATTR_SANTICIFIED=4,
			ATTR_NEEDS_POSTING=8,
		};
	public:
		typedef struct {
			unsigned char		name[17];		/* 16-char filename */
			unsigned short		sub_number;		/* sub-number (MCO 2.x only) */
			unsigned long		pointer;		/* pointer on disk to file */
			unsigned long		length;			/* length of file */
			unsigned long		creation_time;	/* creation time (MCO 2.x only) */
			unsigned long		last_access_time; /* last access time (MCO 2.x only) */
			unsigned short		attr;
			/* made up by this class */
			unsigned char		fs_version;		/* 1=MCO 1.x   2=MCO 2.x */
		} DIRENTRY;
	public:
		entity(reader *parent);
		~entity();
	public:
		unsigned long		seek(unsigned long pos);
		unsigned long		tell();
		int			read(unsigned char *buf,int sz);
		void			reset();
		int			parse(unsigned char buf[32],entity::DIRENTRY *f,unsigned long ofs);
		entity*			getdirent(entity::DIRENTRY *f);
	public:
		int			dir_findfirst(entity::DIRENTRY *f);
		int			dir_findnext(entity::DIRENTRY *f);
	public:
		reader*			mom;
		unsigned long		size;				// file size
		unsigned long		pointer;			// current position (bytes)
		unsigned long		offset;				// offset in bytes
		unsigned char		my_attr;
		unsigned long		findoffset;			// offset used by findfirst/findnext
		DIRENTRY		info;
		int			info_valid;
	};
public:	// initializer and deinitializer functions
	int init();
	int free();
public:	// overrideable methods for reading the "disk"
	/* int blocksize()
	   returns:		the size in bytes of one block (usually 512) */
	virtual int			blocksize()=0;
	/* int blockread(sector,N,buffer)
	   purpose:		reads N sectors starting at sector "sector" into buffer "buffer"
	   returns:		number of sectors read */
	virtual int			blockread(unsigned int sector,int N,unsigned char *buffer)=0;
	/* unsigned long disksize()
	   purpose:		returns the size of the "disk" in sectors */
	virtual unsigned long		disksize()=0;
public:	// overrideable methods for getting "comments" from the reader
	virtual void			comment(int level,char *str,...)=0;
public:	// methods for mounting, unmounting
	/* int mount()
	   purpose:		to "mount" the filesystem
	   returns:		1 if successful, 0 if not (e.g. filesystem error or already mounted) */
	int				mount();
	/* int umount()
	   purpose:		to "unmount" the filesystem
	   returns:		1 if successful, 0 if not (internal error, already unmounted) */
	int				umount();
public:	// methods for obtaining entity objects for...
	entity*				rootdirectory();	// the root directory
	entity*				file(char *path,entity* root,int instance,int type);	// a file, referenced DOS path style (e.g. "\WINDOWS\WIN.INI")
public:
	int				mounted;
	int				sectsize;
	unsigned char*			tmpsect;
public:
	int				GetHeader();
public:
	/* needed because MCO filesystem structures are not aligned to sectors */
	int				cread(unsigned long offset,int len,unsigned char *buf);
public:
	/* vars generated internally */
//				...
	/* from the boot sector */
	unsigned char			MCOfs_version;
	unsigned char			MCO_signature[4];
	unsigned long			MCO_boot_area;
	unsigned long			MCO2_lbt_offset;	/* MCO 2.x locked-block table offset+size */
	unsigned long			MCO2_lbt_size;
	unsigned long			MCO2_adt_offset;	/* MCO 2.x available disk table offset+size */
	unsigned long			MCO2_adt_size;
	unsigned long			MCO_dir_offset;		/* MCO directory offset+size */
	unsigned long			MCO_dir_size;
	unsigned long			MCO1_pfa_offset;	/* MCO 1.x free area offset+size */
	unsigned long			MCO1_pfa_size;
public:
};

////////////////////////////////////////////////
};
// END CMCOFS namespace

#endif //CMCOFS_H
