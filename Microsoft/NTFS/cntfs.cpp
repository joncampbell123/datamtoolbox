
/* a C++ class for reading the NTFS filesystem
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 *
 * NTFS: God what a mess.....
 */

#include "cntfs.h"
#include <string.h>
#include <stdlib.h>
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

// BEGIN CNTFS namespace
namespace CNTFS {
////////////////////////////////////////////////

int reader::init()
{
	mounted=0;
	tmpcluster=NULL;
	RootDir_MFT=NULL;
	tmpsect=NULL;
	The_MFT=NULL;
	tmpMFT=NULL;
	return 1;
}

int reader::freeMe()
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
	else if (BPBSize < 84) {
		comment(C_ERROR,"BPB too small to be NTFS BPB (%u < 33)",BPBSize);
		return 0;
	}

	if (!(tmp[510] == 0x55 && tmp[511] == 0xAA))
		return 0;

	// must be NTFS, not FAT
	if (memcmp(tmp + 3,"NTFS",4)) {
		comment(C_ERROR,"This is not an NTFS partition!");
		return 0;
	}

	BytesPerSector =			llei16(tmp +  11);
	SectorsPerCluster =			tmp[13];
	ReservedSectors =			llei16(tmp +  14);
	FATCopies =					tmp[16];
	RootDirEntries =			llei16(tmp +  17);
	MediaDescriptor	=			tmp[21];
	SectorsPerFAT =				llei16(tmp +  22);
	SectorsPerTrack =			llei16(tmp +  24);
	Heads =						llei16(tmp +  26);
	HiddenSectors =				llei32(tmp +  28);
	TotalSectors =				llei64(tmp +  40);
	MFT_cluster =				llei64(tmp +  48);	// logical cluster of the $MFT
	MFT_cluster_mirror =		llei64(tmp +  56);	// logical cluster of the $MFT mirror
	ClustersPerFileRecSeg =		llei32(tmp +  64);
	ClustersPerIndexBlock =		llei32(tmp +  68);
	VolumeSerial =				llei64(tmp +  72);
	Checksum =					llei32(tmp +  80);

	if (FATCopies != 0 || RootDirEntries != 0 || SectorsPerFAT != 0) {
		comment(C_ERROR,"NTFS partition BPB has invalid values!");
		return 0;
	}

	comment(C_INFO,"Volume serial no 0x%08X%08X",
		((unsigned long)(VolumeSerial >> ((uint64)32))),
		((unsigned long)(VolumeSerial & 0xFFFFFFFF)));

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
	else if (!IsPow2(SectorsPerCluster)) {
		comment(C_ERROR,"Sectors/cluster field %u is not a power of 2",SectorsPerCluster);
		return 0;
	}

	if (MediaDescriptor != 0xF8)
		comment(C_WARNING,"Odd media descriptor %0x2X",MediaDescriptor);

	if (TotalSectors > disksize())
		comment(C_WARNING,"TotalSectors is too large for this disk (%I64u > %I64u)",TotalSectors,disksize());

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

	tmpsect = new unsigned char[sectsize];
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

	tmpcluster = new unsigned char[BytesPerSector * SectorsPerCluster];
	if (!tmpcluster) {
		comment(C_ERROR,"Not enough memory for cluster buffer!");
		umount();
		return 0;
	}

	tmpMFT = new unsigned char[1024];
	if (!tmpMFT) {
		comment(C_ERROR,"Not enough memory for cluster buffer!");
		umount();
		return 0;
	}

	return 1;
}

unsigned char* reader::ReadCluster(uint64 n)
{
	n *= SectorsPerCluster;
	int r = blockread(n,SectorsPerCluster,tmpcluster);
	if (r == 0) return NULL;
	return tmpcluster;
}

unsigned char* reader::ReadMFT(uint64 n)
{
	sint64 clus;
	int cl,cls;

	if (!The_MFT->data_runlist)
		return NULL;

	n *= 2;
	cl = n / SectorsPerCluster;
	cls = n % SectorsPerCluster;
	clus = The_MFT->data_runlist->Map(cl,NULL);
	if (clus == ((sint64)-1)) return NULL;
	clus *= SectorsPerCluster;
	clus += cls;
	int r = blockread(clus,2,tmpMFT);
	if (r == 0) return NULL;
	return tmpMFT;
}

unsigned char* reader::ReadMFTUnmapped(uint64 n)
{
	sint64 clus;
	int cl,cls;

	if (!The_MFT->data_runlist)
		return NULL;

	n *= 2;
	cl = n / SectorsPerCluster;
	cls = n % SectorsPerCluster;
	clus = cl + MFT_cluster;
	clus *= SectorsPerCluster;
	clus += cls;
	int r = blockread(clus,2,tmpMFT);
	if (r == 0) return NULL;
	return tmpMFT;
}

int reader::umount()
{
	if (RootDir_MFT) delete RootDir_MFT;
	RootDir_MFT=NULL;

	if (The_MFT) delete The_MFT;
	The_MFT=NULL;

	if (!mounted) {
		comment(C_ERROR,"Already not mounted!");
		return 0;
	}

	if (tmpcluster) {
		delete tmpcluster;
		tmpcluster=NULL;
	}

	if (tmpsect) {
		delete tmpsect;
		tmpsect=NULL;
	}

	if (tmpMFT) {
		delete tmpMFT;
		tmpMFT=NULL;
	}

	mounted=0;
	return 1;
}

reader::MFT::RecordReader::RecordReader(reader *x)
{
	mom = x;
	copy = NULL;
	attr_enum = NULL;
}

reader::MFT::RecordReader::~RecordReader()
{
	if (copy) delete copy;
	copy=NULL;
}

int reader::MFT::RecordReader::Parse(uint64 n)
{
	unsigned char *buf;

	if (copy) delete copy;
	copy=NULL;

	if (n == 0) {
		if (!(buf = mom->ReadMFTUnmapped(n)))
			return 0;
	}
	else {
		if (!(buf = mom->ReadMFT(n)))
			return 0;
	}

	copy = new unsigned char [1024];
	if (!copy) return 0;

	bytespersec = 512;
	clustsize = 1024;
	memcpy(copy,buf,clustsize);

	memcpy(		header.magic,						buf +		 0, 4);
				header.usa_ofs =			llei16(	buf +		 4);
				header.usa_count =			llei16(	buf +		 6);
				header.lsn =				llei64(	buf +		 8);
				header.seq_number =			llei16(	buf +		16);
				header.link_count =			llei16(	buf +		18);
				header.attr_offset =		llei16(	buf +		20);
				header.flags =				llei16(	buf +		22);
				header.bytes_used =			llei32(	buf +		24);
				header.bytes_alloc =		llei32(	buf +		28);
				header.base_mft =			llei64(	buf +		32);
				header.next_attr_inst =		llei16(	buf +		40);
				header.reserved =			llei16(	buf +		42);
				header.mft_record_number =	llei32(	buf +		44);

	if (memcmp(header.magic,"FILE",4)) {
		mom->comment(C_WARNING,"MFT record in entry %u does not have FILE magic! Ignoring.",(unsigned int)n);
		return 0;
	}

	// commence bizarre USA patching ritual
	if (header.usa_count > 0) {
		unsigned char *usa;
		int Y=0,updval,upd;

		upd = 1;
		usa = buf + header.usa_ofs;
		updval = llei16(usa);
		for (Y=0;Y < 2;Y++)
			if (llei16(buf + (Y*512) + 510) != updval)
				return 0;

		for (Y=0;Y < 2;Y++) {
			slei16(buf + (Y*512) + 510,llei16(usa + (2 * upd)));
			slei16(copy + (Y*512) + 510,llei16(usa + (2 * upd)));
			if (upd < (header.usa_count-1)) upd++;
		}
	}

	return 1;
}

