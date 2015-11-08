
/* a C++ class for reading the FAT filesystem
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 *
 * supported filesystems:
 *
 *   FAT12
 *     native to:      Microsoft MS-DOS and IBM PC-DOS up to 3.x
 *
 *   FAT16
 *     native to:      Microsoft MS-DOS and IBM PC-DOS 4.x through 6.x
 *                     Microsoft Windows 95
 *
 *   FAT32
 *     native to:      Microsoft Windows 95 OEM release 2, 98,
 *                     98 Second Edition, Millenium Edition
 *
 * extra features supported:
 *
 *   Microsoft Windows 95 "Long File Names"
 *
 */

#include "cdosfat.h"
#include <string.h>
#include <stdio.h>

static int IsPow2(unsigned int x)
{
	if (x == 0) return 0;
	while (!(x&1)) x >>= 1;
	if (x == 1) return 1;
	return 0;
}

static void choppadding(char *x)
{
	int l=strlen(x)-1;

	while (l >= 0 && x[l] == 32)
		x[l--] = 0;
}

static unsigned char lfnhash(unsigned char name[11])
{
	unsigned char sum=0;
	int i;

	for (i=0;i < 11;i++) {
		sum  = (sum >> 1) | ((sum & 1) << 7);
		sum += name[i];
	}

	return sum;
}

