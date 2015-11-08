
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include "ciso9660.h"

#include "common/BlockDeviceFile.h"
#include "common/SomeFileStdio.h"

class iso9660reader : public CISO9660::reader
{
public:
	iso9660reader(char *fn);
	~iso9660reader();
public:
	virtual int				blocksize();
	virtual int				blockread(uint64 sector,int N,unsigned char *buffer);
	virtual uint64			disksize();
	virtual void			comment(int level,char *str,...);
	int						ok();
	int						use2352();
public:
	uint64					partition_offset;
	SomeFileStdio*			file;
	BlockDeviceFile*		blkf;
	char					raw2352_mode;
};

void iso9660reader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("iso9660reader[%s]: ",CISO9660::levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int	iso9660reader::use2352()
{
	if (blkf->SetBlockSize(2352) < 0) return 0;
	raw2352_mode=1;
	return 1;
}

int iso9660reader::ok()
{
	if (file && blkf) return 1;
	return 0;
}

int iso9660reader::blocksize()
{
	return 2048;
}

int iso9660reader::blockread(uint64 sector,int N,unsigned char *buffer)
{
	if (raw2352_mode) {
		// "RAW" ISO image, used personally to archive CDs in their entirety.
		// basically it consists of all 2352 bytes in the sector, including
		// sync bytes, header, and CRC codes in addition to data. this method
		// of imaging is also perfect for archiving CDs that have both data
		// and audio tracks because both can be read in this manner.
		sector += partition_offset;
		unsigned char buffy[2352];
		int rd=0;

		while (N-- > 0) {
			if (!blkf->ReadDisk(sector++,buffy,1))
				return rd;

			// make sure sync bytes are there
			if (buffy[ 0] != 0x00 || buffy[ 1] != 0xFF || buffy[ 2] != 0xFF ||
				buffy[ 3] != 0xFF || buffy[ 4] != 0xFF || buffy[ 5] != 0xFF ||
				buffy[ 6] != 0xFF || buffy[ 7] != 0xFF || buffy[ 8] != 0xFF ||
				buffy[ 9] != 0xFF || buffy[10] != 0xFF)
				return rd;

			memcpy(buffer,buffy + 24,2048);
			rd++;
		}

		return rd;
	}

	sector += partition_offset;
	return blkf->ReadDisk(sector,buffer,N);
}

uint64 iso9660reader::disksize()
{
	uint64 sz=blkf->SizeOfDisk();
	if (sz > partition_offset)	sz -= partition_offset;
	else						sz  = 0;
	return sz;
}

iso9660reader::iso9660reader(char *fn)
{
	raw2352_mode=0;
	partition_offset=0;
	file=new SomeFileStdio;
	blkf=new BlockDeviceFile;
	file->Open(fn);
	blkf->Assign(file);
	if (blkf->SetBlockSize(2048) < 0) {
		delete blkf;
		delete file;
	}
}

iso9660reader::~iso9660reader()
{
	delete blkf;
	delete file;
}

static int IsDotDot(char *x)
{
	if (x[0] == '.' && x[1] == 0)
		return 1;
	if (x[0] == '.' && x[1] == '.' && x[2] == 0)
		return 1;

	return 0;
}

int indent=0;
void List(iso9660reader* reader,CISO9660::reader::readdirents* root,uint32 parent)
{
	CISO9660::reader::readdir rd(root);
	int i;

	if (indent > 80)
		return;

	if (rd.first()) {
		indent++;
		do {
			// be careful of the fact that on all ISOs I test the first two entries
			// are '.' and '..' respectively which point to the directory itself
			// and it's parent directory
			if (llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le) != llei32(&rd.current->x.dr.Loc_of_Extent.le) &&
				parent != llei32(&rd.current->x.dr.Loc_of_Extent.le)) {
				CISO9660::reader::RockRidge rocky(root->mom);
				char finalname[300];
				char RockRidge = 0;
				uint32 RRGraftOfs;
				char RRGraft = 0;
				finalname[0] = 0;

				// check for Rock Ridge extensions and get the filename that way
				if (RockRidge = rocky.Check(rd.current)) {
					char *x = rocky.GetName();
					if (x) strcpy(finalname,x);
				}

				if (RockRidge)
					// is this THE relocated Rock Ridge directory?
					if (rocky.IsRelocatedDir())
						continue;		// skip it, we'll come across the graft point eventually

				if (finalname[0] == 0) {
					if (rd.file_version != NULL) {
						if (!strcmp((char*)rd.file_version,"1")) {
							strcpy(finalname,(char*)rd.name);
						}
						else {
							sprintf(finalname,"v%s-%s",rd.file_version,rd.name);
						}
					}
					else {
						strcpy(finalname,(char*)rd.name);
					}
				}

				for (i=1;i < indent;i++) printf("  ");
				printf("%s\n",finalname);

				// is this the graft point for the relocated RR directory?
				if (RockRidge) {
					RRGraftOfs = rocky.GetDirRelocation();
					if (RRGraftOfs > 0 && !(rd.current->x.dr.File_Flags & CISO9660::reader::Sec91_DirRecord_File_Flags_Directory)) {
						// okay, good. unfortunately it only gives us the first sector of the directory that
						// was relocated, so to accurately determine size and properly enumerate that relocated
						// directory we must re-scan the tree and look for it.
						RRGraft = 1;
					}
				}

				if (RockRidge) {
					// is this THE relocated Rock Ridge directory?
					if (rocky.IsSparse()) {
						printf("Sparse Files Not Supported\n");
						continue;		// skip it, we'll come across the graft point eventually
					}
				}

				// handle a Rock Ridge directory that was relocated to fit ISO 9660 standards
				// (directory recursion limits)
				if (RockRidge && RRGraft) {
					mkdir((char*)finalname);
					if (chdir((char*)finalname) < 0) continue;

					for (i=1;i < indent;i++) printf("  ");
					printf("--------------------------\n",finalname);
					CISO9660::reader::readdirents r(root->mom);
					CISO9660::reader::readdirents* DaRoot = reader->GetRootDir();

					if (!DaRoot)
						continue;

					// experience tells me (according to tests with mkisofs) that
					// the grafted directory is usually one level down from root inside
					// another directory named "rr_moved". the Rock Ridge spec though
					// says nothing about that directory having to exist in the root.........
					// so.... scan the entire tree for it.
					if (!rocky.FindByMatchingGraftOffset(&r,DaRoot,0,RRGraftOfs)) {
						delete DaRoot;
						continue;
					}

					r.finish();
					r.seek(0);
					List(reader,&r,llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le));
					printf("\n");
					delete DaRoot;
					if (chdir("..") < 0) continue;
				}
				// handle an ordinary subdirectory
				else if (rd.current->x.dr.File_Flags & CISO9660::reader::Sec91_DirRecord_File_Flags_Directory) {
					mkdir((char*)finalname);
					if (chdir((char*)finalname) < 0) continue;

					for (i=1;i < indent;i++) printf("  ");
					printf("--------------------------\n",finalname);
					CISO9660::reader::readdirents r(root->mom);
					r.add(rd.current);
					r.finish();
					r.seek(0);
					List(reader,&r,llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le));
					printf("\n");

					if (chdir("..") < 0) continue;
				}
				// file
				else {
					FILE *fp=fopen((char*)finalname,"wb");
					if (fp) {
						CISO9660::reader::readdirents r(root->mom);
						unsigned char buffy[4096];
						r.add(rd.current);
						r.finish();
						r.seek(0);
						int rd;

						while ((rd=r.read(buffy,sizeof(buffy))) > 0)
							fwrite(buffy,rd,1,fp);

						fclose(fp);
					}
				}
			}
		} while (rd.next());
		indent--;
	}
}

