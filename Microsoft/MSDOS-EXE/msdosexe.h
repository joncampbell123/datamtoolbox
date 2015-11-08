
#include "common/Types.h"
#include "common/SomeFileStdio.h"

class MSDOSEXE {
public:
typedef struct EXEhdr {
  unsigned short signature;
  unsigned short bytes_in_last_block;
  unsigned short blocks_in_file;
  unsigned short num_relocs;
  unsigned short header_paragraphs;
  unsigned short min_extra_paragraphs;
  unsigned short max_extra_paragraphs;
  unsigned short ss;
  unsigned short sp;
  unsigned short checksum;
  unsigned short ip;
  unsigned short cs;
  unsigned short reloc_table_offset;
  unsigned short overlay_number;
};
public:
	void				Assign(SomeFile *f);
	int					Check();
	EXEhdr*				GetHeader();
	unsigned long		GetWindowsEXEHeader();
	unsigned long		GetResidentOffset();
	unsigned long		GetUsableSize();
	unsigned long		GetResidentSize();
	unsigned long		GetResidentTotalSize();
	void				CheckRelocationTable();
	unsigned long		GetIPoffset();
	unsigned long		GetSPoffset();
public:
	virtual void		comment(char isErr,char *str,...);
private:
	SomeFile*			source;
	EXEhdr				hdr;
	const char*			__func_name;
	void				NAME(const char *s);
	unsigned char		problems;
public:
	enum {
		PROBLEM_USABLE_SIZE=1,
		PROBLEM_HEADER_SIZE=2
	};
};
