/* MFS Macintosh File System dumping utility
   (C) 2004, 2005 Jonathan Campbell <i_like_pie@nerdgrounds.com>

   Examines 400K Macintosh floppy images with MFS filesystem structures
   on them and extracts the files (both data and resource forks). Should
   be very useful for Macintosh enthusiasts with old floppies since MFS
   has been considered obsolete for quite some time now, with support
   dropped since some version of System 7.

   This is probably the best source you'll ever find on MFS. Information
   on MFS is apparently scarce on the internet, leaving me no other option
   than to closely examine every byte in some disk images I found of Apple
   MacOS Systems 1.x, 2.x, and 3.x (non-HFS ones). Fortunately it seems many
   structures in HFS were borrowed from MFS, and most of them are same or
   similar (HFS having more fields tacked on to the end).

   MFS, unlike HFS, uses an allocation table in the same way that MS-DOS
   uses an allocation table. A file is given a starting "allocation block".
   An "Allocation Block Map" is used to indicate which blocks are allocated
   and where the next block is (or if it ends). Each entry is 12 bits long,
   with two 12-bit values per 3 bytes.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include "Apple/common/Apple-Macintosh-Common.h"
#include "common/SomeFileStdio.h"
#include "common/BlockDeviceFile.h"
#include "Apple/Image-DiskCopy/BlockDeviceDiskCopyImage.h"

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

/*********************************************************************
 * MFS specific defines and structures. They are very similar to HFS *
 * but are not exactly the same.                                     *
 *********************************************************************/

/* MFS Master Directory Block. It is the same as the HFS MDB except shorter.
   Most of the values don't seem to make sense beyond the volume name field
   based on what HFS defines here. */
typedef struct {
	aInteger				drSigWord;		// 0xD2D7 for MFS
	aLongInt				drCrDate;		// *Volume creation date
	aLongInt				drLsMod;		// *Volume last modified date
	aInteger				drAtrb;			// *Volume attributes
	aInteger				drNmFls;		// *Number of files in root directory
	aInteger				drVBMSt;		// [1]First block of root directory
	aInteger				drAllocPtr;		// *Start of next allocation search
	aInteger				drNmAlBlks;		// *Number of allocation blocks in volume
	aLongInt				drAlBlkSiz;		// *Allocation block size (bytes)
	aLongInt				drClpSiz;		// *Default clump size
	aInteger				drAlBlSt;		// *First allocation block in volume
	aLongInt				drNxtCNID;		// *Next unused catalog node ID
	aInteger				drFreeBks;		// *Number of unused allocation blocks
	aStr27					drVN;			// *Volume name
} MFSMDB;
// [*] Assumed field name and purpose based on HFS version
//
// [1] This is a guess: in the disk images I have this always corresponds
//     to the root directory location.

typedef struct {
   aOSType		fdType;					// TYPE
   aOSType		fdCreator;				// CREATOR
   aUInt16		fdFlags;				// Finder flags
   aPoint		fdLocation;				// location of icon within the window of the Finder
   aSInt16		fdFldr;					// window in which the file's icon appears in the Finder
} MFSFinderInfo;

typedef struct {
	aSignedByte			filFlags;	// bit 7=used, bit 0=locked, cannot be written
	aSignedByte			filTyp;		// always zero
	MFSFinderInfo		filUsrWds;	// Finder info
	aLongInt			filFlNum;	// File ID
	aInteger			filStBlk;	// first allocation block of data fork
	aLongInt			filLgLen;	// logical EOF of data fork
	aLongInt			filPyLen;	// physical EOF of data fork
	aInteger			filRstBlk;	// first allocation block of resource fork
	aLongInt			filRLgLen;	// logical EOF of resource fork
	aLongInt			filRPyLen;	// physical EOF of resource fork
	aLongInt			filCrDat;	// date/time of creation
	aLongInt			filMdDat;	// date/time of last modification
} MFSCatalogFileRecord;

/*********************************************************************
 * MFS dumping class. Reads an MFS filesystem and dumps the contents *
 * (both data and resource) to your hard drive as separate files.    *
 *********************************************************************/
class MFS_Dumper {
public:
	MFS_Dumper() {
		MFS_ABM=NULL;
		MFS_ABMSz=0;
		MFS_ABMBm=0;
		MFS_OneBlock=NULL;
		MFS_OneBlockSz=0;
	};
	~MFS_Dumper() {
		FreeABM();
		MFSFreeOneBlock();
	};
public:
	MFSMDB				MasterDirB;
/* a copy of the Allocation Bit Map stored in memory */
	unsigned char*		MFS_ABM;
	int					MFS_ABMSz;
	int					MFS_ABMBm;
/* dedicated chunk of memory for handling allocation-block-sized chunks */
	unsigned char*		MFS_OneBlock;
	int					MFS_OneBlockSz;
/* the source block device */
	BlockDevice*		bdev;
public:
	void				SetBlkSource(BlockDevice* bdev);
	unsigned int		ReadABM_Map(int block);
	int					CheckMDB();
	int					LoadABM();
	int					FreeABM();
	int					MFSAllocOneBlock();
	int					MFSFreeOneBlock();
	int					EnumRootDir();
};