int reader::MFT::RecordReader::ReadUSA(unsigned int n)
{
	int o;

	if (n >= header.usa_count) return -1;
	o = header.usa_ofs + (n * 2);
	if (o >= (clustsize-1)) {
		mom->comment(C_ERROR,"MFT record: Attempt to access beyond end of cluster. Spanning MFT USA reading not supported!");
		return -1;
	}
	if (o >= (header.attr_offset-1)) {
		mom->comment(C_ERROR,"MFT record: Attempt to USA that overlaps the first attribute");
		return -1;
	}

	return llei16(copy + o);
}

reader::MFT::RecordReader* reader::ReadMFTNomapping(uint64 n)
{
	reader::MFT::RecordReader* rr = new MFT::RecordReader(this);
	if (!rr) return NULL;
	if (!rr->Parse(n)) {
		delete rr;
		return NULL;
	}

	return rr;
}

int reader::MFT::RecordReader::ParseAttr(unsigned char *buf,reader::MFT::ATTR_RECORD *a)
{
	unsigned char *attr;
	int sz=0;

	memset(a,0,sizeof(reader::MFT::ATTR_RECORD));

	a->type =				llei32(	buf +	 0);
	a->length =				llei32(	buf +	 4);
	a->non_resident =				buf[	 8];
	a->name_length =				buf[	 9];
	a->name_offset =		llei16(	buf +	10);
	a->flags =				llei16(	buf +	12);
	a->instance =			llei16(	buf +	14);

	sz = a->length;
	attr = buf + 16;
	if (a->non_resident) {
		a->data.nonresident.lowest_vcn =				llei64(	attr +	 0);
		a->data.nonresident.highest_vcn =				llei64(	attr +	 8);
		a->data.nonresident.mapping_pairs_offset =		llei16(	attr +	16);
		a->data.nonresident.compression_unit =					attr[	18];
		memcpy(a->data.nonresident.reserved,					attr +	19,		5);
		a->data.nonresident.allocated_size =			llei64(	attr +	24);
		a->data.nonresident.data_size =					llei64(	attr +	32);
		a->data.nonresident.initialized_size =			llei64(	attr +	40);
		a->data.nonresident.compressed_size =			llei64(	attr +	48);

		a->attr_resident_value_ptr = NULL;
		a->attr_resident_value_ptr_fence = NULL;
	}
	else {
		a->data.resident.value_length =					llei32(	attr +	 0);
		a->data.resident.value_offset =					llei16(	attr +	 4);
		a->data.resident.flags =								attr[	 6];
		a->data.resident.reserved =								attr[	 7];

		a->attr_resident_value_ptr = buf + a->data.resident.value_offset;
		a->attr_resident_value_ptr_fence = a->attr_resident_value_ptr + a->data.resident.value_length;
	}

	return sz;
}

int reader::MFT::RecordReader::AttrFirst()
{
	memset(&attr_cur,0,sizeof(attr_cur));
	attr_enum = copy + header.attr_offset;
	attr_fence = copy + header.bytes_used;
	return AttrNext();
}

int reader::MFT::RecordReader::AttrNext()
{
	int sz;

	if (attr_enum >= attr_fence)
		return 0;
	if (attr_cur.type == AT_END)
		return 0;

	sz = ParseAttr(attr_enum,&attr_cur);
	attr_cur.attr_ptr = attr_enum;
	attr_cur.attr_rec_ptr = attr_cur.attr_ptr + 16;
	if ((sz + attr_enum) > attr_fence)
		return 0;

	attr_enum += sz;
	return 1;
}

reader::MFT::RecordReader::AttrFilename::AttrFilename()
{
	name_u=NULL;
	name_a=NULL;
}

reader::MFT::RecordReader::AttrFilename::~AttrFilename()
{
	if (name_u) delete name_u;
	name_u=NULL;
	if (name_a) delete name_a;
	name_a=NULL;
}

int reader::MFT::RecordReader::AttrFilename::ParseName(unsigned char *buf,int buflen)
{
	if (name_u) delete name_u;
	name_u=NULL;
	if (name_a) delete name_a;
	name_a=NULL;

	parent_directory =			llei64(	buf +	 0);
	creation_time =				llei64(	buf +	 8);
	last_data_change_time =		llei64(	buf +	16);
	last_mft_change_time =		llei64(	buf +	24);
	last_access_time =			llei64(	buf +	32);
	allocated_size =			llei64(	buf +	40);
	data_size =					llei64(	buf +	48);
	file_attributes =			llei32(	buf +	56);
	packed_ea_size =			llei16(	buf +	60);
	reparse_point_tag =			llei32(	buf +	60);
	file_name_length =					buf[	64];
	file_name_type =					buf[	65];

	name_u = new unsigned short[file_name_length + 1];
	if (!name_u) return 0;

	memcpy(name_u,buf + 66,file_name_length * 2);
	name_u[file_name_length] = 0;

	return 1;
}

int	reader::MFT::RecordReader::AttrFilename::ParseName(ATTR_RECORD *r)
{
	if (r->type != AT_FILE_NAME) return 0;
	if (r->non_resident) return 0;
	return ParseName(r->attr_ptr + r->data.resident.value_offset,r->data.resident.value_length);
}

int reader::MFT::RecordReader::AttrFilename::GetName(RecordReader *rr,int namspace)
{
	if (name_u) delete name_u;
	name_u=NULL;
	if (name_a) delete name_a;
	name_a=NULL;

	// look for first Filename attribute
	if (!rr->AttrFirst()) return 0;

	do {
		if (!rr->AttrNext()) return 0;
		if (!ParseName(&rr->attr_cur)) continue;
		if (file_name_type != namspace) continue;
		break;
	} while (1);

	return 1;
}

int reader::MFT::RecordReader::AttrFilename::Unicode2Ansi()
{
	int i,l;

	if (!name_u) return 0;
	l=wcslen(name_u);
	if (name_a) delete name_a;
	name_a = new char[l+1];
	if (!name_a) return 0;

	for (i=0;i < l;i++) {
		if (name_u[i] >= 0x80)		name_a[i] = '?';
		else						name_a[i] = (char)name_u[i];
	}
	name_a[i] = 0;
	return 1;
}

