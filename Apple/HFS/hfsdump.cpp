/* HFS Macintosh Hierarchical File System dumping engine.
   (C) 2004, 2005 Jonathan Campbell

   Examines Macintosh floppy images with HFS filesystem structures on them
   and extracts the files (both data and resource forks).

   10/26/2004: Added working support for the extent overflow system "file"
               in case files are fragmented.
   10/29/2004: Modified routine to scan trees and create folders FIRST then
               extract files. Some disks failed extraction because this
		       program would attempt to create the file before the folder
		       was created.
   10/30/2004: Fixed bug in IsBNodeAlloc that mishandled various structures.
		       B*-tree allocation map checking is temporarily disabled because
		       it doesn't seem to be working properly for nodes beyond the
			   header node.
*/

#ifdef LINUX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <direct.h>
#endif

static char export_macbinary=1;
static char export_macbinary_stream=0;		// set to one ONLY IF YOUR MacBinary decoder supports it!

// MacBinary CRC

static unsigned short crctab[] = { 
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
    };

static unsigned short updatecrc(unsigned char i, unsigned short crc)
{
	return ((crc<<8) ^ crctab[(crc>>8) ^ i]);

} /* end of updatecrc() */

// end

#ifdef LINUX

static int mkdir_hack(char *x)
{
	return mkdir(x,0644);
}

static void GetCurrentDirectory(int sz,char *buf)
{
	getcwd(buf,sz);
}

#define mkdir(x) mkdir_hack((char*)(x))
#endif

#include "Apple/common/Apple-Macintosh-Common.h"
#include "common/BlockDeviceFile.h"
#include "common/SomeFileStdio.h"
#include "Apple/Image-DiskCopy/BlockDeviceDiskCopyImage.h"

typedef struct {
	aInteger	xdrStABN;	// first allocation block
	aInteger	xdrNumABlks;	// number of allocation blocks
} aExtDescriptor;

typedef aExtDescriptor aExtDataRec[3];

typedef struct {
// structures common to MFS and HFS
	aInteger				drSigWord;		// 0x4244 for HFS
	aLongInt				drCrDate;		// Volume creation date
	aLongInt				drLsMod;		// Volume last modified date
	aInteger				drAtrb;			// Volume attributes
	aInteger				drNmFls;		// Number of files in root directory
	aInteger				drVBMSt;		// First block of volume bitmap
	aInteger				drAllocPtr;		// Start of next allocation search
	aInteger				drNmAlBlks;		// Number of allocation blocks in volume
	aLongInt				drAlBlkSiz;		// Allocation block size (bytes)
	aLongInt				drClpSiz;		// Default clump size
	aInteger				drAlBlSt;		// First allocation block in volume
	aLongInt				drNxtCNID;		// Next unused catalog node ID
	aInteger				drFreeBks;		// Number of unused allocation blocks
	aStr27					drVN;			// Volume name
// HFS-specific stuff
	aLongInt				drVolBkUp;		// date and time of last backup
	aInteger				drVSeqNum;		// volume backup sequence number
	aLongInt				drWrCnt;		// volume write count
	aLongInt				drXTClpSiz;		// clump size for extents overflow file
	aLongInt				drCTClpSiz;		// clump size for catalog file
	aInteger				drNmRtDirs;		// number of directories in root directory
	aLongInt				drFilCnt;		// number of files in volume
	aLongInt				drDirCnt;		// number of directories in volume
	aLongInt				drFndrInfo[8];		// information used by the Finder
	aInteger				drVCSize;		// size (in blocks) of volume cache
	aInteger				drVBMCSize;		// size (in blocks) of volume bitmap cache
	aInteger				drCtlCSize;		// size (in blocks) of common volume cache
	aLongInt				drXTFlSize;		// size of extents overflow file[1]
	aExtDataRec				drXTExtRec;		// extent record for extents overflow file
	aLongInt				drCTFlSize;		// size of catalog file[1]
	aExtDataRec				drCTExtRec;		// extent record for catalog file
} HFSAppleMDB;
// [1] Errata: Apple documents this field as one with units in allocation blocks. WRONG. It's in bytes.
//             Either that or the records somehow out-size the disk...

typedef struct {
   aOSType		fdType;					// TYPE
   aOSType		fdCreator;				// CREATOR
   aUInt16		fdFlags;				// Finder flags
   aPoint		fdLocation;				// location of icon within the window of the Finder
   aSInt16		fdFldr;					// window in which the file's icon appears in the Finder
} HFSAppleFinderInfo;

typedef struct FXInfo {
   aInteger		fdIconID;
   aInteger		fdReserved[3];
   unsigned char	fdScript;
   unsigned char	fdXFlags;
   aInteger		fdComment;
   aLongInt		fdPutAway;
} HFSAppleFinderXInfo;

typedef struct {
	aLongInt	ndFLink;		// forward link
	aLongInt	ndBLink;		// backward link
	unsigned char	ndType;			// node type
	unsigned char	ndNHeight;		// node level
	aInteger	ndNRecs;		// number of records in node
	aInteger	ndResv2;		// reserved
} HFSNodeDescriptor;

typedef struct {
	aInteger	bthDepth;		// current depth of tree
	aLongInt	bthRoot;		// number of root node
	aLongInt	bthNRecs;		// number of leaf records in tree
	aLongInt	bthFNode;		// number of first leaf node
	aLongInt	bthLNode;		// number of last leaf node
	aInteger	bthNodeSize;		// size of a node
	aInteger	bthKeyLen;		// maximum length of a key
	aLongInt	bthNNodes;		// total number of nodes in tree
	aLongInt	bthFree;		// number of free nodes
	unsigned char	bthResv[76];		// reserved
} HFSBTreeHeader;

typedef struct {
	unsigned char	ckrKeyLen;	/* key length */
	unsigned char	ckrResrv1;	/* reserved */
	aLongInt		ckrParID;	/* parent directory ID */
	aStr31			ckrCName;	/* catalog node name */
} HFSCatalogKeyRecord;

typedef struct {
	aLongInt	thdResrv[2];
	aLongInt	thdParID;
	aStr31		thdCName;
} HFSCatalogFileDataRec_DirectoryThread;

typedef struct {
	aLongInt	fthdResrv[2];
	aLongInt	fthdParID;
	aStr31		fthdCName;
} HFSCatalogFileDataRec_FileThread;

typedef struct {
	unsigned char		filFlags;
	unsigned char		filTyp;
	HFSAppleFinderInfo	filUsrWds;
	aLongInt			filFlNum;
	aInteger			filStBlk;
	aLongInt			filLgLen;
	aLongInt			filPyLen;
	aInteger			filRstBlk;
	aLongInt			filRLgLen;
	aLongInt			filRPyLen;
	aLongInt			filCrDat;
	aLongInt			filMdDat;
	aLongInt			filBkDat;
	HFSAppleFinderXInfo	filFndrInfo;
	aInteger			filClpSize;
	aExtDataRec			filExtRec;
	aExtDataRec			filRExtRec;
	aLongInt			filResrv;
} HFSCatalogFileDataRec_FileRecord;

typedef struct {
	aInteger		dirFlags;
	aInteger		dirVal;
	aLongInt		dirDirID;
	aLongInt		dirCrDat;
	aLongInt		dirMdDat;
// doesn't matter from here...
} HFSCatalogFileDataRec_DirRecord;

typedef struct {
	aInteger			offset;
	aInteger			size;
	unsigned char*		ptr;
} HFSNodePackEnt;

typedef struct {
	int					node;
	HFSNodeDescriptor	nd;
	HFSNodePackEnt		elem[64];
	unsigned char		buf[512];
} HFSNodePack;

