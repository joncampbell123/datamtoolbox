// NOTE: there is additional code that allows the unicode LFNs to
//       be preserved on Windows systems.

#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include "cntfs.h"

#include "common/BlockDeviceFile.h"
#include "common/SomeFileStdio.h"

class ntfsreader : public CNTFS::reader
{
public:
	ntfsreader(char *fn);
	~ntfsreader();
public:
	virtual int				blocksize();
	virtual int				blockread(uint64 sector,int N,unsigned char *buffer);
	virtual uint64			disksize();
	virtual void			comment(int level,char *str,...);
	int						ok();
public:
	uint64					partition_offset;
	SomeFileStdio*			file;
	BlockDeviceFile*		blkf;
};

void ntfsreader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("ntfsreader[%s]: ",CNTFS::levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int ntfsreader::ok()
{
	if (file && blkf) return 1;
	return 0;
}

int ntfsreader::blocksize()
{
	return 512;
}

int ntfsreader::blockread(uint64 sector,int N,unsigned char *buffer)
{
	sector += partition_offset;
	return blkf->ReadDisk(sector,buffer,N);
}

uint64 ntfsreader::disksize()
{
	uint64 sz=blkf->SizeOfDisk();
	if (sz > partition_offset)	sz -= partition_offset;
	else						sz  = 0;
	return sz;
}

ntfsreader::ntfsreader(char *fn)
{
	partition_offset=0;
	file=new SomeFileStdio;
	blkf=new BlockDeviceFile;
	file->Open(fn);
	blkf->Assign(file);
	if (blkf->SetBlockSize(512) < 0) {
		delete blkf;
		delete file;
	}
}

ntfsreader::~ntfsreader()
{
	delete blkf;
	delete file;
}

int indent=0;
void ListDir(CNTFS::reader::MFT::RecordReader *root)
{
	CNTFS::reader::MFT::RecordReader::DirectorySearch ds(root);
	if (indent >= 30) return;
	indent++;
	int i;

	if (ds.Gather()) {
		if (ds.FindFirst()) {
			do {
				if (ds.found_name)
					ds.found_name->Unicode2Ansi();

				// enter subdirectory and enumerate
				if (ds.found_name->file_attributes & CNTFS::reader::MFT::FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT) {
					// avoid reentry problems---don't enter $ROOT again
					if (ds.MFT_current == 5) continue;

					for (i=1;i < indent;i++) printf("      ");
					printf("-----------------------------------------------\n");
					for (i=1;i < indent;i++) printf("      ");
					printf("  CONTENTS OF DIRECTORY %s\n",ds.found_name->name_a);
					for (i=1;i < indent;i++) printf("      ");
					printf("  ---------------------------------------------\n");

					mkdir(ds.found_name->name_a);
					if (chdir(ds.found_name->name_a) < 0) continue;

					// okay, make an object for subdirs
					CNTFS::reader::MFT::RecordReader sub(root->mom);
					if (!sub.Parse(ds.MFT_current)) continue;
					ListDir(&sub);
					printf("\n");
					chdir("..");
				}
				else {
					for (i=1;i < indent;i++) printf("      ");
					printf("  %s\n",ds.found_name->name_a);

					if (ds.found_name->name_a) {
						CNTFS::reader::MFT::RecordReader sub(root->mom);
						if (sub.Parse(ds.MFT_current)) {
							CNTFS::reader::MFT::NTFSFileData filo_basic(&sub);		// the base file object
							CNTFS::reader::MFT::NTFSFileDataCompressed *compressed;	// interpreter object for compressed NTFS files
							CNTFS::reader::MFT::NTFSFileData *filo;					// points to either base or through interpreter

							filo = &filo_basic;
							compressed=NULL;
							if (filo->SetupData()) {
								if (filo->NeedInterpreter()) {
									// if the file is compressed at filesystem level we will need to
									// map all read operations through an interpreter object that will
									// perform on the fly decompression of the compressed data.
									if (filo->attributes & CNTFS::reader::MFT::ATTR_IS_COMPRESSED) {
										compressed = new CNTFS::reader::MFT::NTFSFileDataCompressed(filo);
										if (compressed) {
											if (compressed->SetupData()) {
												filo = compressed;
											}
										}
									}
								}

								if (filo->seek(0) == 0) {
									unsigned char buffera[4096];
									FILE *fp = fopen(ds.found_name->name_a,"wb");
									int br;

									if (!strcmpi(ds.found_name->name_a,"wword60.doc"))
										br = br;

									if (fp) {
										while ((br=filo->read(buffera,sizeof(buffera))) > 0)
											fwrite(buffera,br,1,fp);

										fclose(fp);
									}
								}
							}

							if (compressed) delete compressed;
						}
					}
				}
			} while (ds.FindNext());
		}
	}

	indent--;
}

int main(int argc,char **argv)
{
	CNTFS::reader::MFT::RecordReader *root;
	ntfsreader* r;

	if (argc < 2) {
		printf("%s <disk image>\n");
		return 1;
	}

	r = new ntfsreader(argv[1]);
	if (!r->ok()) return 1;
	if (!r->init()) return 1;
	if (!r->mount()) {
		unsigned char buffy[512];
		int i;

		// look for NTFS partition
		if (r->blockread(0,1,buffy) == 0) return 1;
		for (i=0;i < 4;i++) {
			if (buffy[0x1BE + (i * 0x10) + 4] == 0x07) {
				r->partition_offset = llei32(buffy + 0x1BE + (i * 0x10) + 8);
				break;
			}
		}

		if (i >= 4) return 1;
		if (!r->mount()) return 1;
	}
	if (!r->CheckMFT()) return 1;			// check and verify the $MFT
	if (!(root=r->GetRootDir())) return 1;	// get the root directory

	mkdir("__RIPPED__");
	if (chdir("__RIPPED__") < 0) return 0;

	ListDir(root);

	if (chdir("..") < 0) return 0;
	r->umount();
	r->freeMe();
	delete r;
	return 0;
}
