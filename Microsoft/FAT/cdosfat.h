
#ifndef CDOSFAT_H
#define CDOSFAT_H

#include "common/Types.h"

// BEGIN CDOSFat namespace
namespace CDOSFat {
////////////////////////////////////////////////

// TODO: these macros assume the host architecture is Little Endian!
#define CDF_GETWORD(x)		(llei16(x))
#define CDF_GETDWORD(x)		(llei32(x))

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
	enum FileTypes {		// for use as the "type" parameter in file()
		FT_TYPE_FILE=0,
		FT_TYPE_DIR=1,
	};
public:
	class entity {
	public:
		/* attribute masks for the directory entry */
		enum {
			ATTR_READONLY=1,
			ATTR_HIDDEN=2,
			ATTR_SYSTEM=4,
			ATTR_VOLUMELABEL=8,
			ATTR_DIRECTORY=0x10,
			ATTR_ARCHIVE=0x20,
			ATTR_FILE=7,
		};
	public:
		typedef struct {
			unsigned char	name[12];		// 8-char name or 11-char volume label
			unsigned char	ext[4];			// 3-char extension
			unsigned char	attr;			// 1-char attributes
			unsigned short	time;			// DOS time
			unsigned short	date;			// DOS date
			unsigned long	size;			// file size
			unsigned long	cluster;		// starting cluster
			/* not strictly necesary, but informative fields related to Windows */
			unsigned char	creation_time_cs;	// creation time stamp, 10ths of a second portion
			unsigned short	creation_time;		// creation time stamp
			unsigned short	creation_date;		// creation time stamp
			unsigned short	last_access_date;	// last-accessed time stamp
			unsigned short	OS2_EA_index;		// OS/2 EA database index
			/* made up by this reader class */
			unsigned char	fullname[13];
			unsigned char	rawname[11];
			unsigned long	offset;			// offset in "file" of directory entry
		} FATDIRENTRY;
	public:
		class LFN {
		public:
			LFN();
			~LFN();
			int unicode2ansi();
		public:
			unsigned char*		name_ansi;
			unsigned short*		name_unicode;
			int			length;
		};
	public:
		entity(reader *parent);
		~entity();
	public:
		unsigned long		seek(unsigned long pos);
		unsigned long		tell();
		int					read(unsigned char *buf,int sz);
		void				reset();
		int					parse(unsigned char buf[32],entity::FATDIRENTRY *f,unsigned long ofs);
		entity*				getdirent(entity::FATDIRENTRY *f);
		entity::LFN*		long_filename(entity::FATDIRENTRY *f);
	public:
		int					dir_findfirst(entity::FATDIRENTRY *f);
		int					dir_findnext(entity::FATDIRENTRY *f);
	public:
		reader*				mom;
		unsigned long		cluster_start;		// starting cluster
		unsigned long		file_size;			// file size
		unsigned long		pointer;			// current position (bytes)
		unsigned long		pointer_cr;			// file offset of current cluster
		unsigned long		cluster;			// current cluster
		unsigned long		linear_offset;		// if not part of data area, starting sector
		unsigned long		cluster_size;
		unsigned char		my_attr;
		unsigned long		findoffset;			// offset used by findfirst/findnext
		FATDIRENTRY			info;
		int					info_valid;
	};
public:	// initializer and deinitializer functions
	int init();
	int free();
public:	// overrideable methods for reading the "disk"
	/* int blocksize()
	   returns:		the size in bytes of one block (usually 512) */
	virtual int				blocksize()=0;
	/* int blockread(sector,N,buffer)
	   purpose:		reads N sectors starting at sector "sector" into buffer "buffer"
	   returns:		number of sectors read */
	virtual int				blockread(unsigned int sector,int N,unsigned char *buffer)=0;
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
public:	// methods for obtaining entity objects for...
	entity*					rootdirectory();	// the root directory
	entity*					file(char *path,entity* root,int LFN,int instance,int type);	// a file, referenced DOS path style (e.g. "\WINDOWS\WIN.INI")
public:
	int						mounted;
	int						sectsize;
	unsigned char*			tmpsect;
public:
	int						GetBPB(unsigned char *tmp,int sz);
	unsigned long			ReadFAT(unsigned long N);
public:
	/* vars generated internally */
	unsigned char			FAT;				// set to 12, 16, or 32 depending on FS
	unsigned long			FATMASK;
	unsigned int			TotalClusters;
	unsigned int			RootDirSectors;
	unsigned int			BPBSize;
	unsigned long			DataAreaOffset;
	/* from the boot sector */
	unsigned char			OEMSignature[9];	// 8-char OEM signature
	/* BIOS Parameter Block */
	unsigned int			BytesPerSector;
	unsigned char			SectorsPerCluster;
	unsigned int			ReservedSectors;
	unsigned char			FATCopies;			// # of FAT tables
	unsigned int			RootDirEntries;
	unsigned long			TotalSectors;		// depends on which field this is taken from
	unsigned char			MediaDescriptor;
	unsigned long			SectorsPerFAT;
	unsigned long			SectorsPerTrack;
	unsigned int			Heads;
	unsigned long			HiddenSectors;
	unsigned char			INT13DriveNo;
	unsigned char			WinNTCheckDiskFlags;// AKA "reserved"
	unsigned char			ExtendedSignature;	// 0x29, signals presence of fields:
	unsigned long			SerialNumber;		// ...the serial #
	unsigned char			VolumeLabel[12];	// ...11-char volume label
	unsigned char			FileSystemType[9];	// ...8-char volume label
	/* FAT32 specific, not updated if FAT != 32 */
	unsigned int			MirrorFlags;		// the field, unparsed. it is also split into:
	unsigned char			ActiveFAT32;		// ....number of active FAT
	unsigned char			SingleActiveFAT32;	// ....whether only one FAT is active
	unsigned int			FS32Version;		// filesystem version
	unsigned long			RootDirCluster;		// starting cluster of root directory
	unsigned int			FSInfoSector32;		// filesystem info sector number
	unsigned int			BackupBootSector32;	// backup boot sector location (0 or 0xFFFF if none)
};

////////////////////////////////////////////////
};
// END CDOSFat namespace

#endif //CDOSFAT_H