int reader::MFT::RecordReader::ParseStdInfo(ATTR_RECORD *a,ATTR_StandardInformation *asi)
{
	unsigned char *buf;
	int buflen;

	if (!a || !asi) return 0;
	if (a->non_resident) return 0;
	buflen = a->data.resident.value_length;
	buf = a->attr_ptr + a->data.resident.value_offset;
	asi->creation_time =				llei64(	buf +	 0);
	asi->last_data_change_time =		llei64(	buf +	 8);
	asi->last_mft_change_time =			llei64(	buf +	16);
	asi->last_access_time =				llei64(	buf +	24);
	asi->file_attributes =				llei32(	buf +	32);
	asi->maximum_versions =				llei32(	buf +	36);
	asi->version_number =				llei32(	buf +	40);
	asi->class_id =						llei32(	buf +	44);
	asi->owner_id =						llei32(	buf +	48);
	asi->security_id =					llei32(	buf +	52);
	asi->quota_charged =				llei64(	buf +	56);
	asi->usn =							llei64(	buf +	64);
	return 1;
}

int reader::CheckMFT()
{
	uint64 last_vcn=0,next_vcn=0;
	int datrec=0;

	if (The_MFT) return 1;

	The_MFT = new MasterMFT(this);
	if (!The_MFT) return 0;

	if (!The_MFT->CheckMFT())
		goto fail;

	if (!(The_MFT->header.flags & MFT::MFT_RECORD_IN_USE))
		goto fail;

	// look for the attribute list
	if (!The_MFT->AttrFirst()) {
		comment(C_ERROR,"$MFT has no attributes!");
		goto fail;
	}

	do {
		if (The_MFT->attr_cur.type == MFT::AT_DATA) {
			// the data record must be non-resident
			if (!The_MFT->attr_cur.non_resident) {
				comment(C_ERROR,"Unexpected: $MFT is resident!");
				goto fail;
			}

			// must not be compressed, encrypted, or sparse
			if (The_MFT->attr_cur.flags & (MFT::ATTR_COMPRESS_MASK | MFT::ATTR_IS_ENCRYPTED | MFT::ATTR_IS_SPARSE)) {
				comment(C_ERROR,"$MFT that are compressed/encrypted/spare not supported!");
				goto fail;
			}

			// the lowest Virtual Cluster Number must be 0
			if (The_MFT->attr_cur.data.nonresident.lowest_vcn != 0 && datrec == 0) {
				comment(C_ERROR,"$MFT is corrupt: Lowest VCN != 0!");
				goto fail;
			}
			if (The_MFT->attr_cur.data.nonresident.lowest_vcn < next_vcn) {
				comment(C_ERROR,"$MFT is corrupt: Lowest VCN less than highest VCN + 1 of previous attr record!");
				goto fail;
			}

			// okay (puke) let's unpack the VCN to LCN mappings
			if (!The_MFT->data_runlist->Parse(&The_MFT->attr_cur))
				goto fail;

			// take note of stuff
			if (datrec == 0) last_vcn = The_MFT->attr_cur.data.nonresident.allocated_size / (BytesPerSector * SectorsPerCluster);
			next_vcn = The_MFT->attr_cur.data.nonresident.highest_vcn + 1;

			// count the data records
			datrec++;
		}
	} while (The_MFT->AttrNext());

	if (next_vcn != last_vcn) {
		comment(C_ERROR,"$MFT is corrupt: Incomplete runlist!");
		goto fail;
	}

	The_MFT->data_runlist->Sort();

	return 1;

fail:
	delete The_MFT;
	The_MFT=NULL;
	return 0;
}

reader::MFT::NTFSVCNRunlist::NTFSVCNRunlist(reader *x)
{
	mom = x;
	list = NULL;
	list_max = 0;
	list_alloc = 0;
}

reader::MFT::NTFSVCNRunlist::~NTFSVCNRunlist()
{
	if (list) free(list);
}

int reader::MFT::NTFSVCNRunlist::Parse(ATTR_RECORD *r)
{
	unsigned char *buf,*bufend,b;
	sint64 deltaxcn,nextlcn;
	sint64 vcn,lcn;
	ent* e;

	bufend = r->attr_ptr + r->length;
	buf = r->attr_ptr + r->data.nonresident.mapping_pairs_offset;
	vcn = r->data.nonresident.lowest_vcn;
	lcn = 0;

	if (vcn) {
		if (e=Alloc()) {
			e->vcn = 0;
			e->lcn = LCN_NOT_MAPPED;
			e->len = vcn;
		}
	}

	while (buf < bufend && *buf) {
		if (!(e=Alloc())) return 0;
		e->vcn = vcn;
		b = *buf & 0xF;
		if (b) {
			if ((buf + b) > bufend) return 0;
			deltaxcn = (sint64)((signed char)buf[b--]);
			while (b) deltaxcn = (deltaxcn << ((sint64)8)) | buf[b--];
		}
		else {
			deltaxcn = (sint64)(-1);
		}

		if (deltaxcn < 0)
			return 0;

		e->len = deltaxcn;
		vcn += deltaxcn;
		if (!(*buf & 0xF0)) {
			e->lcn = LCN_HOLE;
		}
		else {
			unsigned char b2 = *buf & 0xF;
			b = b2 + ((*buf >> 4) & 0xF);
			if ((buf + b) > bufend) return 0;
			deltaxcn = (sint64)((signed char)buf[b--]);
			while (b > b2) deltaxcn = (deltaxcn << ((sint64)8)) | buf[b--];
			lcn += deltaxcn;
			if (lcn < ((sint64)-1)) return 0;
			e->lcn = lcn;
		}

		buf += (*buf & 0xF) + ((*buf >> 4) & 0xF) + 1;
	}

	// check for corrupt mapping pairs
	deltaxcn = r->data.nonresident.highest_vcn;
	if (deltaxcn && (vcn - 1) != deltaxcn) return 0;

	// base extent?
	if (!r->data.nonresident.lowest_vcn) {
		sint64 bpc = mom->SectorsPerCluster * mom->BytesPerSector;
		sint64 max_cluster = ((r->data.nonresident.allocated_size + bpc - 1) / bpc) - 1;

		if (deltaxcn) {
			if (deltaxcn < max_cluster) {
				if (!(e=Alloc())) return 0;
				e->vcn = vcn;
				e->len = max_cluster - deltaxcn;
				e->lcn = LCN_NOT_MAPPED;
			}
			else if (deltaxcn > max_cluster) {
				return 0;
			}
		}

		nextlcn = LCN_ENOENT;
	}
	else {
		nextlcn = LCN_NOT_MAPPED;
	}

	if (!(e=Alloc())) return 0;
	e->lcn = nextlcn;
	e->vcn = vcn;
	e->len = 0;

	return 1;
}

reader::MFT::NTFSVCNRunlist::ent* reader::MFT::NTFSVCNRunlist::Alloc()
{
	ent* ne;

	if (!list) {
		list_alloc = 16;
		list_max = 0;
		list = (ent*)::malloc(sizeof(ent) * list_alloc);
		if (!list) return NULL;
	}

	if (list_max >= list_alloc) {
		ent* nw;

		list_alloc += 16;
		nw = (ent*)::realloc((void*)list,sizeof(ent) * list_alloc);
		if (!nw) return NULL;
		list = nw;
	}

	ne = list + list_max++;
	memset(ne,0,sizeof(ent));
	return ne;
}

