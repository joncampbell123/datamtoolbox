
#ifndef CNTFS_H
#define CNTFS_H

#include "common/Types.h"
#include "anysize.h"

// BEGIN CNTFS namespace
namespace CNTFS {
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
public:
public:
	class MFT {
	public:
		typedef struct {
			char		magic[4];		// "FILE"
			uint16		usa_ofs;		// offset to the Update Sequence Array
			uint16		usa_count;		// number of USA entries
			uint64		lsn;			// $LogFile sequence number for this record
			uint16		seq_number;		// Number of times this record has been reused
			uint16		link_count;		// number of hard links
			uint16		attr_offset;	// offset of first attribute record
			uint16		flags;			// MFT record flags
			uint32		bytes_used;		// Number of bytes used in this MFT
			uint32		bytes_alloc;	// Number of bytes allocated for this MFT
			uint64		base_mft;		// Base MFT record
			uint16		next_attr_inst;	// Next attribute instance
			uint16		reserved;
			uint32		mft_record_number;	// (WinXP) Number of this MFT record
		} MFT_RECORD;
		typedef struct {
			uint32			type;			// attribute type
			uint32			length;			// attribute length
			unsigned char	non_resident;	// 0=resident attribute,  1=nonresident attribute
			unsigned char	name_length;	// unicode character size of name of attribute
			uint16			name_offset;	// offset to name
			uint16			flags;			// attribute flags
			uint16			instance;		// attribute instance
			struct {
				/* resident attributes */
				struct {
					uint32			value_length;
					uint16			value_offset;
					unsigned char	flags;
					unsigned char	reserved;
				} resident;
				/* nonresident attributes */
				struct {
					uint64			lowest_vcn;
					uint64			highest_vcn;
					uint16			mapping_pairs_offset;
					unsigned char	compression_unit;
					unsigned char	reserved[5];
					// used only when lowest_vcn == 0
					uint64			allocated_size;
					uint64			data_size;
					uint64			initialized_size;
					// used only if compressed
					uint64			compressed_size;
				} nonresident;
			} data;
			// set by AttrNext
			unsigned char*			attr_ptr;
			unsigned char*			attr_rec_ptr;
			unsigned char*			attr_resident_value_ptr;
			unsigned char*			attr_resident_value_ptr_fence;
		} ATTR_RECORD;
		// Index root resident attribute header (0x90)
		typedef struct {
			uint32					type;
			uint32					collation_rule;
			uint32					index_block_size;
			unsigned char			clusters_per_index_block;
			unsigned char			reserved[3];
			struct {
				uint32				entries_offset;
				uint32				index_length;
				uint32				allocated_size;
				unsigned char		flags;			// index header flags
				unsigned char		reserved[3];
			} index;
		} ATTR_IndexRoot_Header;
		typedef struct {
			// INDEX_ROOT_HEADER
			struct {
				struct {
					sint64			indexed_file;
				} dir;
				struct {
					unsigned short	data_offset;
					unsigned short	data_length;
					uint32			reservedV;
				} vi;
			} data;
			unsigned short			length;
			unsigned short			key_length;
			unsigned short			flags;			// index entry flags
			unsigned short			reserved;

			unsigned char*			p_data;
			unsigned char*			p_data_fence;
			unsigned char*			key;
			unsigned char*			key_fence;
		} ATTR_IndexRoot_Entry;
		// Standard Information attribute
		typedef struct {
			uint64					creation_time;
			uint64					last_data_change_time;
			uint64					last_mft_change_time;
			uint64					last_access_time;
			uint32					file_attributes;
			// only if NTFS 3.x
			uint32					maximum_versions;
			uint32					version_number;
			uint32					class_id;
			uint32					owner_id;
			uint32					security_id;
			uint64					quota_charged;
			uint64					usn;
		} ATTR_StandardInformation;
		// MFT flags
		enum {
			MFT_RECORD_IN_USE =					0x0001,
			MFT_RECORD_IS_DIRECTORY =			0x0002
		};
		// attribute types
		enum {
			AT_UNUSED =							0,
			AT_STANDARD_INFORMATION =			0x10,
			AT_ATTRIBUTE_LIST =					0x20,
			AT_FILE_NAME =						0x30,
			AT_OBJECT_ID =						0x40,
			AT_SECURITY_DESCRIPTOR =			0x50,
			AT_VOLUME_NAME =					0x60,
			AT_VOLUME_INFORMATION =				0x70,
			AT_DATA =							0x80,
			AT_INDEX_ROOT =						0x90,
			AT_INDEX_ALLOCATION =				0xa0,
			AT_BITMAP =							0xb0,
			AT_REPARSE_POINT =					0xc0,
			AT_EA_INFORMATION =					0xd0,
			AT_EA =								0xe0,
			AT_PROPERTY_SET =					0xf0,
			AT_LOGGED_UTILITY_STREAM =			0x100,
			AT_FIRST_USER_DEFINED_ATTRIBUTE =	0x1000,
			AT_END =							0xffffffff
		};
		// special files from the MFT
		enum {
			FILE_MFT =							0,		// $MFT
			FILE_MFTMirror =					1,		// $MFTMirr
			FILE_LogFile =						2,		// $LogFile
			FILE_Volume =						3,		// $Volume
			FILE_AttributeDefinitions =			4,		// $AttrDef
			FILE_RootDirectory =				5,		// .
			FILE_AllocationBitmap =				6,		// $Bitmap (allocation bitmap of clusters)
			FILE_BootSector =					7,		// $Boot (boot sector)
			FILE_BadClusters =					8,		// $BadClus (lists all bad sectors as non-resident data attributes)
			FILE_Secure =						9,		// $Secure (shared security descriptors)
			FILE_UpperCaseUnicodeTable =		10,		// $UpCase (uppercase versions of all unicode characters)
			FILE_ExtendedMFT =					11,		// $Extend (directory containing other system files like $ObjId, etc.)
			// 12 thru 15 are not used but marked as used
		};
		// attribute flags
		enum {
			ATTR_IS_COMPRESSED =				0x0001,
			ATTR_COMPRESS_MASK =				0x00FF,
			ATTR_IS_ENCRYPTED =					0x4000,
			ATTR_IS_SPARSE =					0x8000
		};
		// file attribute flags
		enum {
			FILE_ATTR_READONLY =					0x00000001,
			FILE_ATTR_HIDDEN =						0x00000002,
			FILE_ATTR_SYSTEM =						0x00000004,
			FILE_ATTR_DIRECTORY =					0x00000010,
			FILE_ATTR_ARCHIVE =						0x00000020,
			FILE_ATTR_DEVICE =						0x00000040,
			FILE_ATTR_NORMAL =						0x00000080,
			FILE_ATTR_TEMPORARY =					0x00000100,
			FILE_ATTR_SPARSE_FILE =					0x00000200,
			FILE_ATTR_REPARSE_POINT =				0x00000400,
			FILE_ATTR_COMPRESSED =					0x00000800,
			FILE_ATTR_OFFLINE =						0x00001000,
			FILE_ATTR_NOT_CONTENT_INDEXED =			0x00002000,
			FILE_ATTR_ENCRYPTED =					0x00004000,
			FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT =	0x10000000,
		};
		// file namespaces
		enum {
			FNS_POSIX=0,
			FNS_WIN32=1,
			FNS_DOS=2,
			FNS_WIN32_AND_DOS=3
		};
		// index root entry flags
		enum {
			INDEX_ENTRY_NODE=1,
			INDEX_ENTRY_END=2
		};
		// NTFS VCN run lists
		class NTFSVCNRunlist {
		public:
			NTFSVCNRunlist(reader *x);
			~NTFSVCNRunlist();
			static int __cdecl __sort(const void *elem1,const void *elem2);
		public:
			typedef struct ent {
				sint64		vcn;
				sint64		lcn;
				sint64		len;
			};
		public:
			void			Sort();
			sint64			Map(uint64 N,ent *en=((ent*)0));
		public:
			enum {
				LCN_HOLE =				-1,
				LCN_NOT_MAPPED =		-2,
				LCN_ENOENT =			-3,
			};
		public:
			int			Parse(ATTR_RECORD *r);
			ent*		Alloc();
		public:
			ent*		list;
			int			list_max;
			int			list_alloc;
		public:
			reader*		mom;
		};
	public:
		class NTFSIndexCrap;
		class RecordReader {
		public:
			class AttrFilename {
			public:
				AttrFilename();
				~AttrFilename();
			public:
				int					ParseName(ATTR_RECORD *r);
				int					ParseName(unsigned char *raw,int len);
				int					GetName(RecordReader *rr,int namspace);
				int					Unicode2Ansi();
			public:
				unsigned short*		name_u;
				char*				name_a;
			public:
				uint64				parent_directory;
				uint64				creation_time;
				uint64				last_data_change_time;
				uint64				last_mft_change_time;
				uint64				last_access_time;
				uint64				allocated_size;
				uint64				data_size;
				uint32				file_attributes;
				uint16				packed_ea_size;
				uint32				reparse_point_tag;
				unsigned char		file_name_length;
				unsigned char		file_name_type;
			};
			class DirectorySearch {
			public:
				DirectorySearch(RecordReader *x);
				~DirectorySearch();
			public:
				int					FindFirst();
				int					FindNext();
				int					Gather();
			public:
				AttrFilename*		found_name;
				NTFSIndexCrap*		nic;
				AnySize<sint64>		MFTref;
				int					MFTenum;
				sint64				MFT_current;
			public:
				RecordReader*		mom;
			};
		public:
			RecordReader(reader *x);
			~RecordReader();
			int				Parse(uint64 cluster);
			int				ReadUSA(unsigned int n);
		public:
			int				ParseAttr(unsigned char *buf,ATTR_RECORD *a);
			int				ParseStdInfo(ATTR_RECORD *a,ATTR_StandardInformation *asi);
		public:
			reader*			mom;
			MFT_RECORD		header;
			unsigned char*	copy;		// holds a copy of the MFT record, or first if more than one
			int				bytespersec;
			int				clustsize;
		public:	// attribute enumeration
			unsigned char*	attr_enum;
			unsigned char*	attr_fence;
			ATTR_RECORD		attr_cur;
			int				AttrFirst();
			int				AttrNext();
		};
		// NTFS object data access class
		class NTFSFileData {
		public:
			NTFSFileData();
			NTFSFileData(RecordReader *x);
			~NTFSFileData();
		public:
			virtual int		SetupData();
		public:
			char			got_data;
			unsigned char*	resident_data;
			int				resident_data_len;
			uint64			file_pointer;
			uint64			file_pointer_max;
			RecordReader*	mom;
			NTFSVCNRunlist*	nonresident_map;
			unsigned char*	nonresident_cache;
			int				cluster_size;
			sint64			nonresident_cache_cluster;
			uint32			attributes;
			unsigned char	compression_unit;
			AnySize<NTFSVCNRunlist::ent> last_ent;
		public:
			virtual uint64	seek(uint64 ofs);
			virtual int		read(unsigned char *buf,int len);
			virtual char	NeedInterpreter();
		};
		// NTFS object data access class for compressed files
		class NTFSFileDataCompressed : public NTFSFileData {
		public:
			NTFSFileDataCompressed(NTFSFileData *source);
			~NTFSFileDataCompressed();
		public:
			virtual int		SetupData();
		public:
			NTFSFileData*	source;
			unsigned char	compression_unit;
			int				compression_unit_size;
			unsigned char*	compression_unit_in;
			unsigned char*	compression_unit_out;
			sint64			last_cblock;
		public:
			virtual uint64	seek(uint64 ofs);
			virtual int		read(unsigned char *buf,int len);
			virtual char	NeedInterpreter();
		};
		// NTFS Index root + allocation + bitmap attribute handling code
		class NTFSIndexCrap {
		public:
			NTFSIndexCrap(RecordReader *x);
			~NTFSIndexCrap();
		public:
			int								SetEnumerationType(uint32 typ);
			int								FindFirst();
			int								FindNext();
		public:
			RecordReader*					mom;
			uint32							enum_type;
			int								enum_state;
			struct {
				ATTR_IndexRoot_Header		header;
				unsigned char*				ent;
				unsigned char*				fence;
			} enum_attr90_state;
			struct {
				NTFSVCNRunlist::ent			curcl_ent;
				NTFSVCNRunlist*				runlist;
				uint64						current,max;
				unsigned char*				buffer;
				uint64						curcl;
				// large buffer for parsing index allocation stuff
				unsigned char*				INDX_buffer;		// +---- overall buffer
				unsigned char*				INDX_fence;			// |
				unsigned char*				INDX_chklast;