class HFSDumper {
public:
	HFSDumper() {
		blk_dev=NULL;
		HFS_OneBlock=NULL;
		HFS_OneBlockSz=0;
		dirz=NULL;
		dirzalloc=0;
		MakeCvDirP=NULL;
	}
	~HFSDumper() {
		FreeDirz();
		HFSFreeOneBlock();
	}
public:
// used by this program to recall how folders are connected in HFS
	typedef struct {
		int				ID;
		aStr255			name;
		int				Parent;
	} HFSDirMemory;
public:
	BlockDevice*			blk_dev;
/* Master Directory Block & Allocation Block Map */
	HFSAppleMDB				MasterDirB;
	HFSBTreeHeader			extof_bth;
/* dedicated chunk of memory for handling allocation-block-sized chunks */
	unsigned char*			HFS_OneBlock;
	int						HFS_OneBlockSz;
/* directory memory */
	HFSDirMemory*			dirz;
	int						dirzalloc;
/* misc */
	char					MakeCvDir[8192];
	char*					MakeCvDirP;
public:
	int Assign(BlockDevice *blk);
	int IsBlockAlloc(unsigned int x);
	int CheckMDB();
	int HFSAllocOneBlock();
	int HFSFreeOneBlock();
	int ReadCatalogNodeData(int node,unsigned char *sector);
	int ReadExtentOverflowNodeData(int node,unsigned char *sector);
	int ReadCatalogNode(int node,HFSNodePack *np);
	int ReadExtentOverflowNode(int node,HFSNodePack *np);
	int IsBNodeAlloc(HFSNodePack *cnp,int node,int *state);
	void CopyHFSBTreeHeader(HFSBTreeHeader *h,unsigned char *p);
	int FreeDirz();
	int LookupDirz(aStr255 *name,int *parent,int *ID);
	int LookupDirzById(aStr255 *name,int *parent,int ID);
	int AddDirz(aStr255* name,int parent,int id);
	int MakeConvertToDir(int dId,int pId);
	char *GetMakeConvertDir();
	int MakeDir();
	int MapFileOffsetToDisk(int res,int fid,aExtDataRec ex,int faoffset,int *daoffset,int *dasize);
	int ExtractFile(HFSCatalogFileDataRec_FileRecord *fr,aExtDataRec ex,char *name,int res);
	int ExtractFileAppend(HFSCatalogFileDataRec_FileRecord *fr,aExtDataRec ex,FILE *fp,int res);
	int EnumDir(HFSNodePack *np);
	int EnumRootDir();
};

int HFSDumper::Assign(BlockDevice *blk)
{
	blk_dev = blk;
	return 0;
}

/* is the block allocated according to the VBM? */
int HFSDumper::IsBlockAlloc(unsigned int x)
{
	unsigned char buf[512];
	int i,so;

	if (x >= 65536) return 0;
	if (x >= MasterDirB.drNmAlBlks) return 0;
	i   = x>>3;
	so  = i>>9;
	i  &= 511;
	if (!blk_dev->ReadDisk(so+MasterDirB.drVBMSt,buf,1)) return 0;
	so  = 7-(x&7);
	return (int)((buf[i]>>so)&1);
}

int HFSDumper::CheckMDB()
{
	int i;
	char tmp[257];
	unsigned char buf[512];

	// read the MDB
	if (!blk_dev->ReadDisk(2,buf,1))
		return 0;
	// HFS only, please
	if ((MasterDirB.drSigWord = lbei16(buf)) != 0x4244)
		return 0;

	MasterDirB.drCrDate =					lbei32(	buf+0x02);
	MasterDirB.drLsMod =					lbei32(	buf+0x06);
	MasterDirB.drAtrb =						lbei16(	buf+0x0A);
	MasterDirB.drNmFls =					lbei16(	buf+0x0C);
	MasterDirB.drVBMSt =					lbei16(	buf+0x0E);
	MasterDirB.drAllocPtr =					lbei16(	buf+0x10);
	MasterDirB.drNmAlBlks =					lbei16(	buf+0x12);
	MasterDirB.drAlBlkSiz =					lbei32(	buf+0x14);
	MasterDirB.drClpSiz =					lbei32(	buf+0x18);
	MasterDirB.drAlBlSt =					lbei16(	buf+0x1C);
	MasterDirB.drNxtCNID =					lbei32(	buf+0x1E);
	MasterDirB.drFreeBks =					lbei16(	buf+0x22);
	aStr27cpy(&MasterDirB.drVN,						buf+0x24);
	MasterDirB.drVolBkUp =					lbei32(	buf+0x40);
	MasterDirB.drVSeqNum =					lbei16(	buf+0x44);
	MasterDirB.drWrCnt =					lbei32(	buf+0x46);
	MasterDirB.drXTClpSiz =					lbei32(	buf+0x4A);
	MasterDirB.drCTClpSiz =					lbei32(	buf+0x4E);
	MasterDirB.drNmRtDirs =					lbei16(	buf+0x52);
	MasterDirB.drFilCnt =					lbei32(	buf+0x54);
	MasterDirB.drDirCnt =					lbei32(	buf+0x58);
	for (i=0;i < 8;i++)
		MasterDirB.drFndrInfo[i] =			lbei32(	buf+0x5C+(i*4));
	MasterDirB.drVCSize =					lbei16(	buf+0x7C);
	MasterDirB.drVBMCSize =					lbei16(	buf+0x7E);
	MasterDirB.drCtlCSize =					lbei16(	buf+0x80);
	MasterDirB.drXTFlSize =					lbei32(	buf+0x82);
	MasterDirB.drXTExtRec[0].xdrStABN =		lbei16(	buf+0x86);
	MasterDirB.drXTExtRec[0].xdrNumABlks =	lbei16(	buf+0x88);
	MasterDirB.drXTExtRec[1].xdrStABN =		lbei16(	buf+0x8A);
	MasterDirB.drXTExtRec[1].xdrNumABlks =	lbei16(	buf+0x8C);
	MasterDirB.drXTExtRec[2].xdrStABN =		lbei16(	buf+0x8E);
	MasterDirB.drXTExtRec[2].xdrNumABlks =	lbei16(	buf+0x90);
	MasterDirB.drCTFlSize =					lbei32(	buf+0x92);
	MasterDirB.drCTExtRec[0].xdrStABN =		lbei16(	buf+0x96);
	MasterDirB.drCTExtRec[0].xdrNumABlks =	lbei16(	buf+0x98);
	MasterDirB.drCTExtRec[1].xdrStABN =		lbei16(	buf+0x9A);
	MasterDirB.drCTExtRec[1].xdrNumABlks =	lbei16(	buf+0x9C);
	MasterDirB.drCTExtRec[2].xdrStABN =		lbei16(	buf+0x9E);
	MasterDirB.drCTExtRec[2].xdrNumABlks =	lbei16(	buf+0xA0);

	/* all Macintosh floppies I've ever seen are 512-byte sector formatted, so if
	   the allocation block size is not a multiple of this we have a problem. */
	if ((MasterDirB.drAlBlkSiz&511) != 0) {
		printf("ERROR: Allocation block size not a multiple of 512!\n");
		return 0;
	}

	/* Show the user we found the volume by printing the volume name */
	PascalToCString(tmp,(PascalString*)(&MasterDirB.drVN),sizeof(tmp));
	printf("...Volume name: %s\n",tmp);

	return 1;
}

int HFSDumper::HFSAllocOneBlock()
{
	if (HFS_OneBlock) return 1;
	HFS_OneBlockSz = MasterDirB.drAlBlkSiz;
	HFS_OneBlock = (unsigned char*)malloc(HFS_OneBlockSz);
	if (!HFS_OneBlock) return 0;
	return 1;
}

int HFSDumper::HFSFreeOneBlock()
{
	if (!HFS_OneBlock) return 1;
	free(HFS_OneBlock);
	HFS_OneBlock=NULL;
	HFS_OneBlockSz=0;
	return 1;
}

// handles reading catalog nodes even across different extents
int HFSDumper::ReadCatalogNodeData(int node,unsigned char *sector)
{
	int so,i,sa;

	if (node < 0) return 0;

	i  = 0;
	so = (MasterDirB.drCTExtRec[0].xdrStABN *    MasterDirB.drAlBlkSiz) >> 9;
	sa = (MasterDirB.drCTExtRec[0].xdrNumABlks * MasterDirB.drAlBlkSiz) >> 9;
	while (node >= sa && i < 2) {
		node -= sa;
		i++;
		so = (MasterDirB.drCTExtRec[i].xdrStABN *    MasterDirB.drAlBlkSiz) >> 9;
		sa = (MasterDirB.drCTExtRec[i].xdrNumABlks * MasterDirB.drAlBlkSiz) >> 9;
	}

	if (node < 0 || node >= sa) return 0;
	so += MasterDirB.drAlBlSt + node;
	if (!blk_dev->ReadDisk(so,sector,1)) return 0;
	return 1;
}