/* read the ABM in memory, does not read from block device */
unsigned int MFS_Dumper::ReadABM_Map(int block)
{
	int o=((block-2)>>1)*3;
	int r,v;

	if (block < 2) return 1;
	if (o >= MFS_ABMSz) return 1;

	// One entry in the ABM table is 12 bits long.
	// two 12-bit values are encoded into 3 bytes like this:
	//
	// BYTE |0       |1       |2       |
	// BIT  |76543210|76543210|76543210|
	// -----+--------+--------+--------+----------------
	//      |nnnnnnnn|nnnnmmmm|mmmmmmmm|
	// -----+--------+--------+--------+----------------
	// n = bits 0-11 with the number of the next block
	// m = bits 0-11 with the number of the next block
	//
	// 0 means not allocated
	// 1 means the end of the chain
	// >= 2 means the next block number
	v  = ((int)MFS_ABM[o  ]) << 16;
	v |= ((int)MFS_ABM[o+1]) << 8;
	v |= ((int)MFS_ABM[o+2]);
	r  = (v >> (((block&1)^1)*12))&0xFFF;
	return r;
}

int MFS_Dumper::CheckMDB()
{
	char tmp[257];
	unsigned char buf[512];
	int expectsz,sects;

	// read the MDB
	if (!bdev->ReadDisk(2,buf,1))
		return 0;

	// MFS only, please
	if ((MasterDirB.drSigWord = lbei16(buf)) != 0xD2D7)
		return 0;

	MasterDirB.drCrDate =			lbei32(	buf+0x02);
	MasterDirB.drLsMod =			lbei32(	buf+0x06);
	MasterDirB.drAtrb =				lbei16(	buf+0x0A);
	MasterDirB.drNmFls =			lbei16(	buf+0x0C);
	MasterDirB.drVBMSt =			lbei16(	buf+0x0E);
	MasterDirB.drAllocPtr =			lbei16(	buf+0x10);
	MasterDirB.drNmAlBlks =			lbei16(	buf+0x12);
	MasterDirB.drAlBlkSiz =			lbei32(	buf+0x14);
	MasterDirB.drClpSiz =			lbei32(	buf+0x18);
	MasterDirB.drAlBlSt =			lbei16(	buf+0x1C);
	MasterDirB.drNxtCNID =			lbei32(	buf+0x1E);
	MasterDirB.drFreeBks =			lbei16(	buf+0x22);
	aStr27cpy(&MasterDirB.drVN,		buf+0x24);

	if (MasterDirB.drVBMSt > 10) {
		printf("WARNING: drVBMSt > 10. This code not built to handle that!\n");
		return 0;
	}

	/* This code is optimized around 512-byte sectors. If someone knows
	   of a Macintosh that used other-sized sectors let me know! */
	if ((MasterDirB.drAlBlkSiz&511) != 0) {
		printf("ERROR: Allocation block size not a multiple of 512!\n");
		return 0;
	}

	/* pick out the Allocation Block Map */
	expectsz =	(MasterDirB.drNmAlBlks * 12) >> 3;
	sects =		MasterDirB.drVBMSt - 2;
	MFS_ABMSz =	(sects * 512) - 0x40;
	MFS_ABMBm =	(MFS_ABMSz / 3) << 1;
	if (sects < 1) {
		printf("ERROR: Invalid \"Start of Root Directory\" value in MDB!\n");
		return 0;
	}
	else if (MFS_ABMSz < expectsz) {
		printf("ERROR: The ABM on this disk is too small!\n");
		return 0;
	}

	/* Show the user we found the volume by printing the volume name */
	PascalToCString(tmp,(PascalString*)(&MasterDirB.drVN),sizeof(tmp));
	printf("...Volume name: %s\n",tmp);

	return 1;
}

int MFS_Dumper::LoadABM()
{
	unsigned char tmp[512];
	int s,sm;

	if (MFS_ABM) free(MFS_ABM);
	MFS_ABM = (unsigned char*)malloc(MFS_ABMSz);
	if (!MFS_ABM) return 0;

	// read the ABM from disk
	if (!bdev->ReadDisk(2,tmp,1)) return 0;
	memcpy(MFS_ABM,tmp+0x40,512-0x40);
	sm=(MFS_ABMSz+511)>>9;
	for (s=1;s < sm;s++)
		if (!bdev->ReadDisk(2+s,(MFS_ABM+(s*512))-0x40,1))
			return 0;

	return 1;
}