// BEGIN CDOSFat namespace
namespace CDOSFat {
////////////////////////////////////////////////

int reader::init()
{
	mounted=0;
	return 1;
}

int reader::free()
{
	if (mounted) umount();
	return 1;
}

int reader::GetBPB(unsigned char *tmp,int sz)
{
	/* try to determine the overall size of the structure from the JMP opcode */
	if (tmp[0] == 0xEB && tmp[2] == 0x90)	BPBSize = tmp[1]+2;
	else if (tmp[0] == 0xE9)				BPBSize = ((tmp[2]<<8)|tmp[1])+2;
	else									BPBSize = 0;

	if (BPBSize == 0) {
		comment(C_ERROR,"No JMP opcode detected, not a DOS boot sector");
		return 0;
	}
	else if (BPBSize < 33) {
		comment(C_ERROR,"JMP opcode offset too small, no BPB detected (%u < 33)",BPBSize);
		return 0;
	}

	if (!(tmp[510] == 0x55 && tmp[511] == 0xAA))
		return 0;

	FAT =						0;
	BytesPerSector =			llei16(tmp +  11);
	SectorsPerCluster =			tmp[13];
	ReservedSectors =			llei16(tmp +  14);
	FATCopies =					tmp[16];
	RootDirEntries =			llei16(tmp +  17);
	TotalSectors =				llei16(tmp +  19);
	MediaDescriptor	=			tmp[21];
	SectorsPerFAT =				llei16(tmp +  22);
	SectorsPerTrack =			llei16(tmp +  24);
	Heads =						llei16(tmp +  26);

	if (BPBSize > 31)			HiddenSectors =	llei32(tmp +  28);
	else						HiddenSectors =	llei16(tmp +  28);

	/* now grab other members of the structure, depending on whether it's FAT12/FAT16 or FAT32 */
	if (BPBSize > 66 && SectorsPerFAT == 0) {
		/* FAT32 BPB */
		SectorsPerFAT =								llei32 (tmp +  36);
		MirrorFlags =								llei16 (tmp +  40);
		SingleActiveFAT32 =							MirrorFlags & 0x80;
		ActiveFAT32 =								SingleActiveFAT32 ? (MirrorFlags & 0xF) : 0;
		FS32Version =								llei16 (tmp +  42);
		RootDirCluster =							llei32 (tmp +  44);
		FSInfoSector32 =							llei16 (tmp +  48);
		BackupBootSector32 =						llei16 (tmp +  50);
		INT13DriveNo =										tmp[64];
		WinNTCheckDiskFlags =								tmp[65];
		ExtendedSignature =									tmp[66];
		if (BPBSize > 70)	SerialNumber =			llei32 (tmp +  67);
		else				SerialNumber =					0;
		if (BPBSize > 81)	memcpy(VolumeLabel,				tmp +  71,	11);
		else				memset(VolumeLabel,				0,			11);
		if (BPBSize > 89)	memcpy(FileSystemType,			tmp +  82,	8);
		else				memset(FileSystemType,			0,			8);
		FAT =				32;
	}
	else {
		/* FAT12/FAT16 BPB */
		if (BPBSize > 36)	INT13DriveNo =			tmp[36];
		else				INT13DriveNo =			0x00;
		if (BPBSize > 37)	WinNTCheckDiskFlags =	tmp[37];
		else				WinNTCheckDiskFlags =	0;
		if (BPBSize > 38)	ExtendedSignature =		tmp[38];
		else				ExtendedSignature =		0;
		if (BPBSize > 42)	SerialNumber =			llei32(tmp +  39);
		else				SerialNumber =			0;
		if (BPBSize > 53)	memcpy(VolumeLabel,		tmp + 43,	11);
		else				memset(VolumeLabel,		0,		11);
		if (BPBSize > 61)	memcpy(FileSystemType,	tmp + 54,	8);
		else				memset(FileSystemType,	0,		8);
	}

	// beware: this might be an NTFS partition!
	if (tmp[16] == 0 && llei16(tmp +  17) == 0 && llei16(tmp + 22) == 0) {
		if (!memcmp(tmp + 3,"NTFS",4)) {
			comment(C_ERROR,"Wrong filesystem. Use NTFS filesystem reader.");
			return 0;
		}
	}

	memcpy(OEMSignature,tmp+3,8); OEMSignature[8]=0;
	comment(C_INFO,"OEM name/version: %s",OEMSignature);

	if (!IsPow2(BytesPerSector)) {
		comment(C_ERROR,"Bytes/sector field %u is not a power of 2",BytesPerSector);
		return 0;
	}
	else if (BytesPerSector < 512) {
		comment(C_ERROR,"Bytes/sector field %u too small",BytesPerSector);
		return 0;
	}
	else if (BytesPerSector != sectsize) {
		comment(C_ERROR,"Bytes/sector field != actual sectorsize (%u != %u)",BytesPerSector,sectsize);
		return 0;
	}
	else if (FATCopies == 0) {
		comment(C_ERROR,"# of FAT tables field == 0");
		return 0;
	}
	else if (SectorsPerFAT == 0) {
		comment(C_ERROR,"sectors/FAT field == 0");
		return 0;
	}
	else if (!IsPow2(SectorsPerCluster)) {
		comment(C_ERROR,"Sectors/cluster field %u is not a power of 2",SectorsPerCluster);
		return 0;
	}
	else if (ReservedSectors == 0) {
		comment(C_ERROR,"# of reserved sectors == 0, which is invalid");
		return 0;
	}
	else if (RootDirEntries == 0 && FAT != 32) {
		comment(C_ERROR,"# of root directory entries == 0, which is invalid");
		return 0;
	}

	if (MediaDescriptor < 0xF0 || (MediaDescriptor >= 0xF1 && MediaDescriptor < 0xF8))
		comment(C_WARNING,"Odd media descriptor %0x2X",MediaDescriptor);

	if (TotalSectors == 0) {
		if (!(MediaDescriptor == 0xF8 || MediaDescriptor == 0xFA)) {
			comment(C_WARNING,
				"TotalSectors == 0 and media descriptor %2X suggests that disk size should "
				"be small enough to assume FAT12. Personal experience says that DOS/Windows "
				"makes the same assumption and that if a floppy were somehow formatted "
				"as FAT16 or FAT32 the system would fail to read it.",MediaDescriptor);
		}

		if (BPBSize >= 36) {
			/* look for it then in the FAT16 extensions */
			TotalSectors = llei32(tmp +  32);
		}
		else {
			comment(C_ERROR,"TotalSectors field == 0 and BPB not big enough to hold the extended value!");
			return 0;
		}
	}

	if (TotalSectors > disksize())
		comment(C_WARNING,"TotalSectors is too large for this disk (%u > %u)",TotalSectors,disksize());

	/* the FAT32 filesystem has the root directory as part of the data area */
	if (FAT == 32)	RootDirSectors = 0;
	else			RootDirSectors = (RootDirEntries + 15) >> 4;
	DataAreaOffset  = ReservedSectors;
	DataAreaOffset += SectorsPerFAT * FATCopies;
	DataAreaOffset += RootDirSectors;
	TotalClusters   = TotalSectors - DataAreaOffset;
	TotalClusters  -= TotalClusters % SectorsPerCluster;
	TotalClusters  /= SectorsPerCluster;
	if (TotalClusters <= 0) {
		comment(C_ERROR,"TotalSectors is too small, with %u the number of clusters for the data area is %d",TotalSectors,TotalClusters);
		return 0;
	}

	if (FAT == 32) {
		if (TotalClusters < 65525)	comment(C_WARNING,"Inconsistient filesystem. BPB suggests FAT32 but total cluster count %u < 65525 which would suggest FAT16!",TotalClusters);
	}
	else {
		if (TotalClusters > 4084)	FAT = 16;
		else				FAT = 12;
	}

	if (FAT == 32)		FATMASK = 0xFFFFFFF;
	else				FATMASK = (-1) & ((1 << FAT)-1);
	if (ReadFAT(0) != (MediaDescriptor | (FATMASK & (~0xFF))))
		comment(C_WARNING,"Media descriptor in boot sector doesn't match media descriptor in FAT");
	if (ReadFAT(1) != FATMASK)
		comment(C_WARNING,"FAT entry #1 should be 0x%X",FATMASK);

	return 1;
}

int reader::mount()
{
	if (mounted) {
		comment(C_ERROR,"Already mounted!");
		return 0;
	}

	sectsize=blocksize();
	if (sectsize != 512) {
		comment(C_ERROR,"Block sizes != 512 not supported yet!");
		return 0;
	}

	tmpsect=new unsigned char[sectsize];
	if (!tmpsect) {
		comment(C_ERROR,"Unable to allocate %u-byte buffer",sectsize);
		return 0;
	}

	mounted=1;
	if (!blockread(0,1,tmpsect)) {
		comment(C_ERROR,"Unable to read first sector!");
		umount();
		return 0;
	}

	if (!GetBPB(tmpsect,sectsize)) {
		comment(C_ERROR,"Not a DOS FAT file system!");
		umount();
		return 0;
	}

	return 1;
}

int reader::umount()
{
	if (!mounted) {
		comment(C_ERROR,"Already not mounted!");
		return 0;
	}

	delete tmpsect;
	tmpsect=NULL;
	mounted=0;
	return 1;
}

// FIXME: This is hard-coded to assume 512 bytes/sector!
unsigned long reader::ReadFAT(unsigned long N)
{
	unsigned long R=0;
	unsigned long so,sbo;

	if (N >= TotalClusters) return 0;

	if (FAT == 32) {
		so  = N>>7;			// so  = N/128
		sbo = (N&127)<<2;	// sbo = (N%128) * sizeof(DWORD)
		if (so > SectorsPerFAT) return 0xFFF;
		so += ReservedSectors;

		if (SingleActiveFAT32) {
			if (ActiveFAT32 >= FATCopies) return 0;
			so += ActiveFAT32 * SectorsPerFAT;
		}

		if (!blockread(so,1,tmpsect)) {
			comment(C_WARNING,"Cannot read sector %u!",so);
			return 0;
		}

		R = llei32(tmpsect + sbo)&0xFFFFFFF;
	}
	else if (FAT == 16) {
		so  = N>>8;			// so  = N/256
		sbo = (N&255)<<1;	// sbo = (N%256) * sizeof(WORD)
		if (so > SectorsPerFAT) return 0xFFF;
		so += ReservedSectors;
		if (!blockread(so,1,tmpsect)) {
			comment(C_WARNING,"Cannot read sector %u!",so);
			return 0;
		}

		R = llei16(tmpsect + sbo);
	}
	else if (FAT == 12) {
		sbo  = (N>>1)*3;	// sbo  = (N/2) * sizeof((2*12) bits)
		so   = sbo>>9;		// so   = sbo/512
		sbo &= 511;			// sbo %= 512;
		if (so > SectorsPerFAT) return 0xFFF;
		so  += ReservedSectors;

		if (!blockread(so,1,tmpsect)) {
			comment(C_WARNING,"Cannot read sector %u!",so);
			return 0;
		}

		R = 0;
		for (int i=0;i < 3;i++) {
			R |= ((unsigned long)tmpsect[sbo++]) << (i*8);
			if (sbo >= 512) {
				sbo -= 512;
				so++;
				if ((so+ReservedSectors) > SectorsPerFAT) return 0xFFF;
				if (!blockread(so,1,tmpsect)) {
					comment(C_WARNING,"Cannot read sector %u!",so);
					return 0;
				}
			}
		}

		if (N&1) R >>= 12;
		R &= 0xFFF;
	}

	return R;
}

reader::entity::entity(reader *parent)
{
	mom=parent;
}

reader::entity::~entity()
{
}

unsigned long reader::entity::tell()
{
	return pointer;
}

unsigned long reader::entity::seek(unsigned long pos)
{
	unsigned long clof,t1;

	if (pos > file_size)		pointer = pos = file_size;
	else						pointer = pos;

	/* file/directory object in data area mode, where the
	   chain is dictated by the File Allocation Table. */
	if (linear_offset == 0) {
		/* if there is no starting cluster, then there is nowhere to go */
		if (cluster_start == 0)	pos = 0;
		clof = pointer - (pointer % cluster_size);

		/* do we need to search backwards in the FAT chain? */
		if (clof < pointer_cr) {
			pointer_cr = 0;
			cluster = cluster_start;
		}

		/* search forward in the FAT chain for where we should go */
		while (pointer_cr < clof && cluster >= 2 && cluster < (mom->FATMASK-0xF)) {
			cluster = mom->ReadFAT(cluster);
			pointer_cr += cluster_size;
		}

		/* are we there? */
		if (pointer_cr != clof) {
			pointer = pointer_cr;
			/* why? */
			if (cluster >= (mom->FATMASK-0xF)) {
				/* not an error if a directory */
				if (!(my_attr & 0x10)) {
					mom->comment(C_ERROR,"Truncated FAT chain! Cannot seek to offset %u as promised",pos);
				}
			}
			else {
				mom->comment(C_ERROR,"Unknown error! Cannot seek to offset %u as promised",pos);
			}
		}
		else {
			/* good! */
		}

		/* CHECK! */
		t1=pointer - pointer_cr;
		if (t1 >= cluster_size)	mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster > cluster size!");
		else if (t1 < 0)		mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster < 0!");
	}
	/* linear block of data on disk mode (e.g. FAT12/16 root directory) */
	else {
		/* this always succeeds */
	}

	return pointer;
}

int reader::entity::read(unsigned char *buf,int sz)
{
	unsigned long sct;
	unsigned long t1;
	int rd=0,mrd,oo,blk,crd,dsksize;

	/* special processing for linear offset mode */
	if (linear_offset > 0) {
		if (pointer >= file_size) return 0;
		blk = mom->blocksize();
		sct = pointer - (pointer % blk);
		sct = linear_offset + (pointer / blk);

		while (sz > 0) {
			/* read */
			if (!mom->blockread(sct,1,mom->tmpsect)) {
				mom->comment(C_ERROR,"Cannot read sector %u",sct);
				return 0;
			}

			/* copy */
			oo = pointer % blk;
			mrd = blk - oo;
			if (mrd > sz) mrd = sz;
			memcpy(buf,mom->tmpsect+oo,mrd);

			/* advance */
			pointer += mrd;
			buf += mrd;
			sz -= mrd;
			rd += mrd;
			sct++;
		}

		return rd;
	}

	/* processing for files/directories */
	if (cluster == 0 || cluster >= (mom->FATMASK-0xF))	return 0;
	if (sz <= 0)						return 0;
	if (pointer >= file_size)				return 0;

	/* constrain request to file size */
	t1=file_size - pointer;
	if (sz > t1) sz = t1;

	/* sanity check */
	t1=pointer - pointer_cr;
	if (t1 >= cluster_size)	mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster > cluster size!");
	else if (t1 < 0)		mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster < 0!");

	if (t1 >= cluster_size) {
		mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster > cluster size!");
		return 0;
	}
	else if (t1 < 0) {
		mom->comment(C_ERROR,"Program error! Pointer - Pointer Cluster < 0!");
		return 0;
	}

	blk = mom->blocksize();
	dsksize = mom->disksize();
	while (sz > 0) {
		/* in case the FAT chain ends too soon... */
		if (cluster == 0 || cluster >= (mom->FATMASK-0xF)) return rd;

		/* from the current cluster, how much can we read? */
		t1 = pointer - pointer_cr;
		crd = cluster_size - t1;
		if (crd > sz) crd = sz;

		t1 -= t1 % mom->BytesPerSector;
		sct = mom->DataAreaOffset + ((cluster - 2) * mom->SectorsPerCluster) + (t1 / mom->BytesPerSector);
		if (sct == 4085)
			sct = sct;

		while (crd > 0) {
			/* read */
			if (sct >= dsksize) {
				mom->comment(C_ERROR,"A part of the filesystem reaches beyond the end of the disk (sector %u, disk size %u)",sct,dsksize);
				return rd;
			}
			else if (!mom->blockread(sct,1,mom->tmpsect)) {
				mom->comment(C_ERROR,"Cannot read sector %u",sct);
				return rd;
			}

			/* from the current sector, how much can we read? */
			oo = pointer % blk;
			mrd = blk - oo;
			if (mrd > crd) mrd = crd;
			if (mrd > sz) mrd = sz;

			/* copy */
			oo = pointer % blk;
			mrd = blk - oo;
			if (mrd > sz) mrd = sz;
			memcpy(buf,mom->tmpsect+oo,mrd);

			/* advance */
			pointer += mrd;
			buf += mrd;
			crd -= mrd;
			sz -= mrd;
			rd += mrd;
			sct++;
		}

		/* if it's time to advance to the next cluster, do it */
		t1 = pointer - pointer_cr;
		if (t1 > cluster_size) {
			t1 = cluster_size;
			mom->comment(C_ERROR,"Program bug: cluster read routine went too far! %u bytes overboard",t1-cluster_size);
		}
		else if (t1 < 0) {
			t1 = 0;
			mom->comment(C_ERROR,"Program bug: cluster read routine went too far! %u bytes behind",-t1);
		}

		if (t1 == cluster_size) {
			/* next cluster! */
			pointer_cr += cluster_size;
			cluster = mom->ReadFAT(cluster);
		}
	}

	return rd;
}

void reader::entity::reset()
{
	pointer = 0;
	pointer_cr = 0;
	cluster = cluster_start;
}

/* get an entity object for the root directory */
reader::entity* reader::rootdirectory()
{
	entity *x;

	x = new entity(this);
	if (!x) return NULL;

	if (FAT == 32) {		/* FAT32 systems have a root directory that can exist anywhere */
		x->linear_offset = 0;
		x->file_size = 0x7FFFFFFF;
		x->cluster_start = RootDirCluster;
	}
	else {
		x->linear_offset = DataAreaOffset - RootDirSectors;
		x->file_size = RootDirEntries * 32;
	}

	x->my_attr = 0x10;	/* subdirectory */
	x->info_valid = 0;
	x->cluster_size = SectorsPerCluster * BytesPerSector;
	x->reset();
	return x;
}

int reader::entity::parse(unsigned char buf[32],reader::entity::FATDIRENTRY *f,unsigned long ofs)
{
	if (!f) return 0;
	if (buf[0] == 0 || buf[0] == 0xE5) return 0;
	f->attr = buf[11];
	if (f->attr == 0xF) return 0;	// leave Windows 95 long filenames alone

	memcpy(f->rawname,buf,11);
	if (f->attr & 8) {				// volume label
		memcpy(f->name,buf,11); f->name[11]=0;
		memset(f->ext,0,4);
		choppadding((char*)f->name);
		strcpy((char*)f->fullname,(char*)f->name);
	}
	else {
		memcpy(f->name,buf,8); f->name[8]=0;
		memcpy(f->ext,buf+8,3); f->ext[3]=0;
		choppadding((char*)f->name);
		choppadding((char*)f->ext);
		if (strlen((char*)f->ext) > 0)	sprintf((char*)f->fullname,"%s.%s",f->name,f->ext);
		else							strcpy((char*)f->fullname,(char*)f->name);
	}

	f->offset =				ofs;
	f->creation_time_cs =	buf[13];
	f->creation_time =		llei16 (buf + 14);
	f->creation_date =		llei16 (buf + 16);
	f->last_access_date =	llei16 (buf + 18);
	if (mom->FAT == 32)		f->OS2_EA_index = 0;
	else					f->OS2_EA_index = llei16 (buf + 20);
	f->time =				llei16 (buf + 22);
	f->date =				llei16 (buf + 24);
	if (mom->FAT == 32)		f->cluster = llei16(buf + 26) | (llei16(buf + 20) << 16);
	else					f->cluster = llei16(buf + 26);
	f->size =				llei32(buf + 28);
	return 1;
}

reader::entity* reader::entity::getdirent(reader::entity::FATDIRENTRY *f)
{
	entity *x;

	x = new entity(mom);
	if (!x) return NULL;

	x->my_attr = f->attr;
	x->cluster_size = cluster_size;
	x->cluster_start = f->cluster;
	x->file_size = (f->attr & 0x10) ? 0x7FFFFFFF : f->size;	/* DOS does not give directories size! */
	x->linear_offset = 0;
	x->reset();
	memcpy(&x->info,f,sizeof(reader::entity::FATDIRENTRY));
	x->info_valid = 1;
	return x;
}

int reader::entity::dir_findfirst(reader::entity::FATDIRENTRY *f)
{
	if (!(my_attr&0x10)) return 0;
	findoffset=seek(0);
	return dir_findnext(f);
}

int reader::entity::dir_findnext(reader::entity::FATDIRENTRY *f)
{
	reader::entity::FATDIRENTRY x;
	unsigned char tmp[32];
	int found=0;

	if (!(my_attr&0x10)) return 0;

	do {
		if (findoffset & 31) return 0;
		if (findoffset != seek(findoffset)) return 0;
		if (read(tmp,32) < 32) return 0;
		if (parse(tmp,&x,findoffset)) found=1;
		findoffset += 32;
	} while (!found);

	memcpy(f,&x,sizeof(x));
	return found;
}

reader::entity::LFN* reader::entity::long_filename(reader::entity::FATDIRENTRY *f)
{
	unsigned long off,scansb,gathered=0,lfnsize=0,xxx;
	unsigned char hash,tmp[32],seq;
	LFN *t;
	int i;

	if (f->offset >= 32) {
		/* Long File Names usually precede the actual entry that they refer to */
		off = f->offset - 32;
		scansb = -32;
	}
	else {
		/* or just scan forward for it */
		off = 0;
		scansb = 32;
	}

	if (seek(off) != off) return NULL;
	hash = lfnhash(f->rawname);
	t = NULL;

	/* look for it */
	while (1) {
		/* if we hit the beginning scanning backwards then scan forwards */
		if (seek(off) != off || read(tmp,32) < 32) {
//			if (scansb == -32) {
//				scansb = 32;
//				off = f->offset;
//			}
//			else {
				break;
//			}
		}

		/* is it a long file name and is it relevant to the one we're looking for? */
		if (tmp[11] == 0xF && tmp[26] == 0 && tmp[27] == 0 &&
			tmp[13] == hash && (seq=(tmp[0] & 31)) != 0) {
			seq--;		/* the sequence numbers are 1-based */

			/* the final part? */
			if (tmp[0] & 0x40) {
				/* good, we know how long it is */
				if (lfnsize != 0)
					mom->comment(C_WARNING,"Long Filename entry @ %u has more than one final part",off);
				if (lfnsize < (seq+1))
					lfnsize = seq+1;
			}

			/* assemble the name from the fragments */
			if (gathered & (1<<seq)) {
				mom->comment(C_WARNING,"Long Filename entry @ %u is a repeated copy of a fragment",off);
			}
			else {
				unsigned short *sw;

				if (!t) t = new LFN;
				sw = t->name_unicode + (13 * seq);
				memcpy(sw     ,tmp +  1,5*2);
				memcpy(sw +  5,tmp + 14,6*2);
				memcpy(sw + 11,tmp + 28,2*2);
				gathered |= 1<<seq;
			}

			/* is that all? */
			xxx = (1<<lfnsize)-1;
			if (lfnsize > 0 && (gathered & xxx) == xxx)
				break;
		}
		else {
			/* if it's not a long filename then quit. the LFN fragments
			   are supposed to immediately precede the file/directory
			   entry they refer to. if they don't, then too bad.
			   LFNs are not vital to the filesystem anyway, and it's
			   something that a diagnostic disk utility can repair. */
			break;
		}

		/* advance */
		off += scansb;
	}

	/* if we got something, check to make sure that all is there */
	if (t) {
		for (i=0;i < 31 && (gathered & (1<<i));) i++;
		if (lfnsize == 0) {
			if (i == 0) {
				delete t;
				t=NULL;
			}
			else {
				mom->comment(C_WARNING,"For file %s at %u the Long Filename length is unknown",f->name,f->offset);
				lfnsize = i;
			}
		}
		else if (lfnsize > i) {
			mom->comment(C_WARNING,"For file %s at %u portions of the Long Filename are missing",f->name,f->offset);
			lfnsize = i;
		}
		else if (lfnsize < i) {
			mom->comment(C_WARNING,"For file %s at %u there are superflous Long Filename entries",f->name,f->offset);
			lfnsize = i;
		}
	}

	if (t) {
		t->length = lfnsize * 13;
		t->name_unicode[t->length] = 0;
	}

	return t;
}

reader::entity::LFN::LFN()
{
	/* 31*13 = the maximum length of a "long file name" */
	name_unicode = new unsigned short[31*13 + 1];
	name_ansi = NULL;
}

reader::entity::LFN::~LFN()
{
	if (name_unicode) delete name_unicode;
	if (name_ansi) delete name_ansi;
}

/* take the unicode name and generate an ansi name */
int reader::entity::LFN::unicode2ansi()
{
	int i,o;

	if (!name_unicode || name_ansi) return 0;
	name_ansi = new unsigned char[length+1];
	if (!name_ansi) return 0;

	for (i=o=0;i < length;i++) {
		/* convert high/low surrogate UTF32 extensions to ? */
		if (name_unicode[i] >= 0xD800 && name_unicode[i] <= 0xDFFF) {
			name_ansi[o++] = '?';
			if (name_unicode[i+1] >= 0xD800 && name_unicode[i+1] <= 0xDFFF)
				i++;
		}
		/* convert anything non-ASCII to ? */
		else if (name_unicode[i] & 0xFF00) {
			name_ansi[o++] = '?';
		}
		else {
			name_ansi[o++] = (unsigned char)(name_unicode[i] & 0xFF);
		}
	}

	name_ansi[o] = 0;
	return 1;
}

/* LFN      = find a file who's "long file name" matches "path"
   root     = entity object who we treat as the "root" directory. can be NULL.
   instance = in case there's more than one file named "path" */
reader::entity* reader::file(char *path,entity* root,int LFN,int instance,int type)
{
	entity::FATDIRENTRY f;
	entity* stk[32],*l=NULL;
	int stksp=0,fl,i,match=0;
	char *s,*next;

	/* we don't work with absolute paths e.g. C:\WINDOWS\CRAWL_LIKE_A_DOG.TXT */
	if (path[1] == ':' && path[0] != 0)
		return NULL;

	/* skip first \ if there */
	s = path;
	while (*s == '\\') s++;

	/* start from the root directory */
	if (root)	stk[stksp] = root;
	else		stk[stksp] = rootdirectory();
	if (!stk[stksp]) return NULL;

	/* find the length of the first element */
	/* further down this code, next = next element or NULL, fl = length of this element */
	next = strchr(s,'\\');
	if (!next)		fl = strlen(s);
	else			fl = (int)(next-s);

	i = stk[stksp]->dir_findfirst(&f);
	while (*s && i) {
		/* first (if requested) match by Long Filename */
		if (LFN) {
			entity::LFN* lfn = stk[stksp]->long_filename(&f);
			if (lfn) {
				if (lfn->unicode2ansi()) {
					if (!strnicmp((char*)lfn->name_ansi,s,fl)) {
						/* Long Filename match */
						match=1;
					}
				}

				delete lfn;
			}
		}

		/* then try to match by the traditional 8.3 name */
		if (!match) {
			if (!strnicmp((char*)f.fullname,s,fl)) {
				/* DOS 8.3 filename match */
				match=1;
			}
		}

		/* it is only a match if this is the instance the caller wants */
		if (!(f.attr & entity::ATTR_DIRECTORY))
			if (match)
				if (instance-- > 0)
					match=0;

		/* now act on the match-up */
		if (match) {
			match=0;
			if (f.attr & entity::ATTR_DIRECTORY) {
				/* last element? */
				if (!next) {
					if (type == FT_TYPE_DIR)
						l=stk[stksp]->getdirent(&f);

					break;
				}
				/* enter the directory */
				else {
					l=stk[stksp]->getdirent(&f);
					if (!l) break;			/* stop if failure */
					if (stksp >= 31) break;	/* stop if too deep */

					i=l->dir_findfirst(&f);
					if (i) {
						s=next+1;
						next=strchr(s,'\\');
						if (!next)		fl = strlen(s);
						else			fl = (int)(next-s);
						stk[++stksp]=l;
						l=NULL;
						continue;
					}
					else {
						delete l;
						l=NULL;
					}
				}
			}
			else {
				if (type != FT_TYPE_FILE) {
					break;
				}
				/* not the last element? */
				else if (next) {
					/* disallow using files like directories */
					break;
				}
				/* found it */
				else {
					/* doesn't matter if this fails, it will
					   only cause this function to return NULL */
					l=stk[stksp]->getdirent(&f);
					break;
				}
			}
		}

		/* next? */
		if (!l) {
			i = stk[stksp]->dir_findnext(&f);
			if (!i) break;
		}
	}

	/* clear the objects we used. DO NOT DELETE THE
	   "root" OBJECT GIVEN TO US BY THE CALLER!!! */
	while (stksp >= 0) {
		if (stk[stksp] != root)	delete stk[stksp--];
		else					stksp--;
	}

	return l;
}

////////////////////////////////////////////////
};
// END CDOSFat namespace