// handles reading extent overflow nodes even across different extents
int HFSDumper::ReadExtentOverflowNodeData(int node,unsigned char *sector)
{
	int so,i,sa;

	if (node < 0) return 0;

	i  = 0;
	so = (MasterDirB.drXTExtRec[0].xdrStABN *    MasterDirB.drAlBlkSiz) >> 9;
	sa = (MasterDirB.drXTExtRec[0].xdrNumABlks * MasterDirB.drAlBlkSiz) >> 9;
	while (node >= sa && i < 2) {
		node -= sa;
		i++;
		so = (MasterDirB.drXTExtRec[i].xdrStABN *    MasterDirB.drAlBlkSiz) >> 9;
		sa = (MasterDirB.drXTExtRec[i].xdrNumABlks * MasterDirB.drAlBlkSiz) >> 9;
	}

	if (node < 0 || node >= sa) return 0;
	so += MasterDirB.drAlBlSt + node;
	if (!blk_dev->ReadDisk(so,sector,1)) return 0;
	return 1;
}

int HFSDumper::ReadCatalogNode(int node,HFSNodePack *np)
{
	unsigned char *buf = np->buf;
	int o,maxx;

	if (!ReadCatalogNodeData(node,buf))
		return 0;

	/* node descriptor */
	np->node =						node;
	np->nd.ndFLink =		lbei32(	buf+0x0);
	np->nd.ndBLink =		lbei32(	buf+0x4);
	np->nd.ndType =					buf[0x8];
	np->nd.ndNHeight =				buf[0x9];
	np->nd.ndNRecs =		lbei16(	buf+0xA);
	np->nd.ndResv2 =		lbei16(	buf+0xC);
	maxx =					512-(np->nd.ndNRecs*2);

	if (np->nd.ndNRecs >= 64) {
		printf("ReadCatalogNode(): Node has too many entries!\n");
		return 0;
	}

	for (o=0;o <= np->nd.ndNRecs;o++)
		np->elem[o].offset =	lbei16((buf+510)-(o*2));

	np->elem[np->nd.ndNRecs].ptr = NULL;
	np->elem[np->nd.ndNRecs].size = 0;
	if (np->elem[np->nd.ndNRecs].offset == 0)
		np->elem[np->nd.ndNRecs].offset = 512-(np->nd.ndNRecs*2);

	for (o=0;o < np->nd.ndNRecs;o++) {
		np->elem[o].ptr = np->buf + np->elem[o].offset;
		np->elem[o].size = np->elem[o+1].offset - np->elem[o].offset;
		if (np->elem[o].size < 0 || np->elem[o].offset >= maxx) {
			printf("ReadCatalogNode(): Node has invalid offsets\n");
			return 0;
		}
	}

	if (np->elem[0].offset != 14)
		printf("ReadCatalogNode(): First offset in node, %u != 14, which is unusual\n",np->elem[0].offset);

	return 1;
}

int HFSDumper::ReadExtentOverflowNode(int node,HFSNodePack *np)
{
	unsigned char *buf = np->buf;
	int o,maxx;

	if (!ReadExtentOverflowNodeData(node,buf))
		return 0;

	/* node descriptor */
	np->node =							node;
	np->nd.ndFLink =			lbei32(	buf+0x0);
	np->nd.ndBLink =			lbei32(	buf+0x4);
	np->nd.ndType =						buf[0x8];
	np->nd.ndNHeight =					buf[0x9];
	np->nd.ndNRecs =			lbei16(	buf+0xA);
	np->nd.ndResv2 =			lbei16(	buf+0xC);
	maxx =						512-(np->nd.ndNRecs*2);

	if (np->nd.ndNRecs >= 64) {
		printf("ReadExtentOverflowNode(): Node has too many entries!\n");
		return 0;
	}

	for (o=0;o <= np->nd.ndNRecs;o++)
		np->elem[o].offset =	lbei16((buf+510)-(o*2));

	np->elem[np->nd.ndNRecs].ptr = NULL;
	np->elem[np->nd.ndNRecs].size = 0;
	if (np->elem[np->nd.ndNRecs].offset == 0)
		np->elem[np->nd.ndNRecs].offset = 512-(np->nd.ndNRecs*2);

	for (o=0;o < np->nd.ndNRecs;o++) {
		np->elem[o].ptr = np->buf + np->elem[o].offset;
		np->elem[o].size = np->elem[o+1].offset - np->elem[o].offset;
		if (np->elem[o].size < 0 || np->elem[o].offset >= maxx) {
			printf("ReadCatalogNode(): Node has invalid offsets\n");
			return 0;
		}
	}

	if (np->elem[0].offset != 14)
		printf("ReadExtentOverflowNode(): First offset in node != 14, which is unusual\n");

	return 1;
}

// given a B*-tree header node read the node
// allocation map and see if one is allocated.
int HFSDumper::IsBNodeAlloc(HFSNodePack *cnp,int node,int *state)
{
	HFSNodePack pnp;
	HFSNodePack *np = cnp;
	int mo=0,pf,ph,sh,so;

	if (np->nd.ndNRecs < 3)
		return 0;

	/* move down the chain if necessary */
	while (node >= (np->elem[2].size<<3)) {
		node -= np->elem[2].size<<3;

		// HACK: This code isn't working properly for one
		//       HFS-based hard drive image (forward link is invalid!)
		*state = 1;
		return 1;

		ph = np->nd.ndNHeight;
		pf = np->nd.ndFLink;
		if (np->nd.ndFLink == 0) {
			printf("IsBNodeAlloc(): Unable to find map, forward link not available\n");
			return 0;
		}

		if (!ReadCatalogNode(np->nd.ndFLink,&pnp)) {
			printf("IsBNodeAlloc(): Unable to read map, forward link unreadable\n");
			return 0;
		}

		np = &pnp;
		/* it must be a map node, and it must be the same height, and
		   the back link must point to where we just came from! It also
		   must have at least one entry! */
		if (	np->nd.ndBLink != pf  || np->nd.ndNHeight != ph ||
				np->nd.ndType != 0x02 || np->nd.ndNRecs < 1) {
				printf("IsBNodeAlloc(): Unable to read map, forward link corrupt\n");
				return 0;
		}
	}

	so = node>>3;
	sh = 7-(node&7);
	*state = (int)((np->elem[2].ptr[so]>>sh)&1);
	return 1;
}

void HFSDumper::CopyHFSBTreeHeader(HFSBTreeHeader *h,unsigned char *p)
{
	h->bthDepth =		lbei16(	p+0x00);
	h->bthRoot =		lbei32(	p+0x02);
	h->bthNRecs =		lbei32(	p+0x06);
	h->bthFNode =		lbei32(	p+0x0A);
	h->bthLNode =		lbei32(	p+0x0E);
	h->bthNodeSize =	lbei16(	p+0x12);
	h->bthKeyLen =		lbei16(	p+0x14);
	h->bthNNodes =		lbei32(	p+0x16);
	h->bthFree =		lbei32(	p+0x1A);
	memcpy(h->bthResv,			p+0x1E,	76);
}

int HFSDumper::FreeDirz()
{
	if (dirz) free(dirz);
	dirzalloc=0;
	dirz=NULL;
	return 1;
}

int HFSDumper::LookupDirz(aStr255 *name,int *parent,int *ID)
{
	int i=0;

	if (!dirz || !ID || !parent) return 0;

	while (i < dirzalloc && PascalCmp((PascalString*)name,(PascalString*)(&dirz[i].name)))
		i++;

	if (i >= dirzalloc)
		return 0;

	*parent =	dirz[i].Parent;
	*ID =		dirz[i].ID;
	return 1;
}