int utilmain(int argc,char **argv);

int main(int argc,char **argv)
{
	iso9660reader* r;
	CISO9660::reader::readdirents* root;

	if (argc < 2) {
		printf("%s <disk image>\n");
		printf("%s -util <disk image>\n");
		return 1;
	}

	/* you can use the -util switch to analyze the ISO */
	if (!strcmp(argv[1],"-util")) {
		if (argc < 3) {
			printf("%s -util <disk image>\n");
			return 1;
		}

		return utilmain(argc,argv);
	}

	r = new iso9660reader(argv[1]);
	if (!r->ok()) return 1;
	if (!r->init()) return 1;
	if (!r->mount()) {
		// it might be a raw 2352 byte/sector image...
		r->use2352();
		if (!r->mount()) return 1;
	}

	// check for Microsoft Joliet extensions so we can get the full filename.
	// if not there, fall back to the primary (fully ISO 9660 compliant) volume.
	if (r->CheckJoliet())
		printf("Using Joliet extensions\n");
	else if (!r->ChoosePrimary())
		return 1;

	if (!(root=r->GetRootDir())) return 1;
	mkdir("__RIPPED__");
	if (chdir("__RIPPED__") < 0) return 1;
	List(r,root,0);
	if (chdir("..") < 0) return 1;
	delete root;

	r->umount();
	r->freeMe();
	delete r;
	return 0;
}