				unsigned char*				INDX_ptr;			// +---- pointer for loaded data
				unsigned char*				INDX_last;			// |

				int							INDX_chunk_sz;
				int							INDX_subchunk_N;
				int							INDX_subchunk_max;

				// from index
				unsigned short				usa_ofs;
				unsigned short				usa_count;
				sint64						lsn;
				uint64						index_block_vcn;
				struct {
					uint32					entries_offset;
					uint32					index_length;
					uint32					allocated_size;
					unsigned char			flags;
					uint32					index_counted;
				} index_header;
			} enum_attrA0_state;
			ATTR_IndexRoot_Header*			enum_result_header;
			ATTR_IndexRoot_Entry			enum_result;
		public:
			enum {
				STATE_ATTR_FIRST=0,
				STATE_ATTR_SCAN,
				STATE_ATTR_90_ENUM,
				STATE_ATTR_A0_ENUM,
				STATE_FINISHED
			};
		};
	};
	class MasterMFT : public MFT::RecordReader {
	public:
		MasterMFT(reader *x);
		~MasterMFT();
	public:
		int						CheckMFT();
		sint64					Map(uint64 N,MFT::NTFSVCNRunlist::ent *en=((MFT::NTFSVCNRunlist::ent*)0));
		int						ParseMapped(uint64 vcn);
	public:
		MFT::NTFSVCNRunlist*	data_runlist;
		reader*					mom;
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
	unsigned char*			tmpsect;
	unsigned char*			tmpcluster;
	unsigned char*			tmpMFT;
public:
	int						GetBPB(unsigned char *tmp,int sz);
	unsigned char*			ReadCluster(uint64 n);
	unsigned char*			ReadMFT(uint64 n);
	unsigned char*			ReadMFTUnmapped(uint64 n);
	MFT::RecordReader*		ReadMFTNomapping(uint64 n);
	void					MFTStripMSTBs(unsigned char *buf,int sectors);
	MFT::RecordReader*		GetRootDir();
public:
	/* vars generated internally */
	unsigned int			BPBSize;
	/* BIOS Parameter Block */
	unsigned int			BytesPerSector;
	unsigned char			SectorsPerCluster;
	unsigned int			ReservedSectors;
	unsigned char			FATCopies;			// # of FAT tables
	unsigned int			RootDirEntries;
	uint64					TotalSectors;		// depends on which field this is taken from
	unsigned char			MediaDescriptor;
	unsigned long			SectorsPerFAT;
	unsigned long			SectorsPerTrack;
	unsigned int			Heads;
	unsigned long			HiddenSectors;
	uint64					MFT_cluster;
	uint64					MFT_cluster_mirror;
	uint32					ClustersPerFileRecSeg;
	uint32					ClustersPerIndexBlock;
	uint64					VolumeSerial;
	uint32					Checksum;
public:
	MasterMFT*				The_MFT;
	MFT::RecordReader*		RootDir_MFT;
public:
	int						CheckMFT();
};

////////////////////////////////////////////////
};
// END CNTFS namespace

#endif //CNTFS_H