int HFSDumper::LookupDirzById(aStr255 *name,int *parent,int ID)
{
	int i=0;

	if (!dirz || !ID || !parent) return 0;

	while (i < dirzalloc && (dirz[i].ID != ID || dirz[i].ID == 0))
		i++;

	if (i >= dirzalloc)
		return 0;

	*parent = dirz[i].Parent;
	name->len = dirz[i].name.len;
	memcpy(name->str,dirz[i].name.str,name->len);
	name->str[name->len]=0;
	return 1;
}

int HFSDumper::AddDirz(aStr255* name,int parent,int id)
{
	aStr255 t;
	int i;

	t.len = name->len;
	memcpy(t.str,name->str,name->len);
	t.str[t.len] = 0;
	if (t.len == 0) return 0;
	if (id == 0) return 0;

	if (dirz && dirzalloc == 0)
		return 0;

	if (!dirz) {
		dirzalloc=64;
		dirz=(HFSDirMemory*)malloc(sizeof(HFSDirMemory) * dirzalloc);
		if (!dirz) return 0;
		for (i=0;i < dirzalloc;i++) {
			dirz[i].ID=0;
			dirz[i].Parent=0;
			dirz[i].name.len=0;
		}
		i=0;
	}
	else if (dirz[dirzalloc-1].ID != 0) {
		int od = dirzalloc;
		dirzalloc += 64;
		dirz=(HFSDirMemory*)realloc(dirz,sizeof(HFSDirMemory) * dirzalloc);
		if (!dirz) return 0;
		for (i=od;i < dirzalloc;i++) {
			dirz[i].ID=0;
			dirz[i].Parent=0;
			dirz[i].name.len=0;
		}
		i=od;
	}
	else {
		i=0;
		while (dirz[i].ID != 0 && i < dirzalloc) i++;
		if (i >= dirzalloc) return 0;
	}

	dirz[i].ID = id;
	dirz[i].Parent = parent;
	memcpy(&dirz[i].name,&t,sizeof(aStr255));
	return 1;
}

// make a relative directory path from a directory ID
int HFSDumper::MakeConvertToDir(int dId,int pId)
{
	int i=sizeof(MakeCvDir)-1;
	int parent,l=0;
	aStr255 pname;

	MakeCvDir[i]=0;
	MakeCvDirP=MakeCvDir+i;
	if (dId <= 1) return 1;

	if (!LookupDirzById(&pname,&parent,dId))
		return 0;
	if (parent != pId && pId > 0)
		return 0;

	pname.str[pname.len]=0;
	MacFnFilter((char*)pname.str);

	do {
		if (l == 0) {
			i -= pname.len;
			MakeCvDirP=MakeCvDir+i;
			memcpy(MakeCvDirP,pname.str,pname.len);
		}
		else {
			i -= pname.len+1;
			MakeCvDirP=MakeCvDir+i;
			memcpy(MakeCvDirP,pname.str,pname.len);
			MakeCvDirP[pname.len] = '\\';
		}
		l += pname.len;

		/* keep going */
		dId = parent;
		if (parent > 1) {
			if (!LookupDirzById(&pname,&parent,dId)) {
				MakeCvDir[i  ]='.';
				MakeCvDir[i+1]='\\';
				MakeCvDir[i+2]=0;
				return 0;
			}

			pname.str[pname.len]=0;
			MacFnFilter((char*)pname.str);
		}
	} while (dId > 1);

	i -= 2;
	MakeCvDir[i  ]='.';
	MakeCvDir[i+1]='\\';
	MakeCvDirP=MakeCvDir+i;
	return 1;
}

char *HFSDumper::GetMakeConvertDir()
{
	return MakeCvDirP;
}

int HFSDumper::MakeDir()
{
	char *s,*p;
	int e=1;

	p=MakeCvDirP;
	do {
		s=strchr(p,'\\');
		if (!s) {
			s=p+strlen(p);
			e=0;
		}
		else {
			e=1;
		}
		if (e) *s=0;

		if (strlen(MakeCvDirP) > 0) {
			/* if we can't create the directory,
			   test if it already exists */
			if (mkdir(MakeCvDirP)) {
				char cwd[1024];

				GetCurrentDirectory(1023,cwd);
				if (chdir(MakeCvDirP))
					return 0;
				if (chdir(cwd))
					return 0;
			}
		}

		if (e) *s='\\';
		p=s+1;
	} while (e);

	return 1;
}

int HFSDumper::MapFileOffsetToDisk(int res,int fid,aExtDataRec ex,int faoffset,int *daoffset,int *dasize)
{
	HFSNodePack np;
	aExtDataRec rex,prex;
	int fo=faoffset,ei,i,no,ni,gni;
	unsigned char xkrKeyLen;
	unsigned char xkrFkType;
	unsigned char *pp;
	unsigned char deType = (res ? 0xFF : 0x00);
	aLongInt xkrFNum;
	aInteger xkrFABN;
	int lastFABN,rootRef;

	if (fid <= 0 || !daoffset || !dasize) return 0;

	for (ei=0;ei <= 2 && fo >= ex[ei].xdrNumABlks;) {
		fo -= ex[ei].xdrNumABlks;
		ei++;
	}

	if (ei <= 2 && fo < ex[ei].xdrNumABlks) {
		*daoffset =	ex[ei].xdrStABN+fo;
		*dasize =	ex[ei].xdrNumABlks;
		return 1;
	}

	fo=faoffset;
	if (!ReadExtentOverflowNode(extof_bth.bthRoot,&np)) {
		printf("....ERROR: Cannot read extent overflow root node\n");
		return 0;
	}

#if 0
	// Apple doesn't make this clear but one can do a rough search for
	// the offset and file extents and come up quickly with which node
	// to read by first reading the root node.
	no = extof_bth.bthRoot;
	gni = 0;
	lastFABN = -1;
	lastRootRef = -1;
	rootRef = 0;
	while (no != 0 && gni == 0) {
		for (i=0;i < np.nd.ndNRecs && gni == 0;i++) {
			pp = np.elem[i].ptr;
			if (np.elem[i].size >= 12) {
				xkrKeyLen =				pp[0x00];
				xkrFkType =				pp[0x01];
				xkrFNum =		lbei32(	pp+0x02);
				xkrFABN =		lbei16(	pp+0x06);
				xkrRootRef =	lbei32(	pp+0x08);
				if (xkrKeyLen >= 7) {
					if (xkrFkType == deType) {
						if (xkrFNum == fid) {
							if (lastFABN < 0) {
								lastFABN = xkrFABN;
								lastRootRef = xkrRootRef;
							}
							else if (xkrFABN > faoffset) {
								gni = lastFABN;
								rootRef = lastRootRef;
							}
							else {
								lastFABN = xkrFABN;
								lastRootRef = xkrRootRef;
							}
						}
					}
				}
				else {
					printf("....WARNING: Unusual key size in B*-tree extent overflow header\n");
				}
			}
			else {
				printf("....WARNING: Undersized entry in extent overflow B*-tree header\n");
			}
		}

		/* next node */
		if (gni == 0) {
			no = np.nd.ndFLink;
			if (no > 0) {
				if (!ReadExtentOverflowNode(no,&np)) {
					printf("....ERROR: Cannot read extent overflow root node\n");
					return 0;
				}
			}
		}
	}

	if (gni <= 0 && lastFABN > 0) {
		gni = lastFABN;
		rootRef = lastRootRef;
	}
#else
	rootRef = 0;
	gni = 0;
#endif

	/* now improve the search, using the hints given (if any) from the above search */
	ni = 0;
	lastFABN = 0;
	{
		no = rootRef > 0 ? rootRef : extof_bth.bthFNode;
		if (!ReadExtentOverflowNode(no,&np)) {
			printf("....ERROR: Cannot read extent overflow root node\n");
			return 0;
		}

		ni = 0;
		lastFABN = -1;
		while (no != 0 && ni == 0) {
			for (i=0;i < np.nd.ndNRecs && ni == 0;i++) {
				pp = np.elem[i].ptr;
				if (np.elem[i].size >= 20) {
					xkrKeyLen =						pp[0x00];
					xkrFkType =						pp[0x01];
					xkrFNum =				lbei32(	pp+0x02);
					xkrFABN =				lbei16(	pp+0x06);
					rex[0].xdrStABN =		lbei16(	pp+0x08);
					rex[0].xdrNumABlks =	lbei16(	pp+0x0A);
					rex[1].xdrStABN =		lbei16(	pp+0x0C);
					rex[1].xdrNumABlks =	lbei16(	pp+0x0E);
					rex[2].xdrStABN =		lbei16(	pp+0x10);
					rex[2].xdrNumABlks =	lbei16(	pp+0x12);
					if (xkrKeyLen >= 7) {
						if (xkrFkType == deType) {
							if (xkrFNum == fid) {
								if (lastFABN < 0) {
									memcpy(&prex,&rex,sizeof(rex));
									lastFABN = xkrFABN;
								}
								else if (xkrFABN > faoffset) {
									ni = lastFABN;
								}
								else {
									memcpy(&prex,&rex,sizeof(rex));
									lastFABN = xkrFABN;
								}
							}
						}
					}
					else {
						printf("....WARNING: Unusual key size in B*-tree extent overflow header\n");
					}
				}
				else {
					printf("....WARNING: Undersized entry in extent overflow B*-tree header\n");
				}
			}

			/* next node */
			if (ni == 0) {
				if (np.nd.ndFLink > 0) {
					no = np.nd.ndFLink;
					if (!ReadExtentOverflowNode(no,&np)) {
						printf("....ERROR: Cannot read extent overflow root node\n");
						return 0;
					}
				}
				else {
					no = 0;
				}
			}
		}

		if (ni <= 0) {
			if (lastFABN > 0)	ni = lastFABN;
			else			return 0;
		}

		if (ni > 0) {
			/* now find the exact allocation block */
			int eei=0;

			fo -= ni;
			if (fo < 0) {
//				printf("....ERROR: File allocation block numbering error\n");
				return 0;
			}

			while (eei <= 2 && fo >= prex[eei].xdrNumABlks) {
				fo -= prex[eei].xdrNumABlks;
				eei++;
			}

			if (eei <= 2 && fo < prex[eei].xdrNumABlks) {
				*daoffset =	prex[eei].xdrStABN+fo;
				*dasize =	prex[eei].xdrNumABlks;
				return 1;
			}
			else {
				return 0;
			}
		}
	}

	return 0;
}