int reader::MFT::NTFSVCNRunlist::__sort(const void *elem1,const void *elem2)
{
	ent *e1 = (ent*)elem1;
	ent *e2 = (ent*)elem2;
	return (int)(e1->vcn - e2->vcn);
}

void reader::MFT::NTFSVCNRunlist::Sort()
{
	if (!list) return;
	qsort(list,list_max,sizeof(ent),__sort);
}

reader::MasterMFT::MasterMFT(reader *x) : reader::MFT::RecordReader(x)
{
	data_runlist = new MFT::NTFSVCNRunlist(x);
	mom = x;
}

reader::MasterMFT::~MasterMFT()
{
	if (data_runlist) delete data_runlist;
}

int reader::MasterMFT::CheckMFT()
{
	if (!Parse(0)) return 0;
	return 1;
}

int reader::MasterMFT::ParseMapped(uint64 vcn)
{
	sint64 cluster = Map(vcn,NULL);
	if (cluster == ((sint64)-1)) return 0;
	return Parse(cluster);
}

sint64 reader::MFT::NTFSVCNRunlist::Map(uint64 N,reader::MFT::NTFSVCNRunlist::ent *en)
{
	int i;

	if (!list) return -1;

	// scan the VCN -> LCN runlist
	for (i=0;i < list_max;i++) {
		if (list[i].vcn > N) return -1;
		if ((list[i].vcn + list[i].len) <= N) continue;
		if (en) memcpy(en,&list[i],sizeof(MFT::NTFSVCNRunlist::ent));
		if (list[i].lcn < 0) return -1;
		return list[i].lcn + (N - list[i].vcn);
	}

	return -1;
}

sint64 reader::MasterMFT::Map(uint64 N,MFT::NTFSVCNRunlist::ent *en)
{
	if (!data_runlist) return 0;
	return data_runlist->Map(N,en);
}

reader::MFT::RecordReader* reader::GetRootDir()
{
	if (!The_MFT) return NULL;
	if (RootDir_MFT) return RootDir_MFT;
	RootDir_MFT = ReadMFTNomapping(reader::MFT::FILE_RootDirectory);
	return RootDir_MFT;
}

reader::MFT::NTFSIndexCrap::NTFSIndexCrap(RecordReader *x)
{
	enum_type = reader::MFT::AT_FILE_NAME;
	enum_attrA0_state.runlist = NULL;
	enum_attrA0_state.INDX_buffer = NULL;
	enum_attrA0_state.INDX_fence = NULL;
	enum_attrA0_state.INDX_ptr = NULL;
	mom = x;
}

reader::MFT::NTFSIndexCrap::~NTFSIndexCrap()
{
	if (enum_attrA0_state.runlist) delete enum_attrA0_state.runlist;
	enum_attrA0_state.runlist=NULL;
	if (enum_attrA0_state.INDX_buffer) delete enum_attrA0_state.INDX_buffer;
	enum_attrA0_state.INDX_buffer = NULL;
	enum_attrA0_state.INDX_fence = NULL;
	enum_attrA0_state.INDX_ptr = NULL;
}

int	reader::MFT::NTFSIndexCrap::FindFirst()
{
	enum_state = STATE_ATTR_FIRST;
	if (enum_attrA0_state.runlist) delete enum_attrA0_state.runlist;
	if (enum_attrA0_state.INDX_buffer) delete enum_attrA0_state.INDX_buffer;
	enum_attrA0_state.INDX_buffer = NULL;
	enum_attrA0_state.INDX_fence = NULL;
	enum_attrA0_state.INDX_ptr = NULL;
	enum_attrA0_state.runlist = NULL;
	enum_result_header = NULL;
	return FindNext();
}

