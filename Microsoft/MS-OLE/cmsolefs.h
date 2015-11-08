
#ifndef CMSOLEFS_H
#define CMSOLEFS_H

#include "common/Types.h"

// BEGIN CMSOLEFS namespace
namespace CMSOLEFS {
////////////////////////////////////////////////

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
		/* attribute flags */
		enum {
			ATTR_SUBDIRECTORY=1,
		};
		/* Property Storage type flags */
		enum {
			PSTYPE_STORAGE=1,
			PSTYPE_STREAM=2,
			PSTYPE_ROOT=5
		};
	public:
		typedef struct {
			unsigned short		ps_name_u[33];	// the Property Storage name (unicode)
			unsigned short		ps_name_len;	// length of unicode name in bytes including 0
			unsigned short		ps_type;	// type
			unsigned long		ps_previous;	// previous PS[*]
			unsigned long		ps_next;	// next PS[*]
			unsigned long		ps_directory;	// directory PS
			unsigned long		ps_start;	// starting block of property
			unsigned long		ps_size;
			// [*] "previous" and "next" taken from the LAOLA project documentation.
			//     I think however that they should be called "ref1" and "ref2"
			//     because even though every entry has two pointers to other entries
			//     the same numbers never appear twice in either one. If they were
			//     truely "next" and "previous" pointers the entry pointed to by
			//     "next" could be read and it's "previous" pointer would point
			//     back to where we started from.
			/* made up by this class */
			unsigned char		ps_name[33];	// the Property Storage name (ascii)
			unsigned char		attr;
		} DIRENTRY;
	public:
		entity(reader *parent);
		~entity();
	public:
		int			ScanPropertyStorage();
		int			ScanPropertyStorageRec(unsigned long blk,int recw);
	public:
		unsigned long		seek(unsigned long pos);
		unsigned long		tell();
		int			read(unsigned char *buf,int sz);
		void			reset();
		int			parse(unsigned char buf[0x80],entity::DIRENTRY *f,unsigned long ofs);
		entity*			getdirent(entity::DIRENTRY *f);
		entity*			getdirentdata(entity::DIRENTRY *f);
	public:
		int			dir_findfirst(entity::DIRENTRY *f);
		int			dir_findnext(entity::DIRENTRY *f);
	public:
		reader*			mom;
		unsigned long		adjust;
		unsigned long		size;				// file size (0 if not known)
		unsigned long		pointer;			// current position (bytes)
		unsigned long		starting_block;			// starting block
		unsigned long		current_block;			// current block
		unsigned long		current_block_offset;		// offset in file of current block
		unsigned char		my_attr;
		unsigned long		find_handle;			// current "handle" of PPS
		unsigned long		find_handle_start;		// starting "handle" of PPS
		unsigned char		use_small_blocks;		// 1=blocks are defined as small blocks
		unsigned int		block_size;
		DIRENTRY		info;
		int			info_valid;
		// for enumerating "storage"
		unsigned long*		pps_array;
		int			pps_array_size;
		int			pps_array_alloc;
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
	entity*				small_block_depot();	// the small block depot
	entity*				root_entry();		// the Root Entry
	entity*				file(char *path,entity* root,int instance,int type);	// a file, referenced DOS path style (e.g. "\WINDOWS\WIN.INI")
public:// block-level access
	int				ReadAbsBlock(unsigned long o,int N,unsigned char *buf);
	// Big Block Depot list access
	unsigned long			ReadBBD(unsigned long N);
	// Small Block Depot list access
	unsigned long			ReadSBD(unsigned long N);
public:
	int				mounted;
	int				sectsize;
	unsigned char*			tmpsect;
public:
	int				GetHeader();
public:
	/* vars generated internally */
	entity*				sbd_ent;	// used to read the Small Block Depot
	entity*				root_entry_ent;	// used to gather Small Blocks which are held in the "Root Entry"
	/* from the header */
	unsigned long			num_blocks;
	unsigned long			num_bbd_blocks;	// number of "Big Block Depot" blocks
	unsigned long			root_block;	// root chain first block
	unsigned long			sbd_1st;	// first block of "Small Block Depot"
public:
};

////////////////////////////////////////////////
};
// END CMSOLEFS namespace

#endif //CMSOLEFS_H