int HFSDumper::ExtractFile(HFSCatalogFileDataRec_FileRecord *fr,aExtDataRec ex,char *name,int res)
{
	int su,i,sz,aol,aod,aof,ma,osz;
	FILE *fp;

	fp=fopen(name,"wb");
	if (!fp) return 0;

	su  = ex[0].xdrNumABlks + ex[1].xdrNumABlks + ex[2].xdrNumABlks;
	su *= MasterDirB.drAlBlkSiz;
	if (res) {
		sz = fr->filRLgLen;
//		if (su != fr->filRPyLen)
//			printf("....WARNING: File size does not match sum of extents\n");
	}
	else {
		sz = fr->filLgLen;
//		if (su != fr->filPyLen)
//			printf("....WARNING: File size does not match sum of extents\n");
	}

	ma=MasterDirB.drAlBlkSiz>>9;
	osz=sz;
	aol=0;
	aof=0;
	while (sz > 0) {
		if (aol <= 0) {
			if (!MapFileOffsetToDisk(res,fr->filFlNum,ex,aof,&aod,&aol)) {
				printf("....WARNING: Missing logical block %u in file (sparse file)?\n",aof);
				memset(HFS_OneBlock,0,HFS_OneBlockSz);
				aol =  1;
				aod = -1;
			}
		}

		if (aol > 0) {
			if (aod < 0) {
			}
			else if (!blk_dev->ReadDisk((aod*ma)+MasterDirB.drAlBlSt,HFS_OneBlock,ma)) {
				printf("....ERROR: Cannot read sector %u\n",(aod*ma)+MasterDirB.drAlBlSt);
				memset(HFS_OneBlock,0,ma * 512);
			}

			i = (sz > HFS_OneBlockSz) ? HFS_OneBlockSz : sz;
			sz -= i;
			fwrite(HFS_OneBlock,i,1,fp);
			aol--;
			aod++;
			aof++;
		}
		else {
			printf("....WARNING: Extent read is zero at file block offset %u disk offset %u\n",aof,aod);
		}
	}

	if (sz > 0)
		printf("....WARNING: Wasn't able to extract the entire file\n");

	fclose(fp);
	return 1;
}

int HFSDumper::ExtractFileAppend(HFSCatalogFileDataRec_FileRecord *fr,aExtDataRec ex,FILE *fp,int res)
{
	int su,i,sz,aol,aod,aof,ma,osz;

	fseek(fp,0,SEEK_END);

	su  = ex[0].xdrNumABlks + ex[1].xdrNumABlks + ex[2].xdrNumABlks;
	su *= MasterDirB.drAlBlkSiz;
	if (res) {
		sz = fr->filRLgLen;
//		if (su != fr->filRPyLen)
//			printf("....WARNING: File size does not match sum of extents\n");
	}
	else {
		sz = fr->filLgLen;
//		if (su != fr->filPyLen)
//			printf("....WARNING: File size does not match sum of extents\n");
	}

	ma=MasterDirB.drAlBlkSiz>>9;
	osz=sz;
	aol=0;
	aof=0;
	while (sz > 0) {
		if (aol <= 0) {
			if (!MapFileOffsetToDisk(res,fr->filFlNum,ex,aof,&aod,&aol)) {
				printf("....WARNING: Missing logical block %u in file (sparse file)?\n",aof);
				memset(HFS_OneBlock,0,HFS_OneBlockSz);
				aol =  1;
				aod = -1;
			}
		}

		if (aol > 0) {
			if (aod < 0) {
			}
			else if (!blk_dev->ReadDisk((aod*ma)+MasterDirB.drAlBlSt,HFS_OneBlock,ma)) {
				printf("....ERROR: Cannot read sector %u\n",(aod*ma)+MasterDirB.drAlBlSt);
				memset(HFS_OneBlock,0,ma * 512);
			}

			i = (sz > HFS_OneBlockSz) ? HFS_OneBlockSz : sz;
			sz -= i;
			fwrite(HFS_OneBlock,i,1,fp);
			aol--;
			aod++;
			aof++;
		}
		else {
			printf("....WARNING: Extent read is zero at file block offset %u disk offset %u\n",aof,aod);
		}
	}

	if (sz > 0)
		printf("....WARNING: Wasn't able to extract the entire file\n");

	return 1;
}

static void WriteBlankMacBinary(FILE *fp,char *name,HFSCatalogFileDataRec_FileRecord *fr)
{
	unsigned char buf[128];

	memset(buf,0,sizeof(buf));
	buf[1] = strlen(name);
	strcpy((char*)(buf+2),name);
	memcpy(buf+65,fr->filUsrWds.fdType,4);
	memcpy(buf+69,fr->filUsrWds.fdCreator,4);
	buf[73] = fr->filUsrWds.fdFlags >> 8;
	sbei32(buf+83,fr->filLgLen);
	sbei32(buf+87,fr->filRLgLen);
	sbei32(buf+91,fr->filCrDat);
	sbei32(buf+95,fr->filMdDat);
	buf[101] = fr->filUsrWds.fdFlags;
	memcpy(buf+102,"mBIN",4);
	buf[122] = 130;
	buf[123] = 129;
	// fix the CRC later
	fwrite(buf,128,1,fp);
}

static void MacBinaryPad128(FILE *fp)
{
	unsigned char buf[128];
	unsigned long ofs;
	int fill;

	fseek(fp,0,SEEK_END);
	ofs = ftell(fp);
	fill = 128 - (ofs & 127); if (fill == 128) return;
	memset(buf,0,sizeof(buf));
	fwrite(buf,fill,1,fp);
}

static void UpdateMacBinary(FILE *fp,unsigned long data,unsigned long res)
{
	unsigned char buf[128];
	unsigned short crc;
	int i;

	fseek(fp,0,SEEK_SET);
	fread(buf,128,1,fp);

	sbei32(buf+83,data);
	sbei32(buf+87,res);
	sbei32(buf+116,data+res);

	crc = 0;
	for (i=0;i < 124;i++)
		crc = updatecrc(buf[i],crc);

	sbei16(buf+124,crc);

	fseek(fp,0,SEEK_SET);
	fwrite(buf,128,1,fp);
}

