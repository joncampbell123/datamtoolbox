
#ifndef CISO9660_H
#define CISO9660_H

#include "common/Types.h"
#include "anysize.h"

// BEGIN CISO9660 namespace
namespace CISO9660 {
////////////////////////////////////////////////

// use these macros with the structure members i.e
// CISO9660_32(super.s_magic) to avoid endianness problems
#define CISO9660_16(x)			(llei16((void*)(&x)))
#define CISO9660_32(x)			(llei32((void*)(&x)))
#define CISO9660_64(x)			(llei64((void*)(&x)))

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
	int						mounted;
	int						sectsize;
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
	typedef struct BiDir32 {
		uint32				le;			// little endian version
		uint32				be;			// big endian version
	};
	typedef struct BiDir16 {
		uint16				le;			// little endian version
		uint16				be;			// big endian version
	};
	typedef struct VolumeDescriptor {
		unsigned char		type;
		char				identifier[5];
		unsigned char		version;
		unsigned char		data[2041];
	};
	typedef struct Sec711_DateTime {
		union {
			struct {
				unsigned char	Year;	// since 1900
				unsigned char	Month;
				unsigned char	Day;	// day of the month
				unsigned char	Hour;
				unsigned char	Minute;
				unsigned char	Second;
				signed char		GMT_Offset;	// in 15-minute intervals
			} dt;
			unsigned char	raw[7];
		} x;
	};
	typedef struct Sec91_DirRecord {	// ECMA-119 section 9.1 directory record
		union {
			struct {
				unsigned char	Length_of_Directory_Record;				// @1
				unsigned char	Extended_Attr_Rec_Len;					// @2
				BiDir32			Loc_of_Extent;							// @3
				BiDir32			Data_Length;							// @11
				Sec711_DateTime	Rec_DateTime;							// @19
				unsigned char	File_Flags;								// @26
				unsigned char	File_Unit_Size;							// @27
				unsigned char	Interleave_Gap_Size;					// @28
				BiDir16			Vol_Seq_Number;							// @29
				unsigned char	Length_of_File_Ident;					// @33
				// this is 33 bytes, but not all of the data since more follows:
				// 
				// File Identifier @34 to @33 + Length_of_File_Ident inclusive
				//
				// Padding Field @34 + Length_of_File_Ident only if
				// Length_of_File_Ident is an even number
				//
				// System Use @34 + Length_of_File_Ident [ + padding] to
				// Length_of_Directory_Record - 1
			} dr;
			unsigned char	raw[34];
		} x;
	};
	enum {
		Sec91_DirRecord_File_Flags_NotExist=1,			// "Exists" to be made known to user
		Sec91_DirRecord_File_Flags_Directory=2,			// 1=is a directory
		Sec91_DirRecord_File_Flags_Associated_File=4,	// 1=is an "associated file"
		Sec91_DirRecord_File_Flags_Sec958_Record=8,		// 1=file information conforms to sec 9.5.8
		Sec91_DirRecord_File_Flags_Protection=16,		// 1=owner+group spec. for file and permissions are nonzero
		Sec91_DirRecord_File_Flags_MultiExtend=128,		// 1=not the final directory record for this file
	};
	typedef struct Sec84261_DateTime {	// ECMA-119 section 8.4.26.1 date/time
		union {
			typedef struct {
				unsigned char	year[4];
				unsigned char	month[2];
				unsigned char	day[2];		// day of month
				unsigned char	hour[2];
				unsigned char	minute[2];
				unsigned char	second[2];
				unsigned char	hundredths_of_second[2];
				unsigned char	GMT_Offset_15min_intervals;
			} dt;
			unsigned char	raw[17];
		} x;
	};
	typedef struct PrimaryVolumeData {	// primary Volume Descriptor data area
		// the spec seems to specify offsets that are one-based, not zero based
		// like what we C programmers are accustomed to........ also sizes of
		// data are given in inclusive ranges.
		unsigned char		Unused_8;									// @8
		unsigned char		System_Identifier	[	(  40 -    9)+1];	// @9
		unsigned char		Volume_Identifier	[	(  72 -   41)+1];	// @41
		unsigned char		Unused_73			[	(  80 -   73)+1];	// @73
		BiDir32				Volume_Space_Size;							// @81
		unsigned char		Unused_89			[	( 120 -   89)+1];	// @89
		BiDir16				Volume_Set_Size;							// @121
		BiDir16				Volume_Seq_Number;							// @125
		BiDir16				Logical_Block_Size;							// @129
		BiDir32				Path_Table_Size;							// @133
		BiDir16				Loc_of_Occur_Type_L_Path_Table;				// @141
		BiDir16				Loc_of_OptOccur_Type_L_Path_Table;			// @145
		BiDir16				Loc_of_Occur_Type_M_Path_Table;				// @149
		BiDir16				Loc_of_OptOccur_Type_M_Path_Table;			// @153
		Sec91_DirRecord		DirectoryRecord_for_RootDir;				// @157
		unsigned char		Volume_Set_Identifier[	( 318 -  191)+1];	// @191
		unsigned char		Publisher_Identifier[	( 446 -  319)+1];	// @319
		unsigned char		DataPrep_Identifier[	( 574 -  447)+1];	// @447
		unsigned char		Application_Identifier[	( 702 -  575)+1];	// @575
		unsigned char		CopyrightFile_Identifier[(739 -  703)+1];	// @703
		unsigned char		AbstractFile_Identifier[( 776 -  740)+1];	// @740
		unsigned char		Bibliographic_Identifier[(813 -  777)+1];	// @777
		Sec84261_DateTime	VolCreation;								// @814
		Sec84261_DateTime	VolModification;							// @831
		Sec84261_DateTime	VolExpiration;								// @848
		Sec84261_DateTime	VolEffective;								// @865
		unsigned char		FileStructureVersion;						// @882
		unsigned char		Reserved_883;								// @883
		unsigned char		Application_Use[		(1395 -  884)+1];	// @884
		unsigned char		Reserved_1396[			(2048 - 1396)+1];	// @1396
	};
	typedef struct SecondaryVolumeData {	// secondary Volume Descriptor data area
		unsigned char		Flags;										// @8
		unsigned char		System_Identifier	[	(  40 -    9)+1];	// @9
		unsigned char		Volume_Identifier	[	(  72 -   41)+1];	// @41
		unsigned char		Unused_73			[	(  80 -   73)+1];	// @73
		BiDir32				Volume_Space_Size;							// @81
		unsigned char		Escapes				[	( 120 -   89)+1];	// @89
		BiDir16				Volume_Set_Size;							// @121
		BiDir16				Volume_Seq_Number;							// @125
		BiDir16				Logical_Block_Size;							// @129
		BiDir32				Path_Table_Size;							// @133
		BiDir16				Loc_of_Occur_Type_L_Path_Table;				// @141
		BiDir16				Loc_of_OptOccur_Type_L_Path_Table;			// @145
		BiDir16				Loc_of_Occur_Type_M_Path_Table;				// @149
		BiDir16				Loc_of_OptOccur_Type_M_Path_Table;			// @153
		Sec91_DirRecord		DirectoryRecord_for_RootDir;				// @157
		unsigned char		Volume_Set_Identifier[	( 318 -  191)+1];	// @191
		unsigned char		Publisher_Identifier[	( 446 -  319)+1];	// @319
		unsigned char		DataPrep_Identifier[	( 574 -  447)+1];	// @447
		unsigned char		Application_Identifier[	( 702 -  575)+1];	// @575
		unsigned char		CopyrightFile_Identifier[(739 -  703)+1];	// @703
		unsigned char		AbstractFile_Identifier[( 776 -  740)+1];	// @740
		unsigned char		Bibliographic_Identifier[(813 -  777)+1];	// @777
		Sec84261_DateTime	VolCreation;								// @814
		Sec84261_DateTime	VolModification;							// @831
		Sec84261_DateTime	VolExpiration;								// @848
		Sec84261_DateTime	VolEffective;								// @865
		unsigned char		FileStructureVersion;						// @882
		unsigned char		Reserved_883;								// @883
		unsigned char		Application_Use[		(1395 -  884)+1];	// @884
		unsigned char		Reserved_1396[			(2048 - 1396)+1];	// @1396
	};
#ifdef WIN32
#pragma pack(pop)
#endif
public:
	class readdirents {
	public:
									readdirents(reader *x);
		int							add(Sec91_DirRecord* d);
		int							finish();
		uint64						seek(uint64 o);
		int							read(unsigned char *buf,int len);
		void						setblockalign(char c);
	public:
		reader*						mom;
		AnySize<Sec91_DirRecord>	list;
	public:
		char						lockdown;
		uint64						file_pointer;
		uint64						file_size;
		int							extent_i;
		uint64						extent_ofs;
		unsigned char				buffer[2048];
		sint64						buffer_N;
		char						blockalign_mode;
	};
	class readdir {
	public:
									readdir(readdirents *d);
		int							first();
		int							next();
		Sec91_DirRecord*			current;
		unsigned char				buffer[2048];
		unsigned char*				bufferi;
		unsigned char*				bufferfence;
		uint64						offset;
		unsigned char				fullname[256];
		char						fullname_is_ucs;
		int							fullnamel;
		unsigned char				name[256];
		unsigned char*				file_version;
	public:
		readdirents*				mom;
	};
	// a helper class that takes a Sec91_DirRecord and picks out any Rock Ridge extensions
	class RockRidge {
	public:
									RockRidge(reader* r);
									~RockRidge();
		int							Check(Sec91_DirRecord *r);
		char*						GetName();
		int							IsRelocatedDir();
		uint32						GetDirRelocation();
		int							IsSparse();
		int							FindByMatchingGraftOffset(readdirents *rr,readdirents *root,uint32 parent,uint32 ofs);
	public:
		Sec91_DirRecord*			record;
	public:
		typedef struct field {
			unsigned char*			start;
			unsigned char*			end;
			unsigned char			len;
			unsigned char			version;
			unsigned char			flags;
			unsigned char*			data;
		};
		AnySize<field>				list;
		reader*						mom;
		char*						NM_buffer;
	};
public:
	int						ReadVolDescriptor(int N,VolumeDescriptor *vd);
	int						TotalVolDescriptors;
	VolumeDescriptor		VD_Current;
	PrimaryVolumeData*		VD_Current_data;
	SecondaryVolumeData*	VD_Current_2ndary_data;
	char					VD_Current_valid;
	reader::readdirents*	GetRootDir();
	int						ChoosePrimary();
	int						ChooseSecondary(int N);
	int						CheckJoliet(int N=0);
	char					is_joliet;
};

////////////////////////////////////////////////
};
// END CISO9660 namespace

#endif //CISO9660_H