int reader::MFT::NTFSIndexCrap::FindNext()
{
	int ret=0,limbo=1;

	while (limbo && ret == 0) {
		if (enum_state == STATE_ATTR_FIRST) {
			if (!mom->AttrFirst()) {
				enum_state = STATE_FINISHED;
			}
			else {
				enum_state = STATE_ATTR_SCAN;
			}
		}
		else if (enum_state == STATE_ATTR_SCAN) {
			if (!mom->AttrNext()) {
				enum_state = STATE_FINISHED;
			}
		}

		// TODO: support for encrypted/compressed/sparse attributes
		if (enum_state != STATE_FINISHED && mom->attr_cur.flags & (ATTR_IS_SPARSE|ATTR_IS_ENCRYPTED)) {
			enum_state = STATE_ATTR_SCAN;
			continue;
		}

		switch (enum_state) {
			case STATE_ATTR_SCAN:
				if (mom->attr_cur.type == CNTFS::reader::MFT::AT_INDEX_ROOT && mom->attr_cur.non_resident == 0) {
					unsigned char *buf,*bufend;

					// Apparently even if the "compressed bit" is set
					// the attribute itself is not compressed.

					buf = mom->attr_cur.attr_resident_value_ptr;
					bufend = mom->attr_cur.attr_resident_value_ptr_fence;
					enum_attr90_state.header.type =						llei32(	buf+0x000);
					enum_attr90_state.header.collation_rule =			llei32(	buf+0x004);
					enum_attr90_state.header.index_block_size =			llei32(	buf+0x008);
					enum_attr90_state.header.clusters_per_index_block =			buf[0x00C];
					memcpy(enum_attr90_state.header.reserved,					buf+0x00D,3);
					enum_attr90_state.header.index.entries_offset =		llei32(	buf+0x010);
					enum_attr90_state.header.index.index_length =		llei32(	buf+0x014);
					enum_attr90_state.header.index.allocated_size =		llei32(	buf+0x018);
					enum_attr90_state.header.index.flags =						buf[0x01C];
					memcpy(enum_attr90_state.header.index.reserved,				buf+0x01D,3);

					if (!IsPow2(enum_attr90_state.header.index_block_size))
						break;

					// compute the smallest unit we will read things in
					if (enum_attr90_state.header.index_block_size >= (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector)) {
						enum_attrA0_state.INDX_chunk_sz = enum_attr90_state.header.index_block_size;
						enum_attrA0_state.INDX_subchunk_max = 1;
						if (!IsPow2(enum_attrA0_state.INDX_chunk_sz / (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector)))
							break;
					}
					else {
						enum_attrA0_state.INDX_chunk_sz = mom->mom->SectorsPerCluster * mom->mom->BytesPerSector;
						enum_attrA0_state.INDX_subchunk_max = enum_attrA0_state.INDX_chunk_sz / (1 << enum_attr90_state.header.clusters_per_index_block);
					}

					enum_attr90_state.ent = (unsigned char *)(buf + 0x10 + enum_attr90_state.header.index.entries_offset);
					enum_attr90_state.fence = (unsigned char *)(buf + 0x10 + enum_attr90_state.header.index.index_length);
					enum_result_header = &enum_attr90_state.header;

					// we will go around the horn to STATE_ATTR_90_ENUM if this
					// index represents what the caller is interested in
					if (enum_attr90_state.header.type == enum_type)
						enum_state = STATE_ATTR_90_ENUM;
				}
				else if (mom->attr_cur.type == CNTFS::reader::MFT::AT_INDEX_ALLOCATION && mom->attr_cur.non_resident == 1 && enum_result_header) {
					if (mom->attr_cur.flags & ATTR_IS_COMPRESSED)
						break;

					if (enum_attrA0_state.runlist) delete enum_attrA0_state.runlist;
					enum_attrA0_state.runlist = new NTFSVCNRunlist(mom->mom);
					if (enum_attrA0_state.runlist) {
						if (enum_attrA0_state.runlist->Parse(&mom->attr_cur)) {
							if (enum_attrA0_state.runlist->list_max > 0) {
								enum_attrA0_state.current = 0;
								enum_attrA0_state.max = enum_attrA0_state.runlist->list[enum_attrA0_state.runlist->list_max-1].vcn + enum_attrA0_state.runlist->list[enum_attrA0_state.runlist->list_max-1].len;
								if (enum_attrA0_state.max > 0) {
									enum_attrA0_state.INDX_buffer = new unsigned char[enum_attrA0_state.INDX_chunk_sz];
									if (enum_attrA0_state.INDX_buffer) {
										enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
										enum_attrA0_state.INDX_ptr = enum_attrA0_state.INDX_buffer;
										enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_ptr;
										enum_attrA0_state.INDX_fence = enum_attrA0_state.INDX_buffer + enum_attrA0_state.INDX_chunk_sz;
										enum_state = STATE_ATTR_A0_ENUM;
									}
								}
							}
						}
					}
				}
				break;

			case STATE_ATTR_90_ENUM:
				if (enum_attr90_state.ent >= (enum_attr90_state.fence - 0x10)) {
					enum_state = STATE_ATTR_SCAN;
					break;
				}

				enum_result.length =		llei16(	enum_attr90_state.ent+0x8);
				enum_result.key_length =	llei16(	enum_attr90_state.ent+0xA);
				enum_result.flags =			llei16(	enum_attr90_state.ent+0xC);
				enum_result.reserved =		llei16(	enum_attr90_state.ent+0xE);
				if (enum_result.flags & CNTFS::reader::MFT::INDEX_ENTRY_END) {
					enum_result.data.dir.indexed_file =	0;
					enum_result.data.vi.data_offset =		llei16(	enum_attr90_state.ent+0x0);
					enum_result.data.vi.data_length =		llei16(	enum_attr90_state.ent+0x2);
					enum_result.data.vi.reservedV =			llei32(	enum_attr90_state.ent+0x4);
					enum_result.p_data = enum_attr90_state.ent + enum_result.data.vi.data_offset;
					enum_result.p_data_fence = enum_result.p_data + enum_result.data.vi.data_length;
				}
				else {
					enum_result.data.dir.indexed_file =		llei16(	enum_attr90_state.ent+0x0);
					enum_result.data.vi.data_offset =				0;
					enum_result.data.vi.data_length =				0;
					enum_result.data.vi.reservedV =					0;
					enum_result.p_data_fence =						NULL;
					enum_result.p_data =							NULL;
				}

				enum_result.key = enum_attr90_state.ent + 16;
				enum_result.key_fence = enum_result.key + enum_result.key_length;
				enum_attr90_state.ent += enum_result.length;

				// found an entry, terminate loop so caller can examine it
				limbo = 0;
				ret = 1;
				break;

			case STATE_ATTR_A0_ENUM:
				if (enum_attrA0_state.INDX_ptr >= enum_attrA0_state.INDX_last) {
					if (enum_attrA0_state.INDX_subchunk_N >= enum_attrA0_state.INDX_subchunk_max) {
						sint64 clus,fclus;
						int imax,i;

						if (enum_attrA0_state.current >= enum_attrA0_state.max) {
							enum_state = STATE_ATTR_SCAN;
							break;
						}

						// load up the next INDX
						{
							reader::MFT::NTFSVCNRunlist::ent ent;

							if (enum_attr90_state.header.index_block_size >= (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector)) {
								imax = enum_attr90_state.header.index_block_size / (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector);
							}
							else {
								imax = 1;
							}

							for (i=0;i < imax;i++) {
								clus = enum_attrA0_state.runlist->Map(enum_attrA0_state.current++,&ent);
								if (i == 0) fclus = clus;
								if (clus == ((sint64)-1)) {
									memset(enum_attrA0_state.INDX_buffer + (i * mom->mom->SectorsPerCluster * mom->mom->BytesPerSector),
										0,mom->mom->SectorsPerCluster * mom->mom->BytesPerSector);
								}
								else {
									unsigned char *ptr = mom->mom->ReadCluster(clus);

									if (ptr)
										memcpy(enum_attrA0_state.INDX_buffer + (i * mom->mom->SectorsPerCluster * mom->mom->BytesPerSector),
											ptr,mom->mom->SectorsPerCluster * mom->mom->BytesPerSector);
									else
										memset(enum_attrA0_state.INDX_buffer + (i * mom->mom->SectorsPerCluster * mom->mom->BytesPerSector),
											0,mom->mom->SectorsPerCluster * mom->mom->BytesPerSector);
								}
							}
						}

						// parse the INDX chunk
						if (memcmp(enum_attrA0_state.INDX_buffer,"INDX",4)) {
							enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
							break;
						}

						enum_attrA0_state.INDX_ptr = enum_attrA0_state.INDX_buffer;
						if (enum_attr90_state.header.index_block_size >= (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector))
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_ptr + enum_attr90_state.header.index_block_size;
						else
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_ptr + (1 << enum_attr90_state.header.clusters_per_index_block);

						enum_attrA0_state.usa_ofs =							llei16(	enum_attrA0_state.INDX_ptr +	 4);
						enum_attrA0_state.usa_count =						llei16(	enum_attrA0_state.INDX_ptr +	 6);
						enum_attrA0_state.lsn =								llei64(	enum_attrA0_state.INDX_ptr +	 8);
						enum_attrA0_state.index_block_vcn =					llei64(	enum_attrA0_state.INDX_ptr +	16);
						enum_attrA0_state.INDX_ptr +=															    24;
						enum_attrA0_state.index_header.entries_offset =		llei32(	enum_attrA0_state.INDX_ptr +	 0);
						enum_attrA0_state.index_header.index_length =		llei32(	enum_attrA0_state.INDX_ptr +	 4);
						enum_attrA0_state.index_header.allocated_size =		llei32(	enum_attrA0_state.INDX_ptr +	 8);
						enum_attrA0_state.index_header.flags =						enum_attrA0_state.INDX_ptr[     12];
						enum_attrA0_state.INDX_chklast =							enum_attrA0_state.INDX_ptr + enum_attrA0_state.index_header.index_length;

						if (enum_attrA0_state.usa_count < (1+imax) ||
							enum_attrA0_state.usa_ofs >= (mom->mom->BytesPerSector-1)) {
							enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
							break;
						}

						// commence bizarre USA error checking and swapping of WORDs
						{
							unsigned short usaval;
							int i;

							usaval = llei16(enum_attrA0_state.INDX_buffer + enum_attrA0_state.usa_ofs);
							for (i=0;i < imax;i++) {
								if (llei16(enum_attrA0_state.INDX_buffer + (512*i) + 510) != usaval) {
									enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
									break;
								}
							}

							for (i=0;i < imax;i++)
								slei16(enum_attrA0_state.INDX_buffer + (512*i) + 510,llei16(enum_attrA0_state.INDX_buffer + enum_attrA0_state.usa_ofs + ((i+1)*2)));
						}

						if (enum_attrA0_state.INDX_last > enum_attrA0_state.INDX_chklast)
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_chklast;

						enum_attrA0_state.INDX_ptr += enum_attrA0_state.index_header.entries_offset;
						if (enum_attrA0_state.INDX_ptr >= enum_attrA0_state.INDX_last) {
							enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
							break;
						}

						enum_attrA0_state.INDX_subchunk_N = 1;
					}
					else {
						enum_attrA0_state.INDX_ptr = enum_attrA0_state.INDX_buffer;
						if (enum_attr90_state.header.index_block_size < (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector))
							enum_attrA0_state.INDX_ptr += enum_attrA0_state.INDX_subchunk_N * (1 << enum_attr90_state.header.clusters_per_index_block);

						if (enum_attr90_state.header.index_block_size >= (mom->mom->SectorsPerCluster * mom->mom->BytesPerSector))
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_ptr + enum_attr90_state.header.index_block_size;
						else
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_ptr + (1 << enum_attr90_state.header.clusters_per_index_block);

						if (enum_attrA0_state.INDX_last > enum_attrA0_state.INDX_chklast)
							enum_attrA0_state.INDX_last = enum_attrA0_state.INDX_chklast;

						enum_attrA0_state.INDX_subchunk_N++;
					}
				}

				// so.... any entries?
				if (enum_attrA0_state.INDX_ptr < (enum_attrA0_state.INDX_last-10)) {
					enum_result.length =		llei16(	enum_attrA0_state.INDX_ptr+0x8);
					enum_result.key_length =	llei16(	enum_attrA0_state.INDX_ptr+0xA);
					enum_result.flags =			llei16(	enum_attrA0_state.INDX_ptr+0xC);
					enum_result.reserved =		llei16(	enum_attrA0_state.INDX_ptr+0xE);
					if (enum_result.flags & CNTFS::reader::MFT::INDEX_ENTRY_END) {
						enum_result.data.dir.indexed_file =	0;
						enum_result.data.vi.data_offset =		llei16(	enum_attrA0_state.INDX_ptr+0x0);
						enum_result.data.vi.data_length =		llei16(	enum_attrA0_state.INDX_ptr+0x2);
						enum_result.data.vi.reservedV =			llei32(	enum_attrA0_state.INDX_ptr+0x4);
						enum_result.p_data =							enum_attrA0_state.INDX_ptr + enum_result.data.vi.data_offset;
						enum_result.p_data_fence =						enum_result.p_data + enum_result.data.vi.data_length;
					}
					else {
						enum_result.data.dir.indexed_file =		llei16(	enum_attrA0_state.INDX_ptr+0x0);
						enum_result.data.vi.data_offset =				0;
						enum_result.data.vi.data_length =				0;
						enum_result.data.vi.reservedV =					0;
						enum_result.p_data_fence =						NULL;
						enum_result.p_data =							NULL;
					}

					enum_result.key = enum_attrA0_state.INDX_ptr + 16;
					enum_result.key_fence = enum_result.key + enum_result.key_length;
					enum_attrA0_state.INDX_ptr += enum_result.length;
					enum_attrA0_state.index_header.index_counted += enum_result.length;

					if (enum_result.length < 0x10) {
						enum_attrA0_state.INDX_subchunk_N = enum_attrA0_state.INDX_subchunk_max;
						enum_attrA0_state.INDX_ptr = enum_attrA0_state.INDX_last;
						break;
					}

					// found an entry, terminate loop so caller can examine it
					limbo = 0;
					ret = 1;
				}
				else {
					enum_state = STATE_ATTR_SCAN;
					break;
				}
				break;

			default: // or STATE_FINISHED
				enum_state = STATE_FINISHED;
				limbo = 0;
				break;
		}
	}

	return ret;
}