// given a B*-tree header node enumerate the contents of a directory
int HFSDumper::EnumDir(HFSNodePack *np)
{
	HFSCatalogKeyRecord cat;
	HFSBTreeHeader bth;
	HFSNodePack sn;
	HFSNodePack ep;
	char tmp[256];
	unsigned char *pp;
	int bitset,node,nodes,i,j;
	int cdrType,cdrResrv;

	if (np->nd.ndNRecs < 3) {
		printf("ERROR: B*-tree header node has insufficient records!\n");
		return 0;
	}
	if (np->nd.ndType != 1)
		return 1;

	/* make sure the tree doesn't deny it's own existience */
	if (!IsBNodeAlloc(np,np->node,&bitset)) {
		printf("ERROR: Cannot read B*-tree map!\n");
		return 0;
	}

	if (!bitset)
		printf("WARNING: B*-tree header denies own existience!\n");

	if (np->elem[0].size < 106) {
		printf("WARNING: B*-tree header too small\n");
		return 0;
	}

	CopyHFSBTreeHeader(&bth,np->elem[0].ptr);

	if (MasterDirB.drXTFlSize > 0) {
		if (!ReadExtentOverflowNode(0,&ep)) {
			printf("ERROR: Cannot read extent overflow file\n");
			return 0;
		}

		CopyHFSBTreeHeader(&extof_bth,ep.elem[0].ptr);
	}

	/* read the catalog file */
	node = bth.bthRoot;
	nodes = bth.bthNNodes;
	while (nodes-- > 0 && node != 0) {
		if (!ReadCatalogNode(node,&sn)) {
			printf("ERROR: Cannot read B*-tree node %u\n",node);
		}
		else if (sn.nd.ndType != 0x00) {
			printf("WARNING: Node %u has wrong type\n",node);
		}
		else if (sn.nd.ndNHeight != bth.bthDepth) {
			printf("WARNING: Node %u has wrong depth\n",node);
		}
		else if (!IsBNodeAlloc(np,sn.node,&bitset)) {
			printf("ERROR: Cannot read B*-tree map!\n");
			return 0;
		}
		else {
			if (!bitset)
				printf("WARNING: B*-tree header denies existience of node %u!\n",node);

			/* okay, pick it apart */
			for (i=0;i < sn.nd.ndNRecs;i++) {
				pp = sn.elem[i].ptr;
				cat.ckrKeyLen = pp[0x00];
				if (cat.ckrKeyLen >= sn.elem[i].size) {
					printf("WARNING: Catalog entry has oversized key\n");
				}
				else if (cat.ckrKeyLen > 0) {
					cat.ckrResrv1 =				pp[0x01];
					cat.ckrParID =		lbei32(	pp+0x02);
					aStr31cpy(&cat.ckrCName,	pp+0x06);

					PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
					printf("   Catalog ent, PID %5u: %s\n",cat.ckrParID,tmp);
				}
			}
		}

		/* find the next link in the chain */
		node = sn.nd.ndFLink;
	}

	/* sift through the leaves */
	node = bth.bthFNode;
	nodes = bth.bthNNodes;
	while (nodes-- > 0 && node != 0) {
		if (!ReadCatalogNode(node,&sn)) {
			printf("ERROR: Cannot read B*-tree node %u\n",node);
		}
		else if (sn.nd.ndType != 0xFF) {
			printf("WARNING: Node %u has wrong type\n",node);
		}
//		else if (sn.nd.ndNHeight != bth.bthDepth) {
//			printf("WARNING: Node %u has wrong depth\n",node);
//		}
		else if (!IsBNodeAlloc(np,sn.node,&bitset)) {
			printf("ERROR: Cannot read B*-tree map!\n");
			return 0;
		}
		else {
			if (!bitset)
				printf("WARNING: B*-tree header denies existience of node %u!\n",node);

			/* okay, pick it apart */
			for (i=0;i < sn.nd.ndNRecs;i++) {
				pp = sn.elem[i].ptr;
				cat.ckrKeyLen = pp[0x00];
				if (cat.ckrKeyLen >= sn.elem[i].size) {
					printf("WARNING: Catalog entry has oversized key\n");
				}
				else if (cat.ckrKeyLen > 0) {
					cat.ckrResrv1 =				pp[0x01];
					cat.ckrParID =		lbei32(	pp+0x02);
					aStr31cpy(&cat.ckrCName,	pp+0x06);

					pp += cat.ckrKeyLen+1;
					if ((pp-sn.buf)&1) pp++;
					cdrType = *pp++;
					cdrResrv = *pp++;

					if (cdrType == 2) {
						HFSCatalogFileDataRec_FileRecord fr;

						PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
						printf("   Leaf, PID %5u: %s\n",cat.ckrParID,tmp);

						fr.filFlags =						pp[0x00];
						fr.filTyp =							pp[0x01];
						memcpy(fr.filUsrWds.fdType,			pp+0x02,	4);
						memcpy(fr.filUsrWds.fdCreator,		pp+0x06,	4);
						fr.filUsrWds.fdFlags =		lbei16(	pp+0x0A);
						fr.filUsrWds.fdLocation.v =	lbei16(	pp+0x0C);
						fr.filUsrWds.fdLocation.h =	lbei16(	pp+0x0E);
						fr.filUsrWds.fdFldr =		lbei16(	pp+0x10);
						fr.filFlNum =				lbei32(	pp+0x12);
						fr.filStBlk =				lbei16(	pp+0x16);
						fr.filLgLen =				lbei32(	pp+0x18);
						fr.filPyLen =				lbei32(	pp+0x1C);
						fr.filRstBlk =				lbei16(	pp+0x20);
						fr.filRLgLen =				lbei32(	pp+0x22);
						fr.filRPyLen =				lbei32(	pp+0x26);
						fr.filCrDat =				lbei32(	pp+0x2A);
						fr.filMdDat =				lbei32(	pp+0x2E);
						fr.filBkDat =				lbei32(	pp+0x32);
						fr.filFndrInfo.fdIconID =	lbei16(	pp+0x36);
						for (j=0;j < 3;j++)
							fr.filFndrInfo.fdReserved[j] =
													lbei16(	pp+0x38+(j*2));
						fr.filFndrInfo.fdScript =			pp[0x3E];
						fr.filFndrInfo.fdXFlags =			pp[0x3F];
						fr.filFndrInfo.fdComment =	lbei16(	pp+0x40);
						fr.filFndrInfo.fdPutAway =	lbei32(	pp+0x42);
						fr.filClpSize =				lbei16(	pp+0x46);
						for (j=0;j < 3;j++) {
							fr.filExtRec[j].xdrStABN =
										lbei16(	pp+0x48+(j*4));
							fr.filExtRec[j].xdrNumABlks =
										lbei16(	pp+0x48+(j*4)+2);
						}
						for (j=0;j < 3;j++) {
							fr.filRExtRec[j].xdrStABN =
										lbei16(	pp+0x54+(j*4));
							fr.filRExtRec[j].xdrNumABlks =
										lbei16(	pp+0x54+(j*4)+2);
						}
						fr.filResrv = lbei32(	pp+0x60);
					}
					else if (cdrType == 1) {
						HFSCatalogFileDataRec_DirRecord dr;

						dr.dirFlags =			lbei16(	pp+0x00);
						dr.dirVal =				lbei16(	pp+0x02);
						dr.dirDirID =			lbei32(	pp+0x04);

						PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
						printf("   Leaf, PID %5u: %s  (directory ID %u)\n",cat.ckrParID,tmp,dr.dirDirID);
						AddDirz((aStr255*)(&cat.ckrCName),cat.ckrParID,dr.dirDirID);
					}
					else if (cdrType == 3) {
						HFSCatalogFileDataRec_DirectoryThread dthr;

						if (cat.ckrCName.len > 0) {
							PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
							printf("   Leaf, PID %5u: %s\n",cat.ckrParID,tmp);
						}

						dthr.thdResrv[0] =	lbei32(	pp+0x00);
						dthr.thdResrv[1] =	lbei32(	pp+0x04);
						dthr.thdParID =		lbei32(	pp+0x08);
						aStr31cpy(&dthr.thdCName,	pp+0x0C);
						PascalToCString(tmp,(PascalString*)(&dthr.thdCName),sizeof(tmp));
						printf("   Leaf, PID %5u: %s  (Directory Thread PID %u)\n",cat.ckrParID,tmp,dthr.thdParID);
					}
					else if (cdrType == 4) {
						HFSCatalogFileDataRec_FileThread fthr;

						if (cat.ckrCName.len > 0) {
							PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
							printf("   Leaf, PID %5u: %s\n",cat.ckrParID,tmp);
						}

						fthr.fthdResrv[0] =	lbei32(	pp+0x00);
						fthr.fthdResrv[1] =	lbei32(	pp+0x04);
						fthr.fthdParID =	lbei32(	pp+0x08);
						aStr31cpy(&fthr.fthdCName,	pp+0x0C);
						PascalToCString(tmp,(PascalString*)(&fthr.fthdCName),sizeof(tmp));
						printf("   Leaf, PID %5u: %s  (File Thread PID %u)\n",cat.ckrParID,tmp,fthr.fthdParID);
					}
					else {
						PascalToCString(tmp,(PascalString*)(&cat.ckrCName),sizeof(tmp));
						printf("   Leaf, PID %5u: %s  (UNKNOWN CATALOG RECORD TYPE %u)\n",cat.ckrParID,tmp,cdrType);
					}
				}
			}
		}

		/* find the next link in the chain */
		if (node == bth.bthLNode && sn.nd.ndFLink != 0)
			printf("WARNING: B*-tree chain extends past last node\n");

		node = sn.nd.ndFLink;
	}

	/* sift through the leaves again and create folders */
	node = bth.bthFNode;
	nodes = bth.bthNNodes;
	while (nodes-- > 0 && node != 0) {
		if (!ReadCatalogNode(node,&sn)) {
			printf("ERROR: Cannot read B*-tree node %u\n",node);
		}
		else if (sn.nd.ndType != 0xFF) {
			printf("WARNING: Node %u has wrong type\n",node);
		}
//		else if (sn.nd.ndNHeight != bth.bthDepth) {
//			printf("WARNING: Node %u has wrong depth\n",node);
//		}
		else if (!IsBNodeAlloc(np,sn.node,&bitset)) {
			printf("ERROR: Cannot read B*-tree map!\n");
			return 0;
		}
		else {
			if (!bitset)
				printf("WARNING: B*-tree header denies existience of node %u!\n",node);

			/* okay, pick it apart */
			for (i=0;i < sn.nd.ndNRecs;i++) {
				pp = sn.elem[i].ptr;
				cat.ckrKeyLen = pp[0x00];
				if (cat.ckrKeyLen >= sn.elem[i].size) {
					printf("WARNING: Catalog entry has oversized key\n");
				}
				else if (cat.ckrKeyLen > 0) {
					cat.ckrResrv1 =				pp[0x01];
					cat.ckrParID =		lbei32(	pp+0x02);
					aStr31cpy(&cat.ckrCName,	pp+0x06);

					pp += cat.ckrKeyLen+1;
					if ((pp-sn.buf)&1) pp++;
					cdrType = *pp++;
					cdrResrv = *pp++;

					if (cdrType == 1) {
						HFSCatalogFileDataRec_DirRecord dr;

						dr.dirFlags =		lbei16(	pp+0x00);
						dr.dirVal =			lbei16(	pp+0x02);
						dr.dirDirID =		lbei32(	pp+0x04);

						MakeConvertToDir(dr.dirDirID,cat.ckrParID);
						printf("*Creating Folder %s\n",GetMakeConvertDir());
						if (!MakeDir())
							printf("...Failed!\n");
					}
					else if (cdrType == 2) {
					}
					else if (cdrType == 3) {
						HFSCatalogFileDataRec_DirectoryThread dthr;
						int parent,ID,f;

						dthr.thdResrv[0] =	lbei32(	pp+0x00);
						dthr.thdResrv[1] =	lbei32(	pp+0x04);
						dthr.thdParID =		lbei32(	pp+0x08);
						aStr31cpy(&dthr.thdCName,	pp+0x0C);

						/* double-check */
						f=!LookupDirz((aStr255*)(&dthr.thdCName),&parent,&ID);
						if (!f) f=(parent != dthr.thdParID || ID != cat.ckrParID);
						if (f) {
							PascalToCString(tmp,(PascalString*)(&dthr.thdCName),sizeof(tmp));
							printf("   WARNING: Leaf, PID %5u: %s  (Directory Thread PID %u)\n",cat.ckrParID,tmp,dthr.thdParID);
							printf("            This is inconsistient with directory entry in parent\n");
						}
					}
				}
			}
		}

		/* find the next link in the chain */
		if (node == bth.bthLNode && sn.nd.ndFLink != 0)
			printf("WARNING: B*-tree chain extends past last node\n");

		node = sn.nd.ndFLink;
	}

	/* sift through the leaves again and extract files */
	node = bth.bthFNode;
	nodes = bth.bthNNodes;
	while (nodes-- > 0 && node != 0) {
		if (!ReadCatalogNode(node,&sn)) {
			printf("ERROR: Cannot read B*-tree node %u\n",node);
		}
		else if (sn.nd.ndType != 0xFF) {
			printf("WARNING: Node %u has wrong type\n",node);
		}
//		else if (sn.nd.ndNHeight != bth.bthDepth) {
//			printf("WARNING: Node %u has wrong depth\n",node);
//		}
		else if (!IsBNodeAlloc(np,sn.node,&bitset)) {
			printf("ERROR: Cannot read B*-tree map!\n");
			return 0;
		}
		else {
			if (!bitset)
				printf("WARNING: B*-tree header denies existience of node %u!\n",node);

			/* okay, pick it apart */
			for (i=0;i < sn.nd.ndNRecs;i++) {
				pp = sn.elem[i].ptr;
				cat.ckrKeyLen = pp[0x00];
				if (cat.ckrKeyLen >= sn.elem[i].size) {
					printf("WARNING: Catalog entry has oversized key\n");
				}
				else if (cat.ckrKeyLen > 0) {
					cat.ckrResrv1 =				pp[0x01];
					cat.ckrParID =		lbei32(	pp+0x02);
					aStr31cpy(&cat.ckrCName,	pp+0x06);

					pp += cat.ckrKeyLen+1;
					if ((pp-sn.buf)&1) pp++;
					cdrType = *pp++;
					cdrResrv = *pp++;

					if (cdrType == 1) {
					}
					else if (cdrType == 2) {
						HFSCatalogFileDataRec_FileRecord fr;
						char nname[1024];
						char tmp[256];

						fr.filFlags =							pp[0x00];
						fr.filTyp =								pp[0x01];
						memcpy(fr.filUsrWds.fdType,				pp+0x02,	4);
						memcpy(fr.filUsrWds.fdCreator,			pp+0x06,	4);
						fr.filUsrWds.fdFlags =			lbei16(	pp+0x0A);
						fr.filUsrWds.fdLocation.v =		lbei16(	pp+0x0C);
						fr.filUsrWds.fdLocation.h =		lbei16(	pp+0x0E);
						fr.filUsrWds.fdFldr =			lbei16(	pp+0x10);
						fr.filFlNum =					lbei32(	pp+0x12);
						fr.filStBlk =					lbei16(	pp+0x16);
						fr.filLgLen =					lbei32(	pp+0x18);
						fr.filPyLen =					lbei32(	pp+0x1C);
						fr.filRstBlk =					lbei16(	pp+0x20);
						fr.filRLgLen =					lbei32(	pp+0x22);
						fr.filRPyLen =					lbei32(	pp+0x26);
						fr.filCrDat =					lbei32(	pp+0x2A);
						fr.filMdDat =					lbei32(	pp+0x2E);
						fr.filBkDat =					lbei32(	pp+0x32);
						fr.filFndrInfo.fdIconID =		lbei16(	pp+0x36);
						for (j=0;j < 3;j++)
							fr.filFndrInfo.fdReserved[j] =
													lbei16(	pp+0x38+(j*2));
						fr.filFndrInfo.fdScript =			pp[0x3E];
						fr.filFndrInfo.fdXFlags =			pp[0x3F];
						fr.filFndrInfo.fdComment =	lbei16(	pp+0x40);
						fr.filFndrInfo.fdPutAway =	lbei32(	pp+0x42);
						fr.filClpSize =				lbei16(	pp+0x46);
						for (j=0;j < 3;j++) {
							fr.filExtRec[j].xdrStABN =
										lbei16(	pp+0x48+(j*4));
							fr.filExtRec[j].xdrNumABlks =
										lbei16(	pp+0x48+(j*4)+2);
						}
						for (j=0;j < 3;j++) {
							fr.filRExtRec[j].xdrStABN =
										lbei16(	pp+0x54+(j*4));
							fr.filRExtRec[j].xdrNumABlks =
										lbei16(	pp+0x54+(j*4)+2);
						}
						fr.filResrv = lbei32(	pp+0x60);

						if (export_macbinary) {
							char original_name[128];
							FILE *fp;

							PascalToCString(tmp,(PascalString*)(&cat.ckrCName),256);
							PascalToCString(original_name,(PascalString*)(&cat.ckrCName),256);
							MakeConvertToDir(cat.ckrParID,0);
							MacFnFilter(tmp);
							if (export_macbinary_stream) {
								sprintf(nname,"%s\\CONTENTS.BIN",GetMakeConvertDir());
								printf("*Extracting to %s\n",nname);
								fp = fopen(nname,"rb+");
								if (!fp) fp = fopen(nname,"wb");
								if (fp) fseek(fp,0,SEEK_END);
							}
							else {
								sprintf(nname,"%s\\%s.bin",GetMakeConvertDir(),tmp);
								printf("*Extracting to %s\n",nname);
								fp = fopen(nname,"wb");
								if (fp) fclose(fp);
								fp = fopen(nname,"rb+");
								if (fp) fseek(fp,0,SEEK_SET);
							}
							if (fp) {
								unsigned long o1,o2,o3,dl,rl;

								MacBinaryPad128(fp);
								WriteBlankMacBinary(fp,original_name,&fr);
								o1 = ftell(fp);
								ExtractFileAppend(&fr,fr.filExtRec,fp,0);
								o2 = ftell(fp); dl = o2 - o1;
								MacBinaryPad128(fp);
								o2 = ftell(fp);
								if (fr.filRLgLen > 0) ExtractFileAppend(&fr,fr.filRExtRec,fp,1);
								MacBinaryPad128(fp);
								o3 = ftell(fp); rl = o3 - o2;
								UpdateMacBinary(fp,dl,rl);

								fclose(fp);
							}
							else {
								printf("----FAILED EXTRACTION!\n");
							}
						}
						else {
							PascalToCString(tmp,(PascalString*)(&cat.ckrCName),256);
							MakeConvertToDir(cat.ckrParID,0);
							MacFnFilter(tmp);
							sprintf(nname,"%s\\%s",GetMakeConvertDir(),tmp);
							printf("*Extracting to %s\n",nname);
							if (!ExtractFile(&fr,fr.filExtRec,nname,0))
								printf("...ERROR\n");

							if (fr.filRLgLen > 0) {
								PascalToCString(tmp,(PascalString*)(&cat.ckrCName),256);
								MakeConvertToDir(cat.ckrParID,0);
								MacFnFilter(tmp);
								sprintf(nname,"%s\\.resfork--%s",GetMakeConvertDir(),tmp);
								printf("*Extracting to %s\n",nname);
								if (!ExtractFile(&fr,fr.filRExtRec,nname,1))
									printf("...ERROR\n");
							}
						}
					}
					else if (cdrType == 3) {
					}
				}
			}
		}

		/* find the next link in the chain */
		if (node == bth.bthLNode && sn.nd.ndFLink != 0)
			printf("WARNING: B*-tree chain extends past last node\n");

		node = sn.nd.ndFLink;
	}

	return 1;
}