////////////////////////// -util code

class TheString {
public:
	TheString() {
		str = NULL;
	}
	~TheString() {
		if (str) delete str;
		str = NULL;
	}
	void Set(char *x)
	{
		if (str) delete str;
		str = NULL;

		int len = strlen(x);
		str = new char[len+1];
		if (!str) return;
		strcpy(str,x);
	}
public:
	char*			str;
};

typedef struct {
	uint32					start,len;
	AnySize<TheString>		names;
} utiluse;

AnySize<utiluse>	utilization;
AnySize<utiluse>	utilization_overlaps;

int utilindent=0;
void UtilList(iso9660reader* reader,CISO9660::reader::readdirents* root,uint32 parent)
{
	CISO9660::reader::readdir rd(root);

	if (root->list.item_max < 1)
		return;

	if (rd.first()) {
		utilindent++;
		do {
			// be careful of the fact that on all ISOs I test the first two entries
			// are '.' and '..' respectively which point to the directory itself
			// and it's parent directory
			CISO9660::reader::RockRidge rocky(root->mom);
			if (llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le) != llei32(&rd.current->x.dr.Loc_of_Extent.le) &&
				parent != llei32(&rd.current->x.dr.Loc_of_Extent.le)) {
				if (rd.current->x.dr.File_Flags & CISO9660::reader::Sec91_DirRecord_File_Flags_Directory) {
					CISO9660::reader::readdirents r(root->mom);
					r.add(rd.current);
					r.finish();
					r.seek(0);

					if (r.list.item_max > 0) {
						char buffy[512];
						utiluse* u = utilization.Alloc();
						if (!u) return;
						u->start = llei32(&r.list.item[0]->x.dr.Loc_of_Extent.le);
						u->len = llei32(&r.list.item[0]->x.dr.Data_Length.le);
						u->len = (u->len + 2047) >> 11;
						TheString* s = u->names.Alloc();
						if (!s) return;
						sprintf(buffy,"[directory] %s",rd.name);
						s->Set(buffy);
						char *x = rocky.GetName();
						if (x) {
							s = u->names.Alloc();
							if (!s) return;
							sprintf(buffy,"[directory] %s",x);
							s->Set(buffy);
						}
					}

					UtilList(reader,&r,llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le));
				}
				else {
					char buffy[512];
					utiluse* u = utilization.Alloc();
					if (!u) return;
					u->start = llei32(&rd.current->x.dr.Loc_of_Extent.le);
					u->len = llei32(&rd.current->x.dr.Data_Length.le);
					u->len = (u->len + 2047) >> 11;
					TheString* s = u->names.Alloc();
					if (!s) return;
					sprintf(buffy,"[file] %s",rd.name);
					s->Set(buffy);
					char *x = rocky.GetName();
					if (x) {
						s = u->names.Alloc();
						if (!s) return;
						sprintf(buffy,"[file] %s",x);
						s->Set(buffy);
					}
				}
			}
		} while (rd.next());
		utilindent--;
	}
}