int MFS_Dumper::FreeABM()
{
	if (!MFS_ABM) return 0;
	free(MFS_ABM);
	MFS_ABM=NULL;
	MFS_ABMSz=0;
	MFS_ABMBm=0;
	return 1;
}

int MFS_Dumper::MFSAllocOneBlock()
{
	if (MFS_OneBlock) return 1;
	MFS_OneBlockSz = MasterDirB.drAlBlkSiz;
	MFS_OneBlock = (unsigned char*)malloc(MFS_OneBlockSz);
	if (!MFS_OneBlock) return 0;
	return 1;
}

int MFS_Dumper::MFSFreeOneBlock()
{
	if (!MFS_OneBlock) return 1;
	free(MFS_OneBlock);
	MFS_OneBlock=NULL;
	MFS_OneBlockSz=0;
	return 1;
}

static void WriteBlankMacBinary(FILE *fp,char *name,MFSCatalogFileRecord *fr)
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

int MFS_Dumper::EnumRootDir()
{
	unsigned long o1,o2,o3,dl,rl;
	char original_name[512];
	unsigned char buf[512];
	unsigned char *rp,*fence;
	MFSCatalogFileRecord acr;
	aStr255 FileName;
	char tmp2[290];
	char tmp[257];
	int filez=0;
	int sect=4;
	FILE *dst;

	mkdir("__RIPPED__");

	rp = buf;
	fence = buf+512;
	if (!bdev->ReadDisk(sect++,buf,1)) return 0;
	while (filez < MasterDirB.drNmFls && rp < fence && sect < MasterDirB.drAlBlSt) {
		if ((rp+50) > fence || *rp == 0) {
			rp = buf;
			fence = buf+512;
			if (!bdev->ReadDisk(sect++,buf,1)) return 0;
		}
		/* file entry */
		acr.filFlags =							rp[0x00];
		acr.filTyp =							rp[0x01];
		if (acr.filTyp != 0)					return 0;
		if (!(acr.filFlags & 0x80))				return 0;
		memcpy(&acr.filUsrWds.fdType,			rp+0x02,4);
		memcpy(&acr.filUsrWds.fdCreator,		rp+0x06,4);
		acr.filUsrWds.fdFlags =			lbei16(	rp+0x0A);
		acr.filUsrWds.fdLocation.v =	lbei16(	rp+0x0C);
		acr.filUsrWds.fdLocation.h =	lbei16(	rp+0x0E);
		acr.filUsrWds.fdFldr =			lbei16(	rp+0x10);
		acr.filFlNum =					lbei32(	rp+0x12);
		acr.filStBlk =					lbei16(	rp+0x16);
		acr.filLgLen =					lbei32(	rp+0x18);
		acr.filPyLen =					lbei32(	rp+0x1C);
		acr.filRstBlk =					lbei16(	rp+0x20);
		acr.filRLgLen =					lbei32(	rp+0x22);
		acr.filRPyLen =					lbei32(	rp+0x26);
		acr.filCrDat =					lbei32(	rp+0x2A);
		acr.filMdDat =					lbei32(	rp+0x2E);
		rp += 50;
		/* copy down the filename */
		if ((rp + 1 + *rp) > fence)			return 0;
		aStr255cpy(&FileName,rp);
		rp += 1 + FileName.len;

		PascalToCString(tmp,&FileName,sizeof(tmp));
		printf("...File:                  %s\n",tmp);
		printf("......ID:                 %u\n",acr.filFlNum);
		printf("......Size (data, log):   %u\n",acr.filLgLen);
		printf("......Size (data, phys):  %u\n",acr.filPyLen);
		printf("......Size (res, log):    %u\n",acr.filRLgLen);
		printf("......Size (res, phys):   %u\n",acr.filRPyLen);
		printf("......First Block (data): %u\n",acr.filStBlk);
		printf("......First Block (res):  %u\n",acr.filRstBlk);

		/* dump out the data fork + resource fork as a MacBinary file */
		int M=(MasterDirB.drAlBlkSiz>>9);
		strcpy(original_name,tmp);
		MacFnFilter(tmp);
		sprintf(tmp2,"__RIPPED__\\%s.bin",tmp);
		dst=fopen(tmp2,"wb");
		if (dst) fclose(dst);
		dst=fopen(tmp2,"rb+");
		if (dst) {
			fseek(dst,0,SEEK_SET);
			MacBinaryPad128(dst);
			WriteBlankMacBinary(dst,original_name,&acr);
			o1 = ftell(dst);
			if (acr.filLgLen > 0) {
				unsigned int block;
				unsigned long bytecount,bleft,L;
				int rde;

				bytecount = 0;
				block = acr.filStBlk;
				bleft = acr.filLgLen;
				rde = 1;
				while (	block >= 2 && bytecount < acr.filLgLen && bleft > 0 &&
						(rde=(bdev->ReadDisk(16+((block-2)*M),MFS_OneBlock,M) >= M))) {

					if (bleft >= MasterDirB.drAlBlkSiz)
						L=MasterDirB.drAlBlkSiz;
					else
						L=bleft;

					fwrite(MFS_OneBlock,L,1,dst);

					/* next block in chain... */
					block = ReadABM_Map(block);
					bytecount += L;
					bleft -= L;
				}

				if (!rde)
					printf("WARNING: File allocation chain runs outside disk boundary\n");
				else if (bytecount < acr.filLgLen)
					printf("WARNING: File allocation chain cut short (file size incorrect)\n");
				else if (block >= 2)
					printf("WARNING: File allocation chain run amok (file size incorrect)\n");
			}

			o2 = ftell(dst); dl = o2 - o1;
			MacBinaryPad128(dst);
			o2 = ftell(dst);

			if (acr.filRLgLen > 0) {
				unsigned int block;
				unsigned long bytecount,bleft,L;
				int rde;

				bytecount = 0;
				block = acr.filRstBlk;
				bleft = acr.filRLgLen;
				rde = 1;
				while (	block >= 2 && bytecount < acr.filRLgLen && bleft > 0 &&
						(rde=(bdev->ReadDisk(16+((block-2)*M),MFS_OneBlock,M) >= M))) {

					if (bleft >= MasterDirB.drAlBlkSiz)
						L=MasterDirB.drAlBlkSiz;
					else	
						L=bleft;

					fwrite(MFS_OneBlock,L,1,dst);

					/* next block in chain... */
					block = ReadABM_Map(block);
					bytecount += L;
					bleft -= L;
				}

				if (!rde)
					printf("WARNING: File allocation chain runs outside disk boundary\n");
				if (bytecount < acr.filRLgLen)
					printf("WARNING: File allocation chain cut short (file size incorrect)\n");
				else if (block >= 2)
					printf("WARNING: File allocation chain run amok (file size incorrect)\n");
			}

			o3 = ftell(dst); rl = o3 - o2;
			MacBinaryPad128(dst);
			o3 = ftell(dst);
			UpdateMacBinary(dst,dl,rl);
			fclose(dst);
		}

		/* It seems on System 3.x and System 2.04 floppies the root directory
		   entries are padded with zeros to align them on WORD boundaries. */
		if (*rp == 0) rp++;
		filez++;
	}

	return 1;
}