int reader::MFT::NTFSIndexCrap::SetEnumerationType(uint32 typ)
{
	enum_type = typ;
	return 1;
}

reader::MFT::RecordReader::DirectorySearch::DirectorySearch(RecordReader *x)
{
	mom = x;
	nic = NULL;
	found_name = NULL;
}

reader::MFT::RecordReader::DirectorySearch::~DirectorySearch()
{
	if (found_name) delete found_name;
	found_name=NULL;
	if (nic) delete nic;
	nic=NULL;
}

int reader::MFT::RecordReader::DirectorySearch::FindFirst()
{
	if (MFTref.item_max == 0) return 0;
	MFTenum = 0;
	return FindNext();
}

int reader::MFT::RecordReader::DirectorySearch::FindNext()
{
	reader::MFT::RecordReader mft(mom->mom);
	char fname=0;

	if (MFTenum >= MFTref.item_max)
		return 0;

	if (!mft.Parse(MFT_current = *MFTref.item[MFTenum++]))
		return 0;
	if (!mft.AttrFirst())
		return 0;

	if (found_name) delete found_name;
	found_name=new AttrFilename;

	// this will return the best filename because Windows when
	// maintaining NTFS will always put the DOS name first followed
	// by the Win32 name, or else put it as one entry type Win32 and DOS.
	// However there are times when this doesn't happen, so we work
	// through a little kludge to make this work.
	do {
		if (mft.attr_cur.type == reader::MFT::AT_FILE_NAME &&
			mft.attr_cur.non_resident == 0) {
			unsigned char fnt = mft.attr_cur.attr_resident_value_ptr[65];

			if (fname) {
				// Posix names are the best
				if (found_name->file_name_type == reader::MFT::FNS_POSIX)
					continue;

				// if we already have a Win32 name we don't want any other
				if (found_name->file_name_type == reader::MFT::FNS_WIN32)
					if (fnt != reader::MFT::FNS_POSIX)
						continue;

				// if we already have a Win32 name we don't want any other
				if (found_name->file_name_type == reader::MFT::FNS_WIN32_AND_DOS)
					if (!(fnt == reader::MFT::FNS_POSIX || reader::MFT::FNS_WIN32))
						continue;
			}

			if (found_name->ParseName(&mft.attr_cur))
				fname=1;
		}
	} while (mft.AttrNext());

	if (!fname)
		return 0;

	return 1;
}

static int __cdecl sortMFTref(const void *elem1,const void *elem2)
{
	sint64 **s1 = (sint64**)elem1;
	sint64 **s2 = (sint64**)elem2;
	return (int)(**s1 - **s2);
}

