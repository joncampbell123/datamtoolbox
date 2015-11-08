
#ifndef CPCPARTITIONS_H
#define CPCPARTITIONS_H

#define MAX_ENTRIES			64

// BEGIN CPcPartition namespace
namespace CPcPartition {
////////////////////////////////////////////////

class reader
{
public:
	typedef struct {
		unsigned char		flags;
		unsigned char		start_head;
		unsigned char		start_sector;
		unsigned int		start_cylinder;
		unsigned char		system_id;
		unsigned char		end_head;
		unsigned char		end_sector;
		unsigned int		end_cylinder;
		unsigned long		start;
		unsigned long		size;
		int			parent_ent;				// -1 if MBR
		unsigned long		partition_sector;
		unsigned int		partition_sector_offset;
	} ENTRY;
public:
	enum SystemIDs {
		SID_UNUSED =		0x00,
		SID_DOS_FAT12 =		0x01,
		SID_XENIX2 =		0x02,
		SID_XENIX3 =		0x03,
		SID_DOS_FAT16 =		0x04,
		SID_DOS_EXTENDED =	0x05,
		SID_DOS_BIG =		0x06,
		SID_NTFS =		0x07,
		SID_DOS_FAT32 =		0x0B,
		SID_DOS_FAT32_LBA =	0x0C,
		SID_LBA_EXTENDED =	0x0F,
		SID_UNIX =		0x63,
		SID_LINUX =		0x83,
	};
public:
	class partition {
	public:
		partition(reader *parent);
		~partition();
		int blocksize();
		int blockread(unsigned int sector,int N,unsigned char *buffer);
	public:
		reader *mom;
		unsigned long		size;
		unsigned long		offset;
	};
public:
	int init();
	int free();
public:
	/* int blocksize()
	   returns:		the size in bytes of one block (usually 512) */
	virtual int		blocksize()=0;
	/* int blockread(sector,N,buffer)
	   purpose:		reads N sectors starting at sector "sector" into buffer "buffer"
	   returns:		number of sectors read */
	virtual int		blockread(unsigned int sector,int N,unsigned char *buffer)=0;
	/* unsigned long disksize()
	   purpose:		returns the size of the "disk" in sectors */
	virtual unsigned long	disksize()=0;
public:	// overrideable methods for getting "comments" from the reader
	virtual void		comment(int level,char *str,...)=0;
public:	// methods for mounting, unmounting
	/* int mount()
	   purpose:		to "mount" the filesystem
	   returns:		1 if successful, 0 if not (e.g. filesystem error or already mounted) */
	int			mount();
	/* int umount()
	   purpose:		to "unmount" the filesystem
	   returns:		1 if successful, 0 if not (internal error, already unmounted) */
	int			umount();
public:	// methods for getting partitions
	ENTRY*			info(int index);
	partition*		get(int index);
public:
	int			mounted;
	int			sectsize;
	unsigned char*		tmpsect;
public:
	int			entries;
	ENTRY			entry[MAX_ENTRIES];
};

////////////////////////////////////////////////
};
// END CPcPartition namespace

#ifndef CPCPARTITIONS_H__INSIDER
#undef MAX_ENTRIES
#endif

#endif //CPCPARTITIONS_H
