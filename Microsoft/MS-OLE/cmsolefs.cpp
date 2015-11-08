
/* a C++ class for reading the Microsoft OLE container file structure,
 *
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 */

#include "common/Types.h"
#include "Microsoft/MS-OLE/cmsolefs.h"
#include <string.h>
#include <stdlib.h>

static unsigned char OLE_signature[8] = {0xD0,0xCF,0x11,0xE0,0xA1,0xB1,0x1A,0xE1};

// BEGIN CMSOLEFS namespace
namespace CMSOLEFS {
////////////////////////////////////////////////

#define CMSOLEFS_GETWORD(x)		((unsigned short)llei16((void*)(x)))
#define CMSOLEFS_GETDWORD(x)	((unsigned short)llei32((void*)(x)))

int reader::init()
{
	mounted=0;
	sbd_ent=NULL;
	root_entry_ent=NULL;
	return 1;
}

int reader::free()
{
	if (mounted) umount();
	return 1;
}

int reader::GetHeader()
{
	if (memcmp(tmpsect,OLE_signature,8)) {
		comment(C_ERROR,"OLE signature not found");
		return 0;
	}

	num_blocks =			disksize()+1;
	num_bbd_blocks =		CMSOLEFS_GETDWORD(tmpsect+0x02c);
	root_block =			CMSOLEFS_GETDWORD(tmpsect+0x030);
	sbd_1st =				CMSOLEFS_GETDWORD(tmpsect+0x03c);

	return 1;
}

reader::entity* reader::root_entry()
{
	entity::DIRENTRY f;
	entity *a,*b;

	a=rootdirectory();
	if (!a) return NULL;
	if (!a->dir_findfirst(&f)) return NULL;

	do {
		if (!strcmpi((char*)f.ps_name,"Root Entry")) {
			b=a->getdirentdata(&f);
			if (b) {
				delete a;
				return b;
			}
		}
	} while (a->dir_findnext(&f));

	return NULL;
}

int reader::mount()
{
	if (mounted) {
		comment(C_ERROR,"Already mounted!");
		return 0;
	}

	/* The OLE container deals with 512-byte "blocks", essentially making
	   a filesystem WITHIN A FILE! And it's being used to hold documents?!?!? */
	sectsize=blocksize();
	if (sectsize != 512) {
		comment(C_ERROR,"Bad sector size!");
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

	if (!GetHeader()) {
		comment(C_ERROR,"This is not a Microsoft OLE container!");
		umount();
		return 0;
	}

	sbd_ent=NULL;
	root_entry_ent=root_entry();
	return 1;
}

int reader::umount()
{
	if (!mounted) {
		comment(C_ERROR,"Already not mounted!");
		return 0;
	}

	if (sbd_ent) delete sbd_ent;
	sbd_ent=NULL;
	if (root_entry_ent) delete root_entry_ent;
	root_entry_ent=NULL;
	delete tmpsect;
	tmpsect=NULL;
	mounted=0;
	return 1;
}

reader::entity::entity(reader *parent)
{
	pps_array=NULL;
	pps_array_size=0;
	pps_array_alloc=0;
	mom=parent;
}

reader::entity::~entity()
{
	::free(pps_array);
	pps_array=NULL;
}

unsigned long reader::entity::tell()
{
	if (pointer < adjust) return 0;
	return pointer - adjust;
}

unsigned long reader::entity::seek(unsigned long pos)
{
	unsigned long t1;

	pos += adjust;
	if (size > 0) {
		if (pos > size)		pointer = pos = size;
		else			pointer = pos;
	}
	else {
		pointer = pos;
	}

	if (starting_block >= mom->num_blocks)		pointer = pos = 0;
	if (use_small_blocks)				t1 = pointer & (~0x03F);
	else						t1 = pointer & (~0x1FF);

	if (t1 < current_block_offset) {
		current_block = starting_block;
		current_block_offset = 0;
	}

	while (current_block_offset < t1 && current_block < mom->num_blocks) {
		if (use_small_blocks)	current_block = mom->ReadSBD(current_block);
		else			current_block = mom->ReadBBD(current_block);
		current_block_offset += block_size;
	}

	if (current_block_offset != t1)
		pointer = current_block_offset;

	return tell();
}

int reader::entity::read(unsigned char *buf,int sz)
{
	unsigned long fo,rem;
	int rd=0,br,bo;

	while (sz > 0) {
		if (current_block >= 0xFFFFFFFD) return rd;
		bo = pointer - current_block_offset;
		br = block_size - bo;
		if (size > 0) {
			rem = size - pointer;
			if (br > rem) br = rem;
		}
		if (br <= 0) return rd;
		if (br > sz) br = sz;


		if (use_small_blocks) {
			unsigned char tmp[64];

			fo = current_block * block_size;
			if (!mom->root_entry_ent) return rd;
			if (mom->root_entry_ent->seek(fo) != fo) return rd;
			if (mom->root_entry_ent->read(tmp,block_size) < block_size) return rd;
			memcpy(mom->tmpsect,tmp,64);
		}
		else {
			if (!mom->ReadAbsBlock(current_block,1,mom->tmpsect)) return rd;
		}

		pointer += br;
		memcpy(buf,mom->tmpsect+bo,br);
		buf += br;
		rd += br;
		sz -= br;

		if (pointer >= (current_block_offset+block_size)) {
			if (use_small_blocks)	current_block = mom->ReadSBD(current_block);
			else					current_block = mom->ReadBBD(current_block);
			current_block_offset += block_size;
		}
	}

	return rd;
}

void reader::entity::reset()
{
	pointer = 0;
	current_block_offset = 0;
	current_block = starting_block;
}

/* get an entity object for the root directory */
reader::entity* reader::rootdirectory()
{
	entity *x;

	x = new entity(this);
	if (!x) return NULL;

	x->size = 0;
	x->my_attr = entity::ATTR_SUBDIRECTORY;
	x->starting_block = root_block;
	x->block_size = 512;
	x->use_small_blocks = 0;
	x->find_handle_start = 0;
	x->info_valid = 0;
	x->adjust = 0;
	x->reset();
	x->ScanPropertyStorage();
	return x;
}

reader::entity* reader::small_block_depot()
{
	entity *x;

	if (sbd_1st >= 0xFFFFFFFD) return NULL;

	x = new entity(this);
	if (!x) return NULL;

	x->size = 0;
	x->my_attr = entity::ATTR_SUBDIRECTORY;
	x->starting_block = sbd_1st;
	x->block_size = 512;
	x->use_small_blocks = 0;
	x->find_handle_start = 0;
	x->info_valid = 0;
	x->adjust = 0;
	x->reset();
	x->ScanPropertyStorage();
	return x;
}

int reader::entity::parse(unsigned char buf[0x80],reader::entity::DIRENTRY *f,unsigned long ofs)
{
	int i;

	if (!f) return 0;

	for (i=0;i < 32;i++) f->ps_name_u[i] =	CMSOLEFS_GETWORD (buf + (i<<1));
	f->ps_name[32] =			0;
	f->ps_name_u[32] =			0;
	f->ps_name_len =			CMSOLEFS_GETWORD (buf + 0x40);
	f->ps_type =				CMSOLEFS_GETWORD (buf + 0x42) & 0xFF;
	f->ps_previous =			CMSOLEFS_GETDWORD(buf + 0x44);
	f->ps_next =				CMSOLEFS_GETDWORD(buf + 0x48);
	f->ps_directory =			CMSOLEFS_GETDWORD(buf + 0x4C);
	f->ps_start =				CMSOLEFS_GETDWORD(buf + 0x74);
	f->ps_size =				CMSOLEFS_GETDWORD(buf + 0x78);

	for (i=0;i < 32;i++) {
		if (f->ps_name_u[i]>>8)	f->ps_name[i] = '?';
		else			f->ps_name[i] = (unsigned char)f->ps_name_u[i];
	}

	f->attr =
		(f->ps_type == PSTYPE_STORAGE || f->ps_type == PSTYPE_ROOT) ?
		ATTR_SUBDIRECTORY : 0;

	return 1;
}

reader::entity* reader::entity::getdirent(reader::entity::DIRENTRY *f)
{
	entity *x;

	x = new entity(mom);
	if (!x) return NULL;

	/* this is a bit difficult compared to other readers.
	   it is possible for a directory to have a stream as well
	   as contain a subdirectory. the most common use for this
	   call however is to get subdirectories so for directories
	   that's what we'll make the object out to return. a caller
	   that wants the data associated with directories can use
	   getdirentdata() */

	if ((f->ps_type == PSTYPE_STORAGE || f->ps_type == PSTYPE_ROOT) && f->ps_directory != 0xFFFFFFFF) {
		/* "storage" or "root", so the new reader inherits our variables */
		x->size = size;
		x->use_small_blocks = use_small_blocks;
		x->block_size = block_size;
		x->starting_block = starting_block;
		x->adjust = 0;
		x->find_handle_start = f->ps_directory;
		x->ScanPropertyStorage();
	}
	else {
		/* "stream" */
		x->size = f->ps_size;
		x->use_small_blocks = (f->ps_size < 4096 && f->ps_type != PSTYPE_ROOT) ? 1 : 0;
		x->block_size = x->use_small_blocks ? 64 : 512;
		x->starting_block = f->ps_start;
		x->adjust = 0;
	}

	x->my_attr = f->attr;
	x->reset();
	memcpy(&x->info,f,sizeof(reader::entity::DIRENTRY));
	x->info_valid = 1;
	return x;
}

reader::entity* reader::entity::getdirentdata(reader::entity::DIRENTRY *f)
{
	entity *x;

	x = new entity(mom);
	if (!x) return NULL;

	x->size = f->ps_size;
	x->use_small_blocks = (f->ps_size < 4096 && f->ps_type != PSTYPE_ROOT) ? 1 : 0;
	x->block_size = x->use_small_blocks ? 64 : 512;
	x->starting_block = f->ps_start;
	x->adjust = 0;
	x->my_attr = f->attr;
	x->reset();
	memcpy(&x->info,f,sizeof(reader::entity::DIRENTRY));
	x->info_valid = 1;
	return x;
}

int reader::entity::dir_findfirst(reader::entity::DIRENTRY *f)
{
	if (!(my_attr&ATTR_SUBDIRECTORY)) return 0;
	find_handle=0;
	return dir_findnext(f);
}

int reader::entity::dir_findnext(reader::entity::DIRENTRY *f)
{
	reader::entity::DIRENTRY x;
	unsigned long o;
	unsigned char tmp[0x80];
	int found=0;

	if (!(my_attr&ATTR_SUBDIRECTORY)) return 0;
	if (!pps_array) return 0;

	do {
		if (find_handle >= pps_array_size) return 0;
		o = pps_array[find_handle] << 7;
		if (o != seek(o)) return 0;
		if (read(tmp,0x80) < 0x80) return 0;
		if (parse(tmp,&x,find_handle)) found=1;
		find_handle++;
	} while (!found);

	memcpy(f,&x,sizeof(x));
	return found;
}

/* root     = entity object who we treat as the "root" directory. can be NULL.
   instance = in case there's more than one file named "path" */
reader::entity* reader::file(char *path,entity* root,int instance,int type)
{
	entity::DIRENTRY f;
	entity* stk[32],*l=NULL;
	int stksp=0,fl,i,match=0;
	char *s,*next;

	/* skip first / if there */
	s = path;
	while (*s == '/') s++;

	/* start from the root directory */
	if (root)	stk[stksp] = root;
	else		stk[stksp] = rootdirectory();
	if (!stk[stksp]) return NULL;

	/* find the length of the first element */
	/* further down this code, next = next element or NULL, fl = length of this element */
	next = strchr(s,'/');
	if (!next)		fl = strlen(s);
	else			fl = (int)(next-s);

	i = stk[stksp]->dir_findfirst(&f);
	while (*s && i) {
		if (!match) {
			if (!strnicmp((char*)f.ps_name,s,fl)) {
				match=1;
			}
		}

		/* it is only a match if this is the instance the caller wants */
		if (!(f.attr & entity::ATTR_SUBDIRECTORY))
			if (match)
				if (instance-- > 0)
					match=0;

		/* now act on the match-up */
		if (match) {
			match=0;
			if (f.attr & entity::ATTR_SUBDIRECTORY) {
				/* last element? */
				if (!next) {
					if (type == FT_TYPE_DIR)
						l=stk[stksp]->getdirent(&f);

					/* it is permissible in this FS to
					   open a directory for it's stream,
					   rather than for the subdirectory */
					if (type == FT_TYPE_FILE)
						l=stk[stksp]->getdirentdata(&f);

					break;
				}
				/* enter the directory */
				else {
					l=stk[stksp]->getdirent(&f);
					if (!l) break;		/* stop if failure */
					if (stksp >= 31) break;	/* stop if too deep */

					i=l->dir_findfirst(&f);
					if (i) {
						s=next+1;
						next=strchr(s,'/');
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
		else			stksp--;
	}

	return l;
}

int reader::ReadAbsBlock(unsigned long o,int N,unsigned char *buf)
{
	/* blocks in the file are numbered so that the header is block -1, etc */
	o++;

	/* sanity checks */
	if (o >= disksize() || N < 1 || !buf) return 0;

	/* read it */
	return blockread(o,N,buf);
}

unsigned long reader::ReadBBD(unsigned long N)
{
	unsigned long so,bo,sb,ao;

	if (N >= 0xFFFFFFFD || N >= num_blocks) return 0xFFFFFFFF;

	sb  = N>>7;
	bo  = (sb<<2) + 0x4C;
	so  = (bo>>9)-1;
	bo &= 511;
	ao  = (N&127)<<2;

	if (!ReadAbsBlock(so,1,tmpsect)) return 0xFFFFFFFF;
	so = CMSOLEFS_GETDWORD(tmpsect+bo);
	if (!ReadAbsBlock(so,1,tmpsect)) return 0xFFFFFFFF;
	so = CMSOLEFS_GETDWORD(tmpsect+ao);
	return so;
}

unsigned long reader::ReadSBD(unsigned long N)
{
	unsigned char buf[4];
	unsigned long so;

	if (!sbd_ent) sbd_ent=small_block_depot();
	if (!sbd_ent) return 0xFFFFFFFF;

	so = N<<2;
	if (sbd_ent->seek(so) != so) return 0xFFFFFFFF;
	if (sbd_ent->read(buf,4) < 4) return 0xFFFFFFFF;
	so = CMSOLEFS_GETDWORD(buf);
	return so;
}

/* we must do this ahead of time to compensate for the
   strange way that prev/next pointers are used to find
   what we need. ugh... */
int reader::entity::ScanPropertyStorage()
{
	if (pps_array) return 0;
	pps_array_alloc = 32;
	pps_array_size = 0;
	pps_array = (unsigned long*)(malloc(pps_array_alloc * sizeof(unsigned long)));
	if (!pps_array) return 0;
	return ScanPropertyStorageRec(find_handle_start,40);
}

int reader::entity::ScanPropertyStorageRec(unsigned long blk,int recw)
{
	unsigned char buf[0x80];
	unsigned long o;
	DIRENTRY f;

	o = blk << 7;
	if (seek(o) != o) return 0;
	if (read(buf,0x80) < 0x80) return 0;
	if (!parse(buf,&f,blk)) return 0;

	if (pps_array_size >= pps_array_alloc) {
		pps_array_alloc += 32;
		pps_array = (unsigned long*)(realloc(pps_array,pps_array_alloc * sizeof(unsigned long)));
		if (!pps_array) return 0;
	}

	pps_array[pps_array_size++] = blk;

// 09-08-2005: What the hell? It seems 0xFFFF also happens to mean "end of chain"?
//             Are the whole 32 bits even used? Are invalid values used on purpose
//             because the implementation will just use them and not follow up?
//             What if the document were larger than 32MB? (65535 * 512 is about 32MB)
	if (f.ps_previous < 0xFFFFFFFC && recw >= 1)
		ScanPropertyStorageRec(f.ps_previous,recw-1);

	if (f.ps_next < 0xFFFFFFFC && recw >= 1)
		ScanPropertyStorageRec(f.ps_next,recw-1);

	return 1;
}

////////////////////////////////////////////////
};
// END CMSOLEFS namespace