int	reader::MFT::RecordReader::DirectorySearch::Gather()
{
	AttrFilename afa;
	sint64 *cluster;
	int i;

	if (found_name) delete found_name;
	found_name=NULL;
	if (nic) delete nic;
	nic=NULL;

	MFTref.Flush();
	if (!mom->AttrFirst()) return 0;
	nic = new NTFSIndexCrap(mom);
	if (!nic) return 0;
	nic->SetEnumerationType(reader::MFT::AT_FILE_NAME);
	if (!nic->FindFirst()) return 0;

	do {
		if (nic->enum_result.flags & reader::MFT::INDEX_ENTRY_END)
			continue;

		if (!afa.ParseName(nic->enum_result.key,(int)(nic->enum_result.key_fence - nic->enum_result.key)))
			continue;

		cluster = MFTref.Alloc();
		if (!cluster) return 0;
		*cluster = nic->enum_result.data.dir.indexed_file;
	} while (nic->FindNext());

	if (MFTref.item_max == 0)
		return 0;

	// sort the entries, then remove duplicates
	qsort(MFTref.item,MFTref.item_max,sizeof(sint64*),sortMFTref);
	for (i=0;i < (MFTref.item_max-1);)
		if (*MFTref.item[i] == *MFTref.item[i+1])
			MFTref.Free(MFTref.item[i+1]);
		else
			i++;

	return 1;
}

reader::MFT::NTFSFileData::NTFSFileData()
{
	nonresident_cache_cluster = -1;
	nonresident_cache=NULL;
	nonresident_map=NULL;
	resident_data_len=0;
	resident_data=NULL;
	got_data=0;
	mom=NULL;
}

reader::MFT::NTFSFileData::NTFSFileData(RecordReader *x)
{
	nonresident_cache_cluster = -1;
	nonresident_cache=NULL;
	nonresident_map=NULL;
	resident_data_len=0;
	resident_data=NULL;
	got_data=0;
	mom=x;
}

reader::MFT::NTFSFileData::~NTFSFileData()
{
	if (resident_data) delete resident_data;
	resident_data=NULL;
	if (nonresident_map) delete nonresident_map;
	nonresident_map=NULL;
	if (nonresident_cache) delete nonresident_cache;
	nonresident_cache=NULL;
}

int reader::MFT::NTFSFileData::read(unsigned char *buf,int len)
{
	uint64 rem;
	int rd=0;

	last_ent.Flush();

	if (!got_data) return 0;
	if (file_pointer >= file_pointer_max) return 0;
	rem = file_pointer_max - file_pointer;
	if (len > rem) len = rem;

	if (nonresident_map) {
		while (len > 0) {
			uint64 foc = file_pointer / cluster_size;
			int foca = (int)(file_pointer % cluster_size);
			int fsz = len;

			if (fsz > (cluster_size-foca))
				fsz =  cluster_size-foca;

			if (nonresident_cache_cluster == foc) {
				memcpy(buf,nonresident_cache + foca,fsz);
				file_pointer += fsz;
				buf += fsz;
				len -= fsz;
				rd += fsz;
			}
			else {
				reader::MFT::NTFSVCNRunlist::ent* ent = last_ent.Alloc();
				nonresident_cache_cluster = -1;
				uint64 clusty = nonresident_map->Map(foc,ent);
				if (clusty == ((uint64)-1)) {
					if (ent->lcn == reader::MFT::NTFSVCNRunlist::LCN_HOLE) {
						memset(buf,0,fsz);
						file_pointer += fsz;
						buf += fsz;
						len -= fsz;
						rd += fsz;
						continue;
					}
					else {
						break;
					}
				}

				unsigned char *bufferia = mom->mom->ReadCluster(clusty);
				if (!bufferia) break;
				memcpy(nonresident_cache,bufferia,cluster_size);
				nonresident_cache_cluster = foc;
				memcpy(buf,nonresident_cache + foca,fsz);
				file_pointer += fsz;
				buf += fsz;
				len -= fsz;
				rd += fsz;
			}
		}

		if (len < 0)
			len = len;
	}
	else {
		memcpy(buf,resident_data + file_pointer,len);
		file_pointer += len;
		buf += len;
		rd = len;
	}

	return rd;
}

uint64 reader::MFT::NTFSFileData::seek(uint64 ofs)
{
	if (!got_data) return 0;
	if (ofs > file_pointer_max) ofs = file_pointer_max;
	file_pointer = ofs;

	if (nonresident_map) {
		uint64 foc = file_pointer / cluster_size;
		if (foc != nonresident_cache_cluster) 
			nonresident_cache_cluster = -1;
	}
	else {
		// nothing to do for resident data
	}

	return file_pointer;
}

char reader::MFT::NTFSFileData::NeedInterpreter()
{
	if (attributes & reader::MFT::ATTR_IS_COMPRESSED)
		return 1;
	if (attributes & reader::MFT::ATTR_IS_ENCRYPTED)
		return 1;

	return 0;
}

int reader::MFT::NTFSFileData::SetupData()
{
	if (resident_data) delete resident_data;
	resident_data=NULL;
	if (nonresident_map) delete nonresident_map;
	nonresident_map=NULL;
	nonresident_cache_cluster = -1;
	if (nonresident_cache) delete nonresident_cache;
	nonresident_cache=NULL;

	if (mom->AttrFirst()) {
		do {
			if (mom->attr_cur.type == reader::MFT::AT_DATA) {
				compression_unit = mom->attr_cur.data.nonresident.compression_unit;
				attributes = mom->attr_cur.flags;

				if (mom->attr_cur.non_resident) {
					nonresident_map = new NTFSVCNRunlist(mom->mom);
					if (!nonresident_map) return 0;
					if (!nonresident_map->Parse(&mom->attr_cur)) return 0;

					file_pointer = 0;
					file_pointer_max = 0;
					cluster_size = mom->mom->SectorsPerCluster * 512;
					nonresident_cache = new unsigned char[cluster_size];
					if (!nonresident_cache) return 0;
					nonresident_cache_cluster = -1;
					got_data = 1;

					if (nonresident_map->list_max > 0) {
//						file_pointer_max  = nonresident_map->list[nonresident_map->list_max - 1].vcn + nonresident_map->list[nonresident_map->list_max - 1].len;
//						if (file_pointer_max > 0) file_pointer_max--;
//						file_pointer_max *= cluster_size;
//						if (file_pointer_max < mom->attr_cur.data.nonresident.data_size)
							file_pointer_max = mom->attr_cur.data.nonresident.data_size;
					}

					return 1;
				}
				else {
					/* resident data can be cached and held in memory */
					resident_data_len = (int)(mom->attr_cur.attr_resident_value_ptr_fence - mom->attr_cur.attr_resident_value_ptr);
					if (resident_data_len > 0) {
						resident_data = new unsigned char[resident_data_len];
						if (!resident_data) return 0;
						memcpy(resident_data,mom->attr_cur.attr_resident_value_ptr,resident_data_len);
					}

					file_pointer = 0;
					file_pointer_max = resident_data_len;
					got_data = 1;
					return 1;
				}
			}
		} while (mom->AttrNext());
	}

	return 0;
}

reader::MFT::NTFSFileDataCompressed::NTFSFileDataCompressed(NTFSFileData *src)
{
	compression_unit_in = NULL;
	compression_unit_out = NULL;
	source = src;
}