void MFS_Dumper::SetBlkSource(BlockDevice* dev)
{
	if (!dev) return;
	if (dev->GetBlockSize() != 512) return;
	bdev = dev;
}

/*********************************************************************
 * main application entry point. associates a block device with an   *
 * image file of your choice and uses the MFS dumper on it.          *
 *********************************************************************/

int main(int argc,char **argv)
{
	SomeFileStdio filesrc;				/* file source */
	BlockDeviceDiskCopyImage blk_dimg;	/* block device using DiskCopy file source */
	BlockDeviceFile blk_dev;			/* block device using file source */
	MFS_Dumper mfs;						/* MFS dumping code */

	if (argc < 2) {
		printf("MFSDUMP <image of Macintosh floppy with MFS filesystem>\n");
		return 1;
	}

	if (filesrc.Open(argv[1]) < 0) {
		printf("Unable to open disk image!\n");
		return 1;
	}
	if (blk_dev.Assign(&filesrc) < 0) {
		printf("Cannot assign file source!\n");
		return 1;
	}
	if (blk_dev.SetBlockSize(512) < 0) {
		printf("Cannot set block size to 512\n");
		return 1;
	}

	mfs.SetBlkSource(&blk_dev);
	if (!mfs.CheckMDB()) {
		mfs.SetBlkSource(&blk_dimg);
		/* probably a DiskCopy image */
		if (blk_dimg.Assign(&filesrc) < 0 ||
			blk_dimg.SetBlockSize(512) < 0 ||
			!mfs.CheckMDB()) {
			printf("ERROR: Invalid Macintosh File System MDB!\n");
			return 1;
		}
	}
	if (!mfs.LoadABM()) {
		printf("ERROR: Cannot load Allocation Block Map!\n");
		return 1;
	}

	if (!mfs.MFSAllocOneBlock()) {
		printf("ERROR: Could not allocate MFS block buffer\n");
		return 1;
	}

	if (!mfs.EnumRootDir()) {
		printf("ERROR: Invalid data in the root directory!\n");
		return 1;
	}

	return 0;
}