int __cdecl utilcomp(const void *e1,const void *e2)
{
	utiluse* u1 = *((utiluse**)e1);
	utiluse* u2 = *((utiluse**)e2);
	return u1->start - u2->start;
}

void SortUtil()
{
	qsort(utilization.item,utilization.item_max,sizeof(utiluse*),utilcomp);
}

void CombineUtil()
{
	int i;

	for (i=0;i < (utilization.item_max-1);) {
		if (utilization.item[i]->start == utilization.item[i+1]->start) {
			TheString *s = utilization.item[i]->names.Alloc();
			int n;

			for (n=0;n < utilization.item[i+1]->names.item_max;n++)
				s->Set(utilization.item[i+1]->names.item[n]->str);

			utilization.Free(utilization.item[i+1]);
		}
		else {
			i++;
		}
	}
}

void NoteOverlapsAndGaps()
{
	int i,imax = utilization.item_max - 1;

	for (i=0;i < imax;i++) {
		uint32 ende = utilization.item[i]->start + utilization.item[i]->len;
		if (ende < utilization.item[i+1]->start) {
			// gap
			utiluse *u = utilization.Alloc();
			u->start = ende;
			u->len = utilization.item[i+1]->start - ende;
			TheString *s = u->names.Alloc();
			s->Set("[unused gap]");
		}
		else if (ende > utilization.item[i+1]->start) {
			char buffy[1024],*n1,*n2;
			int ii = i;

			while (ii < imax) {
				uint32 where = utilization.item[ii+1]->start;
				uint32 wheree = utilization.item[ii+1]->start + utilization.item[ii+1]->len;
				if (ii < (imax-1))
					if (wheree > utilization.item[ii+2]->start)
						wheree = utilization.item[ii+2]->start;

				if (wheree > ende) wheree = ende;
				utiluse *u = utilization_overlaps.Alloc();
				u->start = where;
				u->len = wheree - where;
				TheString *s = u->names.Alloc();
				if (utilization.item[ii]->names.item_max > 0)	n1 = utilization.item[ii]->names.item[0]->str;
				else											n1 = "(unknown)";
				if (utilization.item[ii+1]->names.item_max > 0)	n2 = utilization.item[ii+1]->names.item[0]->str;
				else											n2 = "(unknown)";
				sprintf(buffy,"[Overlapping region: %s and %s]",n1,n2);
				s->Set(buffy);

				if (wheree >= ende)
					break;

				where = wheree;
				ii++;
			}

			// correct the entry for completeness
			utilization.item[i]->len = utilization.item[i+1]->start - utilization.item[i]->start;
		}
	}
}

// list the directory itself
void ListRoot(CISO9660::reader::readdirents *root)
{
	if (root->list.item_max < 1) return;
	utiluse* u = utilization.Alloc();
	if (!u) return;
	u->start = llei32(&root->list.item[0]->x.dr.Loc_of_Extent.le);
	u->len = llei32(&root->list.item[0]->x.dr.Data_Length.le);
	u->len = (u->len + 2047) >> 11;
	TheString* s = u->names.Alloc();
	if (!s) return;
	s->Set("ISO 9660 Root Directory");
}

void AddLastOne(iso9660reader* r)
{
	uint64 maxy = r->disksize();
	int i = utilization.item_max - 1;
	if (i < 0) return;
	utiluse *u = utilization.item[i];
	uint64 last = u->start + u->len;

	if (last < maxy) {
		utiluse *nu = utilization.Alloc();
		nu->start = last;
		nu->len = maxy - last;
		TheString *s = nu->names.Alloc();
		s->Set("[remainder]");
	}
	else if (last > maxy) {
		utiluse *nu = utilization.Alloc();
		nu->start = maxy;
		nu->len = last - maxy;
		TheString *s = nu->names.Alloc();
		s->Set("[region reaching beyond the block device limit]");
	}
}