int HFSDumper::EnumRootDir()
{
	int i;
	HFSNodePack np;

	/* CHECK */
//	sum =	MasterDirB.drCTExtRec[0].xdrNumABlks + MasterDirB.drCTExtRec[1].xdrNumABlks + 
//			MasterDirB.drCTExtRec[2].xdrNumABlks;

//	if ((sum*MasterDirB.drAlBlkSiz) != MasterDirB.drCTFlSize)
//		printf("WARNING: Catalog file size != sum of allocation extents\n");

	i=0;
	while (i <= 2 && MasterDirB.drCTExtRec[i].xdrNumABlks == 0) i++;
	if (i > 2) {
		printf("ERROR: NULL-length catalog file!\n");
		return 0;
	}

	/* get the first header node */
	if (!ReadCatalogNode(0,&np)) {
		printf("ERROR: Cannot read catalog file from disk!\n");
		return 0;
	}

	/* looking for: Header node for the entire tree, start of chain, level 0 */
	if (np.nd.ndType != 0x01 || np.nd.ndNHeight != 0x00 || np.nd.ndBLink != 0) {
		printf("ERROR: First node in catalog file not B*-tree header!\n");
		return 0;
	}

	/* go! */
	if (!EnumDir(&np)) {
		printf("ERROR in the root B*-tree!\n");
		return 0;
	}

	return 1;
}

