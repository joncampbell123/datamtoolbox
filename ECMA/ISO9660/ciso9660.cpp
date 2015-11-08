
/* a C++ class for reading the Linux ext2/ext3 filesystem
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 */

#include "CISO9660.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int IsBidir16Valid(CISO9660::reader::BiDir16 *b)
{
	return (llei16(&b->le) == lbei16(&b->be));
}

static int IsBidir32Valid(CISO9660::reader::BiDir32 *b)
{
	return (llei32(&b->le) == lbei32(&b->be));
}

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

// BEGIN CISO9660 namespace
namespace CISO9660 {
////////////////////////////////////////////////

int reader::init()
{
	mounted=0;
	is_joliet=0;
	TotalVolDescriptors=0;
	return 1;
}

int	reader::ChoosePrimary()
{
	VolumeDescriptor v;
	int x=0;

	x=0;
	is_joliet=0;
	if (!ReadVolDescriptor(x,&v)) return 0;
	while (!memcmp(v.identifier,"CD001",5) && v.type != 1 && x < TotalVolDescriptors) {
		x++;
		if (!ReadVolDescriptor(x,&v)) break;
	}
	VD_Current_2ndary_data = NULL;
	memcpy(&VD_Current,&v,sizeof(VolumeDescriptor));
	VD_Current_data=(PrimaryVolumeData*)(&VD_Current.data);
	VD_Current_valid=1;
	return 1;
}

// returns 1 if Joliet extensions are present.
int	reader::CheckJoliet(int N)
{
	int x=0;

	while (ChooseSecondary(x++)) {
		// make sure secondary data is there
		if (!VD_Current_2ndary_data)
			continue;

		// make sure UCS-2 escape sequences are there
		{
			unsigned char *esc = VD_Current_2ndary_data->Escapes;
			char good=0;
			if (esc[0] == '%' && esc[1] == '/')
				if (esc[2] == '@' || esc[2] == 'C' || esc[2] == 'E')
					good=1;

			if (!good)
				continue;
		}

		// it works
		if (N-- == 0) {
			is_joliet=1;
			return 1;
		}
	}

	return 0;
}

int	reader::ChooseSecondary(int N)
{
	VolumeDescriptor v;
	int x=0;

	x=0;
	is_joliet=0;
	if (!ReadVolDescriptor(x,&v)) return 0;
	while (!memcmp(v.identifier,"CD001",5) && x < TotalVolDescriptors) {
		if (v.type == 2) {
			if (N-- == 0) {
				memcpy(&VD_Current,&v,sizeof(VolumeDescriptor));
				VD_Current_data=(PrimaryVolumeData*)(&VD_Current.data);
				VD_Current_2ndary_data=(SecondaryVolumeData*)(&VD_Current.data);
				VD_Current_valid=1;
				return 1;
			}
		}

		x++;
		if (!ReadVolDescriptor(x,&v)) break;
	}
	return 0;
}

int reader::ReadVolDescriptor(int N,VolumeDescriptor *vd)
{
	unsigned char buf[2048];

	if (!blockread(N+16,1,buf))
		return 0;

	vd->type =				buf[	0];
	memcpy(vd->identifier,	buf +	1,5);
	vd->version =			buf[	6];
	memcpy(vd->data,		buf +	7,2041);
	return 1;
}

int reader::freeMe()
{
	if (mounted) umount();
	return 1;
}

int reader::mount()
{
	VolumeDescriptor v;

	VD_Current_data = NULL;
	if (sizeof(PrimaryVolumeData) != 2041) {
		comment(C_ERROR,"C compiler error!");
		return 0;
	}

	if (mounted) {
		comment(C_ERROR,"Already mounted!");
		return 0;
	}

	sectsize=blocksize();
	if (sectsize != 2048) {
		comment(C_ERROR,"Block sizes != 2048 not supported yet!");
		return 0;
	}

	is_joliet=0;
	VD_Current_valid=0;
	if (!ReadVolDescriptor(0,&v)) return 0;

	// ideally the first entry should be a CD001 Primary Vol Descriptor
	if (v.type > 1) return 1;
	while (!memcmp(v.identifier,"CD001",5) && v.type != 0xFF && TotalVolDescriptors < 32) {
		if (v.type == 1 && !VD_Current_valid) {
			memcpy(&VD_Current,&v,sizeof(VolumeDescriptor));
			VD_Current_data=(PrimaryVolumeData*)(&VD_Current.data);
			VD_Current_valid=1;
		}

		TotalVolDescriptors++;
		if (!ReadVolDescriptor(TotalVolDescriptors,&v)) break;
	}
	VD_Current_2ndary_data = NULL;
	if (v.type == 0xFF && !memcmp(v.identifier,"CD001",5))
		TotalVolDescriptors++;

	// did we find a primary volume descriptor?
	if (!VD_Current_valid)
		return 0;

	// sanity checks of data
	if (!IsBidir16Valid(&VD_Current_data->Logical_Block_Size)) {
		// decide which is more sane
		char b1 = IsPow2(llei16(&VD_Current_data->Logical_Block_Size.le));
		char b2 = IsPow2(lbei16(&VD_Current_data->Logical_Block_Size.be));

		if (!b1 && !b2)
			return 0;
		if (b1)
			sbei16(&VD_Current_data->Logical_Block_Size.be,
				llei16(&VD_Current_data->Logical_Block_Size.le));
		else if (b2)
			slei16(&VD_Current_data->Logical_Block_Size.le,
				lbei16(&VD_Current_data->Logical_Block_Size.be));
	}

	// I've never seen a CD-ROM ISO filesystem that has a Logical Block Size
	// != 2048 so let's assume for now they don't exist
	if (llei16(&VD_Current_data->Logical_Block_Size.le) != 2048)
		return 0;

	// sanity checks for the root directory record
	if (VD_Current_data->DirectoryRecord_for_RootDir.x.dr.Length_of_Directory_Record < 33)
		return 0;
	if (VD_Current_data->DirectoryRecord_for_RootDir.x.dr.Length_of_File_Ident != 0)
		comment(C_WARNING,"Root directory has a name??");
	if (!(VD_Current_data->DirectoryRecord_for_RootDir.x.dr.File_Flags & Sec91_DirRecord_File_Flags_Directory))
		comment(C_WARNING,"Root directory is in denial that it is a directory");
	if (VD_Current_data->DirectoryRecord_for_RootDir.x.dr.File_Flags & Sec91_DirRecord_File_Flags_MultiExtend)
		comment(C_WARNING,"Multi-extent root directory, which is impossible");

	mounted=1;
	return 1;
}

int reader::umount()
{
	if (!mounted) {
		comment(C_ERROR,"Already not mounted!");
		return 0;
	}

	mounted=0;
	return 1;
}

reader::readdirents::readdirents(reader *x)
{
	list.Flush();
	blockalign_mode=0;
	extent_i = -1;
	buffer_N = -1;
	file_size = 0;
	lockdown = 0;
	mom = x;
}

int reader::readdirents::finish()
{
	if (lockdown) return 1;
	if (list.item_max == 0) return 0;

	file_size = 0;
	if (list.item_max == 1) {
		Sec91_DirRecord *r = list.item[0];
		file_size = llei32(&r->x.dr.Data_Length.le);
	}
	else {
		Sec91_DirRecord *r;
		int i;

		for (i=0;i < list.item_max;i++) {
			r = list.item[i];
			file_size += llei32(&r->x.dr.Data_Length.le);
		}
	}

	extent_i = -1;
	buffer_N = -1;
	file_pointer=0;
	blockalign_mode=0;
	lockdown=1;
	if (seek(0) != 0) return 0;
	return 1;
}

void reader::readdirents::setblockalign(char c)
{
	if (!lockdown) return;
	blockalign_mode=c;

	file_size = 0;
	if (list.item_max == 1) {
		Sec91_DirRecord *r = list.item[0];
		file_size = llei32(&r->x.dr.Data_Length.le);
		if (blockalign_mode && file_size & 2047) file_size = (file_size|2047)+1;
	}
	else {
		Sec91_DirRecord *r;
		int i;

		for (i=0;i < list.item_max;i++) {
			r = list.item[i];
			file_size += llei32(&r->x.dr.Data_Length.le);
			if (blockalign_mode && file_size & 2047) file_size = (file_size|2047)+1;
		}
	}

	seek(0);
}

uint64 reader::readdirents::seek(uint64 o)
{
	if (!lockdown) return 0;
	if (o > file_size) o = file_size;
	extent_ofs = 0;
	extent_i = -1;
	buffer_N = -1;
	file_pointer = o;
	return o;
}

int reader::readdirents::read(unsigned char *buf,int len)
{
	int rd=0;

	if (!lockdown) return 0;

	while (len > 0) {
		if (extent_i != -1)
			if (extent_i < 0 || extent_i >= list.item_max)
				extent_i = -1;

		if (extent_i != -1)
			if (file_pointer < extent_ofs)
				extent_i = -1;

		if (extent_i != -1)
			if (list.item[extent_i] != NULL && file_pointer >= (extent_ofs + llei32(&list.item[extent_i]->x.dr.Data_Length.le)))
				extent_i = -1;

		if (extent_i == -1) {
			uint64 offo = file_pointer;
			int j;

			extent_ofs = 0;
			for (j=0;j < list.item_max;j++) {
				Sec91_DirRecord *r = list.item[j];
				if (offo >= llei32(&r->x.dr.Data_Length.le)) {
					extent_ofs += llei32(&r->x.dr.Data_Length.le);
					offo -= llei32(&r->x.dr.Data_Length.le);
				}
				else {
					break;
				}
			}

			if ((extent_i = j) >= list.item_max)
				return rd;
		}

		if (extent_i < 0)
			return rd;

		uint64 filerem = file_size - file_pointer;
		uint64 exto = file_pointer - extent_ofs;
		uint64 extsz = llei32(&list.item[extent_i]->x.dr.Data_Length.le);
		if (blockalign_mode && (extsz & 2047) != 0) extsz = (extsz|2047)+1;
		extsz -= exto;
		int secto = exto & 2047;
		int secsz = 2048 - secto;
		int sz = len;

		if (sz > extsz)
			sz = extsz;
		if (sz > secsz)
			sz = secsz;
		if (sz > len)
			sz = len;
		if (sz == 0)
			return rd;

		uint64 sector = llei32(&list.item[extent_i]->x.dr.Loc_of_Extent.le) + (exto >> 11);
		if (buffer_N != -1 && sector == buffer_N) {
			memcpy(buf,buffer+secto,sz);
			file_pointer += sz;
			buf += sz;
			len -= sz;
			rd += sz;
		}
		else {
			buffer_N = sector;
			if (!mom->blockread(sector,1,buffer))
				memset(buffer,0,2048);

			memcpy(buf,buffer+secto,sz);
			file_pointer += sz;
			buf += sz;
			len -= sz;
			rd += sz;
		}
	}

	return rd;
}

int reader::readdirents::add(Sec91_DirRecord* d)
{
	if (lockdown) return 0;

	// bitch about features we don't support
	if (d->x.dr.File_Flags & Sec91_DirRecord_File_Flags_MultiExtend) {
		mom->comment(reader::C_ERROR,"Multi-extent ISO files not supported");
		return 0;
	}

	if (d->x.dr.Interleave_Gap_Size != 0 ||
		d->x.dr.File_Unit_Size != 0) {
		mom->comment(reader::C_ERROR,"Interleaved files not supported");
		return 0;
	}

	// based on insightful comments in Linux ISOFS code, check for
	// dipshits who put unrelated data in the uppermost byte (cruft).
	// while we're at it check the validity of the data length against
	// the extent.
	{
		uint32 lel = llei32(&d->x.dr.Data_Length.le);
		uint32 bel = lbei32(&d->x.dr.Data_Length.be);
		uint32 size;

		// check upper byte vs. upper byte and look for this cruft.
		// don't blindly assume that it's cruft 
		if ((lel & 0xFF000000) && !(bel & 0xFF000000)) {
			mom->comment(reader::C_WARNING,"Possible cruft in little endian side of ISO dir data size");
			lel &= 0xFFFFFF;
		}
		else if (!(lel & 0xFF000000) && (bel & 0xFF000000)) {
			mom->comment(reader::C_WARNING,"Possible cruft in big endian side of ISO dir data size");
			bel &= 0xFFFFFF;
		}

		// in case one field was set to zero and the other wasnt?
		if (lel == 0 && bel > 0) {
			mom->comment(reader::C_WARNING,"Little endian ISO dir data size is 0, using big endian data size");
			bel = lel;
		}
		else if (bel == 0 && lel > 0) {
			mom->comment(reader::C_WARNING,"Big endian ISO dir data size is 0, using little endian data size");
			lel = bel;
		}

		// finally, decide based on size (which is smallest?)
		if (lel > bel) {
			mom->comment(reader::C_WARNING,"ISO dir data size issue: LE size > BE size using LE size");
			size = lel;
		}
		else if (lel < bel) {
			mom->comment(reader::C_WARNING,"ISO dir data size issue: BE size > LE size using BE size");
			size = bel;
		}
		else {
			size = lel;
		}

		// that will be the official size used in this code
		slei32(&d->x.dr.Data_Length.le,size);
		sbei32(&d->x.dr.Data_Length.be,size);
	}

	Sec91_DirRecord *nd = list.Alloc();
	if (!nd) return 0;
	memcpy(nd,d,sizeof(Sec91_DirRecord));
	return 1;
}

reader::readdirents* reader::GetRootDir()
{
	if (!VD_Current_data) return NULL;
	reader::readdirents* n = new reader::readdirents(this);
	n->add(&VD_Current_data->DirectoryRecord_for_RootDir);
	n->finish();
	return n;
}

reader::readdir::readdir(readdirents *d)
{
	mom = d;
	current = NULL;
	bufferi = NULL;
	bufferfence = NULL;
	mom->setblockalign(1);
}

int reader::readdir::first()
{
	bufferi = NULL;
	bufferfence = buffer + 2048;
	current = NULL;
	offset = 0;
	return next();
}

int reader::readdir::next()
{
again:
	if (bufferi == NULL) {
		bufferi = buffer;
		if (mom->seek(offset) != offset)
			return 0;
		offset += 2048;
		memset(buffer,0,2048);
		if (mom->read(buffer,2048) < 2048)
			return 0;
	}

	current = (Sec91_DirRecord*)bufferi;
	if (current->x.dr.Length_of_Directory_Record < 33 ||
		(bufferi+current->x.dr.Length_of_Directory_Record) > bufferfence) {
		bufferi = NULL;
		goto again;
	}

	file_version = NULL;
	fullname_is_ucs = mom->mom->is_joliet;
	int l = current->x.dr.Length_of_File_Ident;
	int n = current->x.dr.Length_of_Directory_Record - 33;
	if (l > n) l = n;
	fullnamel = l;
	if (fullnamel > 0) memcpy(fullname,((unsigned char*)current) + 33,fullnamel);
	fullname[fullnamel] = 0;

	if (fullname_is_ucs) {
		int i;

		for (i=0;i < fullnamel;i += 2) {
			if (fullname[i] != 0)
				name[i>>1] = '?';
			else
				name[i>>1] = fullname[i+1];
		}

		name[i>>1] = 0;
	}
	else {
		memcpy(name,fullname,256);
	}

	file_version = (unsigned char*)strchr((char*)name,';');
	if (file_version) *file_version++ = 0;
	bufferi += current->x.dr.Length_of_Directory_Record;
	return 1;
}

reader::RockRidge::RockRidge(reader* r)
{
	NM_buffer = NULL;
	record = NULL;
	mom = r;
}

int reader::RockRidge::Check(Sec91_DirRecord *r)
{
	unsigned char *beg,*end;
	list.Flush();
	record = r;

	if (NM_buffer) delete NM_buffer;
	NM_buffer=NULL;

	beg = r->x.raw + 33 + r->x.dr.Length_of_File_Ident;
	if (!(r->x.dr.Length_of_File_Ident & 1)) beg++;
	end = r->x.raw + r->x.dr.Length_of_Directory_Record;

	/* look for Rock Ridge */
	if (memcmp(beg,"RR",2)) {
		beg++;
		// in case of an off-by-one error...
		if (memcmp(beg,"RR",2)) {
			return 0;
		}
	}

	field *fiel = list.Alloc();
	if (!fiel) return 0;

	fiel->start = beg;
	fiel->len = beg[2];
	fiel->version = beg[3];
	fiel->flags = beg[4];
	fiel->end = fiel->start + fiel->len;
	fiel->data = fiel->start + 5;
	beg += fiel->len;

	// give warning about unusual RR blocks
	if (fiel->len < 5) {
		mom->comment(C_ERROR,"Found Rock Ridge in System Use Field but length < 3!");
		return 0;
	}
	else if (fiel->len > 5) {
		mom->comment(C_WARNING,"Found Rock Ridge field with length > 3 (%u)!",fiel->len);
	}
	else if (fiel->version != 1) {
		mom->comment(C_WARNING,"Found Rock Ridge field with version != 1 (%u)!",fiel->version);
	}

	while (fiel->len >= 5 && fiel->end <= end && fiel->start < end) {
		fiel = list.Alloc();
		if (!fiel) return 0;

		fiel->start = beg;
		fiel->len = beg[2];
		fiel->version = beg[3];
		fiel->flags = beg[4];
		fiel->end = fiel->start + fiel->len;
		fiel->data = fiel->start + 5;
		beg += fiel->len;
	}

	return 1;
}

reader::RockRidge::~RockRidge()
{
	if (NM_buffer) delete NM_buffer;
}

char* reader::RockRidge::GetName()
{
	field *f=NULL;
	int i;

	if (NM_buffer) delete NM_buffer;
	NM_buffer=NULL;

	for (i=0;i < list.item_max && f == NULL;i++)
		if (!memcmp(list.item[i]->start,"NM",2))
			f = list.item[i];

	if (!f)
		return NULL;

	int len = (int)(f->end - f->data);
	if (len <= 0)
		return NULL;

	NM_buffer = new char[len+1];
	if (!NM_buffer) return NULL;
	memcpy(NM_buffer,f->data,len);
	NM_buffer[len] = 0;
	return NM_buffer;
}

int	reader::RockRidge::IsRelocatedDir()
{
	field *f=NULL;
	int i;

	for (i=0;i < list.item_max && f == NULL;i++)
		if (!memcmp(list.item[i]->start,"RE",2) ||
			!memcmp(list.item[i]->start,"PL",2))
			f = list.item[i];

	if (!f)
		return 0;

	return 1;
}

uint32 reader::RockRidge::GetDirRelocation()
{
	field *f=NULL;
	uint32 le,be;
	int i;

	for (i=0;i < list.item_max && f == NULL;i++)
		if (!memcmp(list.item[i]->start,"CL",2))
			f = list.item[i];

	if (!f)
		return 0;

	le = llei32(f->start+4);
	be = lbei32(f->start+8);
	if (le != be)
		return 0;

	return le;
}

// when first called by caller:
// rr =             the readdirents object to add directory entries too
// root =           the starting directory to start scanning from
// parent =         sector of parent directory (0 for the actual root directory)
// ofs =            sector number to match directory extents against
int reader::RockRidge::FindByMatchingGraftOffset(reader::readdirents *rr,reader::readdirents *root,uint32 parent,uint32 ofs)
{
	CISO9660::reader::readdir rd(root);
	static int match_indent=0;
	int match=0;

	if (match_indent > 80)
		return 0;

	if (rd.first()) {
		match_indent++;
		do {
			// be careful of the fact that on all ISOs I test the first two entries
			// are '.' and '..' respectively which point to the directory itself
			// and it's parent directory
			if (llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le) != llei32(&rd.current->x.dr.Loc_of_Extent.le) &&
				parent != llei32(&rd.current->x.dr.Loc_of_Extent.le) &&
				(rd.current->x.dr.File_Flags & CISO9660::reader::Sec91_DirRecord_File_Flags_Directory)) {
				CISO9660::reader::RockRidge rocky(root->mom);
				char graft=0;

				if (rocky.Check(rd.current))
					if (rocky.IsRelocatedDir())
						graft=1;

				if (!graft) {
					CISO9660::reader::readdirents r(root->mom);
					r.add(rd.current);
					r.finish();
					r.seek(0);
					if (FindByMatchingGraftOffset(rr,&r,llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le),ofs))
						match = 1;
				}
				else {
					uint32 of = llei32(&rd.current->x.dr.Loc_of_Extent.le);
					uint32 sz = llei32(&rd.current->x.dr.Data_Length.le);
					sz = (sz+2047)>>11;
					if (ofs >= of && ofs < (of+sz)) {
						if (ofs != of)
							mom->comment(C_WARNING,"Rock Ridge directory relocation point does not point to the beginning of the directory extent!");

						rr->add(rd.current);
						match = 1;
					}
				}
			}
		} while (rd.next());
		match_indent--;
	}

	return match;
}

int reader::RockRidge::IsSparse()
{
	field *f=NULL;
	int i;

	for (i=0;i < list.item_max && f == NULL;i++)
		if (!memcmp(list.item[i]->start,"SF",2))
			f = list.item[i];

	if (!f)
		return 0;

	return 1;
}

////////////////////////////////////////////////
};
// END CISO9660 namespace