int utilmain(int argc,char **argv)
{
	uint32 waste=0;
	int verbose=0,i;
	iso9660reader* r;
	CISO9660::reader::readdirents* root;

	for (i=3;i < argc;i++) {
		if (argv[i][0] == '-') {
			char *m = argv[i]+1;

			if (m[0] == 'v')
				verbose++;
		}
	}

	r = new iso9660reader(argv[2]);		// because argv[1] == "-util"
	if (!r->ok()) return 1;
	if (!r->init()) return 1;
	if (!r->mount()) {
		// it might be a raw 2352 byte/sector image...
		r->use2352();
		if (!r->mount()) return 1;
	}

	// unlike the normal routine we check to see how fully utilized the ISO
	// image space is by analyzing ALL of the structures

	// list the system use area
	{
		utiluse* u = utilization.Alloc();
		if (!u) return 1;
		u->start = 0;
		u->len = 16;
		TheString* s = u->names.Alloc();
		if (!s) return 1;
		s->Set("System Use Area");
	}

	// list the volume descriptors
	{
		utiluse* u = utilization.Alloc();
		if (!u) return 1;
		u->start = 16;
		u->len = r->TotalVolDescriptors;
		TheString* s = u->names.Alloc();
		if (!s) return 1;
		s->Set("ISO 9660 Volume Descriptors");
	}

	// first check the stock ISO 9660 structure
	if (r->ChoosePrimary()) {
		if (root=r->GetRootDir()) {
			ListRoot(root);
			UtilList(r,root,0);
			delete root;
		}
	}

	// then the Joliet extensions, if any
	if (r->CheckJoliet()) {
		if (root=r->GetRootDir()) {
			ListRoot(root);
			UtilList(r,root,0);
			delete root;
		}
	}

	SortUtil();
	AddLastOne(r);
	CombineUtil();
	NoteOverlapsAndGaps();
	SortUtil();

	if (utilization.item_max > 0)
		if (verbose >= 1)
			printf("The following regions are associated with known ISO structures:\n");
		else
			printf("The following gaps were found according to known ISO structures:\n");

	{
		int i,n;

		for (i=0;i < utilization.item_max;i++) {
			char show = (verbose >= 1) ? 1 : 0;
			utiluse* u = utilization.item[i];
			char iswaste = 0;

			if (u->names.item_max > 0) {
				for (n=0;n < u->names.item_max;n++) {
					TheString *s = u->names.item[n];
					if (!strcmp(s->str,"[unused gap]")) {
						iswaste = 1;
						show = 1;
					}
					else if (!strcmp(s->str,"[remainder]")) {
						iswaste = 1;
						show = 1;
					}
					else if (!strcmp(s->str,"[region reaching beyond the block device limit]")) {
						show = 1;
					}
				}
			}

			if (iswaste)
				waste += u->len;

			if (show) {
				printf("  @ %8u-%8u: ",u->start,u->start + u->len + -1);
				if (u->names.item_max > 0) {
					TheString *s = u->names.item[0];
					printf("%s\n",s->str);
				}
				else {
					printf("\n");
				}
				for (n=1;n < u->names.item_max;n++) {
					TheString *s = u->names.item[n];
					printf("               a.k.a:  ");
					printf("%s\n",s->str);
				}
			}
		}
	}

	if (utilization_overlaps.item_max > 0)
		printf("\nThe following regions were found to overlap:\n");

	{
		int i,n;

		for (i=0;i < utilization_overlaps.item_max;i++) {
			utiluse* u = utilization_overlaps.item[i];
			printf("  @ %8u-%8u: ",u->start,u->start + u->len + -1);
			if (u->names.item_max > 0) {
				TheString *s = u->names.item[0];
				printf("%s\n",s->str);
			}
			else {
				printf("\n");
			}
			for (n=1;n < u->names.item_max;n++) {
				TheString *s = u->names.item[n];
				printf("               a.k.a:  ");
				printf("%s\n",s->str);
			}
		}
	}

	printf("%u sectors are wasted/unknown in this image\n",waste);

	r->umount();
	r->freeMe();
	delete r;
	return 0;
}
