
/* a C++ class for reading the Syntomic Systems MCO filesystems
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 *
 * supported filesystems:
 *
 *   MCO1
 *     native to:      MCO 1.x
 *
 *   MCO2
 *     native to:      MCO 2.x
 *
 */

#include "Other/MCO/cmcofs.h"
#include <string.h>
#include <stdio.h>

// BEGIN CMCOFS namespace
namespace CMCOFS {
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

int reader::GetHeader()
{
	unsigned char sig[32];

	MCOfs_version = 0;

	/* look for MCO 2.x */
	if (cread(8,32,sig) < 32) {
		comment(C_WARNING,"Cannot read enough to determine if MCO 2.x");
		return 0;
	}
	else if (!memcmp(sig,"MCO2",4)) {
		MCOfs_version = 2;
	}

	/* look for MCO 1.x */
	if (MCOfs_version == 0) {
		if (cread(32768,32,sig) < 32) {
			comment(C_WARNING,"Cannot read enough to determine if MCO 1.x");
			return 0;
		}
		else if (!memcmp(sig,"MCO1",4)) {
			MCOfs_version = 1;
		}
	}

	if (MCOfs_version == 0)
		return 0;

	memcpy(MCO_signature,sig,4);
	if (MCOfs_version == 1) {
		MCO_boot_area =				CMCOFS_GETDWORD(sig +  4);
		MCO2_lbt_offset =			0;
		MCO2_lbt_size =				0;
		MCO2_adt_offset =			0;
		MCO2_adt_size =				0;
		MCO_dir_offset =			CMCOFS_GETDWORD(sig + 16);
		MCO_dir_size =				CMCOFS_GETDWORD(sig + 20);
		MCO1_pfa_offset =			CMCOFS_GETDWORD(sig + 24);
		MCO1_pfa_size =				CMCOFS_GETDWORD(sig + 28);

		if (MCO_dir_size < MCO_dir_offset) {
			comment(C_WARNING,"pointer to end of directory < pointer to directory itself!");
			return 0;
		}

		MCO_dir_size -= MCO_dir_offset;
	}
	else if (MCOfs_version == 2) {
		MCO_boot_area =				CMCOFS_GETDWORD(sig +  4);
		MCO2_lbt_offset =			CMCOFS_GETDWORD(sig +  8);
		MCO2_lbt_size =				CMCOFS_GETDWORD(sig + 12);
		MCO2_adt_offset =			CMCOFS_GETDWORD(sig + 16);
		MCO2_adt_size =				CMCOFS_GETDWORD(sig + 20);
		MCO_dir_offset =			CMCOFS_GETDWORD(sig + 24);
		MCO_dir_size =				CMCOFS_GETDWORD(sig + 28);
		MCO1_pfa_offset =			0;
		MCO1_pfa_size =				0;
	}

	if (MCO_dir_offset == 0) {
		comment(C_ERROR,"There is no directory!");
		return 0;
	}

	return 1;
}

int reader::cread(unsigned long offset,int len,unsigned char *buf)
{
	unsigned long ss;
	int so,sl,rd;

	rd = 0;
	ss = (offset - (offset % sectsize)) / sectsize;
	while (len > 0) {
		so = offset % sectsize;
		sl = sectsize - so;
		if (sl > len) sl = len;

		if (!blockread(ss,1,tmpsect)) {
			comment(C_ERROR,"Cannot read sector %u\n",ss);
			return rd;
		}

		memcpy(buf,tmpsect+so,sl);
		offset += sl;
		buf += sl;
		len -= sl;
		rd += sl;
		ss++;
	}

	return rd;
}

int reader::mount()
{
	if (mounted) {
		comment(C_ERROR,"Already mounted!");
		return 0;
	}

	/* with MCO's filesystem sector size doesn't matter */
	sectsize=blocksize();
	if (sectsize < 1) {
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
		comment(C_ERROR,"Not an MCO filesystem!");
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
	if (pos > size)		pointer = pos = size;
	else			pointer = pos;

	return pointer;
}

int reader::entity::read(unsigned char *buf,int sz)
{
	unsigned long ps;
	int rd;

	rd = sz;
	ps = size - pointer;
	if (rd > ps) rd = ps;
	if (rd <= 0) return 0;

	rd = mom->cread(offset + pointer,rd,buf);
	pointer += rd;
	return rd;
}

void reader::entity::reset()
{
	pointer = 0;
}

/* get an entity object for the root directory */
reader::entity* reader::rootdirectory()
{
	entity *x;

	x = new entity(this);
	if (!x) return NULL;

	x->offset = MCO_dir_offset;
	x->size = MCO_dir_size;
	x->my_attr = entity::ATTR_SUBDIRECTORY;
	x->info_valid = 0;
	x->reset();
	return x;
}

int reader::entity::parse(unsigned char buf[36],reader::entity::DIRENTRY *f,unsigned long ofs)
{
	if (!f) return 0;
	if (buf[0] == 0) return 0;

	f->fs_version = mom->MCOfs_version;
	if (f->fs_version == 2) {
		memcpy(&f->name,buf,16);	f->name[16] = 0;
		f->sub_number =			CMCOFS_GETWORD (buf+16);
		f->pointer =			CMCOFS_GETDWORD(buf+18);
		f->length =			CMCOFS_GETDWORD(buf+22);
		f->creation_time =		CMCOFS_GETDWORD(buf+26);
		f->last_access_time =		CMCOFS_GETDWORD(buf+30);
		f->attr =			CMCOFS_GETWORD (buf+34);
	}
	else if (f->fs_version == 1) {
		memcpy(&f->name,buf,16);	f->name[16] = 0;
		f->sub_number =			0;
		f->pointer =			CMCOFS_GETDWORD(buf+26);
		f->length =			CMCOFS_GETDWORD(buf+30);
		f->creation_time =		0;
		f->last_access_time =		0;
		f->attr =			0;
	}

	return 1;
}

reader::entity* reader::entity::getdirent(reader::entity::DIRENTRY *f)
{
	entity *x;

	x = new entity(mom);
	if (!x) return NULL;

	x->my_attr = f->attr;
	x->offset = f->pointer;
	x->size = f->length;
	x->reset();
	memcpy(&x->info,f,sizeof(reader::entity::DIRENTRY));
	x->info_valid = 1;
	return x;
}

int reader::entity::dir_findfirst(reader::entity::DIRENTRY *f)
{
	if (!(my_attr&ATTR_SUBDIRECTORY)) return 0;
	findoffset=seek(0);
	return dir_findnext(f);
}

int reader::entity::dir_findnext(reader::entity::DIRENTRY *f)
{
	reader::entity::DIRENTRY x;
	unsigned char tmp[36];
	int found=0;

	if (!(my_attr&ATTR_SUBDIRECTORY)) return 0;

	do {
		if (findoffset % 36) return 0;
		if (findoffset != seek(findoffset)) return 0;
		if (read(tmp,36) < 36) return 0;
		if (parse(tmp,&x,findoffset)) found=1;
		findoffset += 36;
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
			if (!strnicmp((char*)f.name,s,fl)) {
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
		else					stksp--;
	}

	return l;
}

////////////////////////////////////////////////
};
// END CMCOFS namespace