reader::MFT::NTFSFileDataCompressed::~NTFSFileDataCompressed()
{
	if (compression_unit_in) delete compression_unit_in;
	compression_unit_in = NULL;
	if (compression_unit_out) delete compression_unit_out;
	compression_unit_out = NULL;
}

int	reader::MFT::NTFSFileDataCompressed::SetupData()
{
	if (compression_unit_in) delete compression_unit_in;
	compression_unit_in = NULL;
	if (compression_unit_out) delete compression_unit_out;
	compression_unit_out = NULL;

	if (!source) return 0;
	if (!(source->attributes & reader::MFT::ATTR_IS_COMPRESSED)) return 0;
	if (!source->nonresident_map) return 0;
	compression_unit = source->compression_unit;
	compression_unit_size = (1 << compression_unit) * source->cluster_size;

	// apparently compression does not like blocks >= 64KB
	if (compression_unit_size >= 65536) return 0;

	compression_unit_in = new unsigned char[compression_unit_size];
	if (!compression_unit_in) return 0;
	compression_unit_out = new unsigned char[compression_unit_size];
	if (!compression_unit_out) return 0;

	file_pointer_max = source->file_pointer_max;
	file_pointer = 0;
	return 1;
}

uint64 reader::MFT::NTFSFileDataCompressed::seek(uint64 ofs)
{
	if (!got_data) return 0;
	if (ofs > file_pointer_max) ofs = file_pointer_max;
	file_pointer = ofs;
	return file_pointer;
}

static unsigned char *NTFSDecompress(unsigned char *in,unsigned char *out,int sz)
{
	unsigned char *inf = in + sz;
	unsigned char *outf = out + sz;

	unsigned char *debugfence = out + (sz>>1);

	while (in <= (inf - 6)) {
		int sbhdr = llei16(in);
		int sblen = sbhdr & 0xFFF;
		unsigned char *sbfence = in + sblen + 3;
		unsigned char *sbstart;

		if (out >= debugfence)
			out = out;

		if (sbfence > inf)
			return out;

		/* if not compressed, just copy */
		if (!(sbhdr & 0x8000)) {
			in += 2;
			if ((in+0x1000) > sbfence || (out+0x1000) > outf)
				return out;
			if ((sbfence-in) != 0x1000)
				return out;
			memcpy(out,in,0x1000);
			in += 0x1000;
			out += 0x1000;
		}
		/* if compressed... */
		else {
			unsigned char *oute = out + 0x1000;
			if (oute > outf)
				return out;

			in += 2;
			sbstart = out;

			do {
				if (in >= sbfence) {
					out = oute;
					break;
				}

				if (in > sbfence || out > oute)
					return out;

				unsigned char tag = *in++;
				int token;
				for (token=0;token < 8;token++,tag >>= 1) {
					unsigned int lg,pt,len,mno;
					unsigned char *destba;
					int i;

					if (in >= sbfence || out > oute)
						break;

					if (!(tag & 1)) {	// symbol token
						*out++ = *in++;
						continue;
					}

					// phrase token
					if (out == sbstart)
						return out;

					lg = 0;
					for (i = ((int)(out - sbstart)) - 1;i >= 0x10;i >>= 1)
						lg++;

					pt = llei16(in); in += 2;
					destba = (out - (pt >> (12 - lg))) - 1;
					if (destba < sbstart)
						return out;
					len = (pt & (0xfff >> lg)) + 3;

					if ((out+len) > oute)
						return out;

					mno = out - destba;
					// apparently the copy operation relies on byte-by-byte copying?
					if (len <= mno) {
						memcpy(out,destba,len);
						out += len;
					}
					else {
						memcpy(out,destba,mno);
						destba += mno;
						out += mno;
						len -= mno;
						while (len--) *out++ = *destba++;
					}
				}
			} while (1);
		}

		in = sbfence;
	}
}

int	reader::MFT::NTFSFileDataCompressed::read(unsigned char *buf,int len)
{
	int rd=0;

	if (file_pointer >= file_pointer_max)
		return 0;

	while (len > 0) {
		uint64 maxrd = file_pointer_max - file_pointer;
		sint64 cblock = file_pointer / compression_unit_size;
		int cblocko = file_pointer % compression_unit_size;
		int cblocksz = compression_unit_size - cblocko;

		if (cblocksz > len)
			cblocksz = len;
		if (cblocksz > maxrd)
			cblocksz = maxrd;
		if (cblocksz == 0)
			break;

		if (cblock == last_cblock) {
			memcpy(buf,compression_unit_out + cblocko,cblocksz);
			file_pointer += cblocksz;
			len -= cblocksz;
			buf += cblocksz;
			rd += cblocksz;
		}
		else {
			char compress=0;
			sint64 ofo = cblock * compression_unit_size;
			if (source->seek(ofo) != ofo)
				return rd;
			memset(compression_unit_in,0,compression_unit_size);
			if (cblock == 13)
				file_pointer = file_pointer;
			int ord = source->read(compression_unit_in,compression_unit_size);
			if (ord < compression_unit_size)
				memset(compression_unit_in+ord,0,compression_unit_size-ord);

			/* detect non-compression */
			if (source->last_ent.item_max > 0) {
				NTFSVCNRunlist::ent* first = source->last_ent.item[0];
				NTFSVCNRunlist::ent* second = NULL;

				if (source->last_ent.item_max >= 2) {
					int idx = source->last_ent.item_max - 1;
					while (idx > 1 && source->last_ent.item[idx]->lcn >= 0) idx--;
					second = source->last_ent.item[idx];
				}

				if (first && second) {
					sint64 curv = (cblock * (1 << compression_unit)) - first->vcn;
					if (curv >= 0) {
						if (first->lcn >= 0 && first->len > 0) {
							if (second->len > 0 && second->lcn == reader::MFT::NTFSVCNRunlist::LCN_HOLE) {
								if ((first->vcn % (1 << compression_unit)) == 0) {
									if (((second->vcn + second->len) % (1 << compression_unit)) == 0) {
										if ((first->vcn + first->len) == second->vcn) {
											if ((curv % (1 << compression_unit)) == 0) {
												if (((curv + first->vcn) % (1 << compression_unit)) == 0) {
													if ((second->vcn % (1 << compression_unit)) != 0) {
														compress = 1;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

			if (compress) {
				memset(compression_unit_out,0,compression_unit_size);
				unsigned char *stop = NTFSDecompress(compression_unit_in,compression_unit_out,compression_unit_size);
				int outsz = (int)(stop - compression_unit_out);
				if (outsz < compression_unit_size)
					outsz = outsz;
			}
			else {
				if (ord < compression_unit_size)
					ord = ord;
				memcpy(compression_unit_out,compression_unit_in,compression_unit_size);
			}

			memcpy(buf,compression_unit_out + cblocko,cblocksz);
			file_pointer += cblocksz;
			last_cblock = cblock;
			len -= cblocksz;
			buf += cblocksz;
			rd += cblocksz;
		}
	}

	return rd;
}

char reader::MFT::NTFSFileDataCompressed::NeedInterpreter()
{
	return 0;
}

////////////////////////////////////////////////
};
// END CNTFS namespace