static int AppleMapHFSOffset=0;
static int AppleMapPartitionOffset=0;
static int AppleMapPartitionSize=0;
static int AppleMapHFSSize=0;
int AppleMap(BlockDeviceFile *b)
{
	unsigned char buf[512];
	int i;

	AppleMapHFSOffset = 0;
	if (b->ReadDisk(0,buf,1) < 1)
		return 0;

	if (memcmp(buf,"ER",2))
		return 0;

	if (b->ReadDisk(1,buf,1) < 1)
		return 0;

	if (memcmp(buf,"PM",2))
		return 0;

	if (strcmp((char*)(buf+48),"Apple_partition_map"))
		return 0;

	AppleMapPartitionOffset = lbei32(buf +  8);
	AppleMapPartitionSize   = lbei32(buf + 12);

	for (i=0;i < AppleMapPartitionSize;i++) {
		if (b->ReadDisk(AppleMapPartitionOffset + i,buf,1) < 1)
			return 0;
		if (memcmp(buf,"PM",2))
			return 0;
		if (strcmp((char*)(buf+48),"Apple_HFS"))
			continue;

		AppleMapHFSOffset = lbei32(buf +  8);
		AppleMapHFSSize   = lbei32(buf + 12);
		return 1;
	}

	return 0;
}

/* MAIN APP */
int main(int argc,char **argv)
{
	SomeFileStdio file;
	BlockDeviceDiskCopyImage blk_dimg;
	BlockDeviceFile blk_f;
	HFSDumper hfs;

	if (argc < 2) {
		printf("HFSDUMP <image of Macintosh floppy with HFS filesystem>\n");
		return 1;
	}

	if (file.Open(argv[1]) < 0) {
		printf("Unable to open disk image!\n");
		return 1;
	}

	if (blk_f.Assign(&file) < 0) {
		printf("Unable to assign file!\n");
		return 1;
	}

	if (hfs.Assign(&blk_f) < 0) {
		printf("Unable to block file device to HFS dumper!\n");
		return 1;
	}

	/* check for an Apple Partition map. if one is present, locate the first Apple_HFS partition */
	if (AppleMap(&blk_f))
		blk_f.SetBase(AppleMapHFSOffset * 512);

	if (!hfs.CheckMDB()) {
		/* probably a DiskCopy image? */
		if (hfs.Assign(&blk_dimg) < 0 ||
			blk_dimg.Assign(&file) < 0 ||
			!hfs.CheckMDB()) {
			printf("ERROR: Invalid Macintosh File System MDB!\n");
			return 1;
		}
	}

	if (!hfs.HFSAllocOneBlock()) {
		printf("ERROR: Could not allocate HFS block buffer\n");
		return 1;
	}

	if (!hfs.EnumRootDir()) {
		printf("ERROR: Invalid data in the root directory!\n");
		return 1;
	}

	return 0;
}
