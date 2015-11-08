
/* test program for CMSOLEFS classes */

#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#include <string.h>
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Microsoft/MS-OLE/cmsolefs.h"

#ifdef LINUX
unsigned long long lseek64(int fd,unsigned long long off,int whence);
#endif

class msolereader : public CMSOLEFS::reader
{
public:
	virtual int					blocksize();
	virtual int					blockread(unsigned int sector,int N,unsigned char *buffer);
	virtual unsigned long		disksize();
	virtual void				comment(int level,char *str,...);
public:
	unsigned long				bsize;
	SomeFile*					sf;
};

static char *levs[] = {
	"PANIC   ",	"CRITICAL",	"ERROR   ",	"WARNING ",
	"INFO    ",	"DEBUG   ",	"        ",
};

void msolereader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("msolereader[%s]: ",levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int msolereader::blocksize()
{
	return 512;
}

int msolereader::blockread(unsigned int sector,int N,unsigned char *buffer)
{
	uint64 o;
	int r;

	o = ((uint64)sector) * ((uint64)512);
	if (sf->Seek(o) != o) return 0;
	r = sf->Read(buffer,N*512);
	if (r < 0)	r = 0;
	else		r >>= 9;
	return r;
}

unsigned long msolereader::disksize()
{
	return sf->GetSize() >> 9;
}

class MSWord97Reader {
public:
	MSWord97Reader();
	~MSWord97Reader();
public:
	typedef struct FIB {
		unsigned short		wIdent;						// 0x0000: magic number
		unsigned short		nFib;						// 0x0002: FIB version written
		unsigned short		nProduct;					// 0x0004: product version written by
		unsigned short		lid;						// 0x0006: language stamp
		unsigned short		pnNext;						// 0x0008: ?
		unsigned short		flags_0x000A;				// 0x000A: flags
		unsigned short		nFibBack;					// 0x000C: file format is compatible with readers that understand >= this version
		unsigned long		lKey;						// 0x000E: File encryption key
		unsigned char		envr;						// 0x0012: file was created in.... 0=Windows Word  1=Macintosh Word
		unsigned char		fMac;						// 0x0013: flags
		unsigned short		chs;						// 0x0014: default extended character set... 0=Windows ANSI   1=Macintosh
		unsigned short		chsTables;					// 0x0016: default extended character set for tables... 0=Windows ANSI   1=Macintosh
		unsigned long		fcMin;						// 0x0018: file offset of first character of text
		unsigned long		fcMac;						// 0x001C: file offset of last character in document text stream + 1
		unsigned short		csw;						// 0x0020: count of fields in the array of shorts
		union {
			unsigned short		rgsw;					// 0x0022: beginning of the array of shorts
			unsigned short		wMagicCreated;			// 0x0022: unique number identifying the file's creator
		} u0022;
		unsigned short		wMagicRevised;				// 0x0024: identifies the file's last modifier
		unsigned short		wMagicCreatedPrivate;		// 0x0026: private
		unsigned short		wMagicRevisedPrivate;		// 0x0028: private
		unsigned short		pnFbpChpFirst_W6;			// 0x002A: ?
		unsigned short		pnChpFirst_W6;				// 0x002C: ?
		unsigned short		cpnBteChp_W6;				// 0x002E: ?
		unsigned short		pnFbpPapFirst_W6;			// 0x0030: ?
		unsigned short		pnPapFirst_W6;				// 0x0032: ?
		unsigned short		cpnBtePap_W6;				// 0x0034: ?
		unsigned short		pnFbpLvcFirst_W6;			// 0x0036: ?
		unsigned short		pnLvcFirst_W6;				// 0x0038: ?
		unsigned short		cpnBteLvc_W6;				// 0x003A: ?
		unsigned short		lidFE;						// 0x003C: language ID if the document was written by Word Far East Edition
		unsigned short		clw;						// 0x003E: Number of fields in the array of longs
		union {
			unsigned long	rglw;						// 0x0040: Beginning of the array of longs
			unsigned long	cbMac;						// 0x0040: file offset of last byte written to file + 1
		} u0040;
		unsigned long		lProductCreated;			// 0x0044: contains the build date of the creator. 10695 indicates the creator program was compiled on Jan 6, 1995
		unsigned long		lProductRevised;			// 0x0048: contains the build date of the File's last modifier
		unsigned long		ccpText;					// 0x004C: length of main document text stream
		unsigned long		ccpFtn;						// 0x0050: length of footnote subdocument text stream
		unsigned long		ccpHdd;						// 0x0054: length of header subdocument text stream
		unsigned long		ccpMcr;						// 0x0058: length of macro subdocument text stream, which should now always be 0.
		unsigned long		ccpAtn;						// 0x005C: length of annotation subdocument text stream
		unsigned long		ccpEdn;						// 0x0060: length of endnote subdocument text stream
		unsigned long		ccpTxbx;					// 0x0064: length of textbox subdocument text stream
		unsigned long		ccpHdrTxbx;					// 0x0068: length of header textbox subdocument text stream.
		unsigned long		pnFbpChpFirst;				// 0x006C: when there was insufficient memory for Word to expand the plcfbte at save time, the plcfbte is written to the file in a linked list of 512-byte pieces starting with this pn
		unsigned long		pnChpFirst;					// 0x0070: the page number of the lowest numbered page in the document that records CHPX FKP information
		unsigned long		cpnBteChp;					// 0x0074: count of CHPX FKPs recorded in file. In non-complex files if the number of entries in the plcfbteChpx is less than this, the plcfbteChpx is incomplete.
		unsigned long		pnFbpPapFirst;				// 0x0078: when there was insufficient memory for Word to expand the plcfbte at save time, the plcfbte is written to the file in a linked list of 512-byte pieces starting with this pn 
		unsigned long		pnPapFirst;					// 0x007C: the page number of the lowest numbered page in the document that records PAPX FKP information
		unsigned long		cpnBtePap;					// 0x0080: count of PAPX FKPs recorded in file. In non-complex files if the number of entries in the plcfbtePapx is less than this, the plcfbtePapx is incomplete.
		unsigned long		pnFbpLvcFirst;				// 0x0084: when there was insufficient memory for Word to expand the plcfbte at save time, the plcfbte is written to the file in a linked list of 512-byte pieces starting with this pn
		unsigned long		pnLvcFirst;					// 0x0088: the page number of the lowest numbered page in the document that records LVC FKP information
		unsigned long		cpnBteLvc;					// 0x008C: count of LVC FKPs recorded in file. In non-complex files if the number of entries in the plcfbtePapx is less than this, the plcfbtePapx is incomplete.
		unsigned long		fcIslandFirst;				// 0x0090: ?
		unsigned long		fcIslandLim;				// 0x0094: ?
		unsigned short		cfclcb;						// 0x0098: Number of fields in the array of FC/LCB pairs.
		union {
			unsigned long	rgfclcb;					// 0x009A: Beginning of array of FC/LCB pairs.
			unsigned long	fcStshfOrig;				// 0x009A: file offset of original allocation for STSH in table stream. During fast save Word will attempt to reuse this allocation if STSH is small enough to fit.
		} u009A;
		unsigned long		lcbStshfOrig;				// 0x009E: count of bytes of original STSH allocation
		unsigned long		fcStshf;					// 0x00A2: offset of STSH in table stream.
		unsigned long		lcbStshf;					// 0x00A6: count of bytes of current STSH allocation
		unsigned long		fcPlcffndRef;				// 0x00AA: offset in table stream of footnote reference PLCF of FRD structures. CPs in PLC are relative to main document text stream and give location of footnote references.
		unsigned long		lcbPlcffndRef;				// 0x00AE: count of bytes of footnote reference PLC== 0 if no footnotes defined in document.
		unsigned long		fcPlcffndTxt;				// 0x00B2: offset in table stream of footnote text PLC. CPs in PLC are relative to footnote subdocument text stream and give location of beginnings of footnote text for corresponding references recorded in plcffndRef. No structure is stored in this plc. There will just be n+1 FC entries in this PLC when there are n footnotes
		unsigned long		lcbPlcffndTxt;				// 0x00B6: count of bytes of footnote text PLC. == 0 if no footnotes defined in document
		unsigned long		fcPlcfandRef;				// 0x00BA: offset in table stream of annotation reference ATRD PLC. The CPs recorded in this PLC give the offset of annotation references in the main document.
		unsigned long		lcbPlcfandRef;				// 0x00BE: count of bytes of annotation reference PLC.
		unsigned long		fcPlcfandTxt;				// 0x00C2: offset in table stream of annotation text PLC. The Cps recorded in this PLC give the offset of the annotation text in the annotation sub document corresponding to the references stored in the plcfandRef. There is a 1 to 1 correspondence between entries recorded in the plcfandTxt and the plcfandRef. No structure is stored in this PLC.
		unsigned long		lcbPlcfandTxt;				// 0x00C6: count of bytes of the annotation text PLC
		unsigned long		fcPlcfsed;					// 0x00CA: offset in table stream of section descriptor SED PLC. CPs in PLC are relative to main document.
		unsigned long		lcbPlcfsed;					// 0x00CE: count of bytes of section descriptor PLC.
		unsigned long		fcPlcpad;					// 0x00D2: ?
		unsigned long		lcbPlcpad;					// 0x00D6: ?
		unsigned long		fcPlcfphe;					// 0x00DA: offset in table stream of PHE PLC of paragraph heights. CPs in PLC are relative to main document text stream. Only written for files in complex format. Should not be written by third party creators of Word files. 
		unsigned long		lcbPlcfphe;					// 0x00DE: count of bytes of paragraph height PLC. ==0 when file is non-complex.
		unsigned long		fcSttbfglsy;				// 0x00E2: offset in table stream of glossary string table. This table consists of Pascal style strings (strings stored prefixed with a length byte) concatenated one after another.
		unsigned long		lcbSttbfglsy;				// 0x00E6: count of bytes of glossary string table. == 0 for non-glossary documents.!=0 for glossary documents
		unsigned long		fcPlcfglsy;					// 0x00EA: offset in table stream of glossary PLC. CPs in PLC are relative to main document and mark the beginnings of glossary entries and are in 1-1 correspondence with entries of sttbfglsy. No structure is stored in this PLC. There will be n+1 FC entries in this PLC when there are n glossary entries.
		unsigned long		lcbPlcfglsy;				// 0x00EE: count of bytes of glossary PLC.== 0 for non-glossary documents.!=0 for glossary documents.
		unsigned long		fcPlcfhdd;					// 0x00F2: byte offset in table stream of header HDD PLC. CPs are relative to header subdocument and mark the beginnings of individual headers in the header subdocument. No structure is stored in this PLC. There will be n+1 FC entries in this PLC when there are n headers stored for the document.
		unsigned long		lcbPlcfhdd;					// 0x00F6: count of bytes of header PLC. == 0 if document contains no headers
		unsigned long		fcPlcfbteChpx;				// 0x00FA: offset in table stream of character property bin table.PLC. FCs in PLC are file offsets in the main stream. Describes text of main document and all subdocuments.
		unsigned long		lcbPlcfbteChpx;				// 0x00FE: count of bytes of character property bin table PLC
		unsigned long		fcPlcfbtePapx;				// 0x0102: offset in table stream of paragraph property bin table.PLC. FCs in PLC are file offsets in the main stream. Describes text of main document and all subdocuments.
		unsigned long		lcbPlcfbtePapx;				// 0x0106: count of bytes of paragraph property bin table PLC
		unsigned long		fcPlcfsea;					// 0x010A: offset in table stream of PLC reserved for private use. The SEA is 6 bytes long.
		unsigned long		lcbPlcfsea;					// 0x010E: count of bytes of private use PLC.
		unsigned long		fcSttbfffn;					// 0x0112: offset in table stream of font information STTBF. The sttbfffn is a STTBF where is string is actually an FFN structure. The nth entry in the STTBF describes the font that will be displayed when the chp.ftc for text is equal to n. See the FFN file structure definition.
		unsigned long		lcbSttbfffn;				// 0x0116: count of bytes in sttbfffn.
		unsigned long		fcPlcffldMom;				// 0x011A: offset in table stream to the FLD PLC of field positions in the main document. The CPs point to the beginning CP of a field, the CP of field separator character inside a field and the ending CP of the field. A field may be nested within another field. 20 levels of field nesting are allowed.
		unsigned long		lcbPlcffldMom;				// 0x011E: count of bytes in plcffldMom
		unsigned long		fcPlcffldHdr;				// 0x0122: offset in table stream to the FLD PLC of field positions in the header subdocument.
		unsigned long		lcbPlcffldHdr;				// 0x0126: count of bytes in plcffldHdr
		unsigned long		fcPlcffldFtn;				// 0x012A: offset in table stream to the FLD PLC of field positions in the footnote subdocument.
		unsigned long		lcbPlcffldFtn;				// 0x012E: count of bytes in plcffldFtn
		unsigned long		fcPlcffldAtn;				// 0x0132: offset in table stream to the FLD PLC of field positions in the annotation subdocument.
		unsigned long		lcbPlcffldAtn;				// 0x0136: count of bytes in plcffldAtn
		unsigned long		fcPlcffldMcr;				// 0x013A: no longer used
		unsigned long		lcbPlcffldMcr;				// 0x013E: no longer used
		unsigned long		fcSttbfbkmk;				// 0x0142: offset in table stream of the STTBF that records bookmark names in the main document
		unsigned long		lcbSttbfbkmk;				// 0x0146: ?
		unsigned long		fcPlcfbkf;					// 0x014A: offset in table stream of the PLCF that records the beginning CP offsets of bookmarks in the main document. See BKF structure definition
		unsigned long		lcbPlcfbkf;					// 0x014E: ?
		unsigned long		fcPlcfbkl;					// 0x0152: offset in table stream of the PLCF that records the ending CP offsets of bookmarks recorded in the main document. No structure is stored in this PLCF.
		unsigned long		lcbPlcfbkl;					// 0x0156: ?
		unsigned long		fcCmds;						// 0x015A: offset in table stream of the macro commands. These commands are private and undocumented.
		unsigned long		lcbCmds;					// 0x015E: undocument size of undocument structure not documented above
		unsigned long		fcPlcmcr;					// 0x0162: ?
		unsigned long		lcbPlcmcr;					// 0x0166: ?
		unsigned long		fcSttbfmcr;					// 0x016A: ?
		unsigned long		lcbSttbfmcr;				// 0x016E: ?
		unsigned long		fcPrDrvr;					// 0x0172: offset in table stream of the printer driver information (names of drivers, port, etc.)
		unsigned long		lcbPrDrvr;					// 0x0176: count of bytes of the printer driver information (names of drivers, port, etc.)
		unsigned long		fcPrEnvPort;				// 0x017A: offset in table stream of the print environment in portrait mode.
		unsigned long		lcbPrEnvPort;				// 0x017E: count of bytes of the print environment in portrait mode.
		unsigned long		fcPrEnvLand;				// 0x0182: offset in table stream of the print environment in landscape mode.
		unsigned long		lcbPrEnvLand;				// 0x0186: count of bytes of the print environment in landscape mode.
		unsigned long		fcWss;						// 0x018A: offset in table stream of Window Save State data structure. WSS contains dimensions of document's main text window and the last selection made by Word user.
		unsigned long		lcbWss;						// 0x018E: count of bytes of WSS. ==0 if unable to store the window state. Should not be written by third party creators of Word files.
		unsigned long		fcDop;						// 0x0192: offset in table stream of document property data structure.
		unsigned long		lcbDop;						// 0x0196: count of bytes of document properties
		unsigned long		fcSttbfAssoc;				// 0x019A: offset in table stream of STTBF of associated strings. The strings in this table specify document summary info and the paths to special documents related to this document. See documentation of the STTBFASSOC
		unsigned long		lcbSttbfAssoc;				// 0x019E: ?
		unsigned long		fcClx;						// 0x01A2: offset in table stream of beginning of information for complex files. Consists of an encoding of all of the prms quoted by the document followed by the plcpcd (piece table) for the document.
		unsigned long		lcbClx;						// 0x01A6: count of bytes of complex file information == 0 if file is non-complex.
		unsigned long		fcPlcfpgdFtn;				// 0x01AA: ?
		unsigned long		lcbPlcfpgdFtn;				// 0x01AE: ?
		unsigned long		fcAutosaveSource;			// 0x01B2: offset in table stream of the name of the original file. fcAutosaveSource and cbAutosaveSource should both be 0 if autosave is off.
		unsigned long		lcbAutosaveSource;			// 0x01B6: count of bytes of the name of the original file.
		unsigned long		fcGrpXstAtnOwners;			// 0x01BA: offset in table stream of group of strings recording the names of the owners of annotations stored in the document
		unsigned long		lcbGrpXstAtnOwners;			// 0x01BE: count of bytes of the group of strings
		unsigned long		fcSttbfAtnbkmk;				// 0x01C2: offset in table stream of the sttbf that records names of bookmarks for the annotation subdocument
		unsigned long		lcbSttbfAtnbkmk;			// 0x01C6: length in bytes of the sttbf that records names of bookmarks for the annotation subdocument
		unsigned long		fcPlcdoaMom;				// 0x01CA: ?
		unsigned long		lcbPlcdoaMom;				// 0x01CE: ?
		unsigned long		fcPlcdoaHdr;				// 0x01D2: ?
		unsigned long		lcbPlcdoaHdr;				// 0x01D6: ?
		unsigned long		fcPlcspaMom;				// 0x01DA: offset in table stream of the FSPA PLC for main document. == 0 if document has no office art objects.
		unsigned long		lcbPlcspaMom;				// 0x01DE: length in bytes of the FSPA PLC of the main document.
		unsigned long		fcPlcspaHdr;				// 0x01E2: offset in table stream of the FSPA PLC for header document. == 0 if document has no office art objects.
		unsigned long		lcbPlcspaHdr;				// 0x01E6: length in bytes of the FSPA PLC of the header document.
		unsigned long		fcPlcfAtnbkf;				// 0x01EA: offset in table stream of BKF (bookmark first) PLC of the annotation subdocument
		unsigned long		lcbPlcfAtnbkf;				// 0x01EE: length in bytes of BKF (bookmark first) PLC of the annotation subdocument
		unsigned long		fcPlcfAtnbkl;				// 0x01F2: offset in table stream of BKL (bookmark last) PLC of the annotation subdocument
		unsigned long		lcbPlcfAtnbkl;				// 0x01F6: length in bytes of PLC marking the CP limits of the annotation bookmarks. No structure is stored in this PLC.
		unsigned long		fcPms;						// 0x01FA: offset in table stream of PMS (Print Merge State) information block. This contains the current state of a print merge operation
		unsigned long		lcbPms;						// 0x01FE: length in bytes of PMS. ==0 if no current print merge state. Should not be written by third party creators of Word files.
		unsigned long		fcFormFldSttbs;				// 0x0202: offset in table stream of form field Sttbf which contains strings used in form field dropdown controls
		unsigned long		lcbFormFldSttbs;			// 0x0206: length in bytes of form field Sttbf
		unsigned long		fcPlcfendRef;				// 0x020A: offset in table stream of endnote reference PLCF of FRD structures. CPs in PLCF are relative to main document text stream and give location of endnote references.
		unsigned long		lcbPlcfendRef;				// 0x020E: ?
		unsigned long		fcPlcfendTxt;				// 0x0212: offset in table stream of PlcfendRef which points to endnote text in the endnote document stream which corresponds with the plcfendRef. No structure is stored in this PLC.
		unsigned long		lcbPlcfendTxt;				// 0x0216: ?
		unsigned long		fcPlcffldEdn;				// 0x021A: offset in table stream to FLD PLCF of field positions in the endnote subdoc
		unsigned long		lcbPlcffldEdn;				// 0x021E: ?
		unsigned long		fcPlcfpgdEdn;				// 0x0222: ?
		unsigned long		lcbPlcfpgdEdn;				// 0x0226: ?
		unsigned long		fcDggInfo;					// 0x022A: offset in table stream of the office art object table data. The format of office art object table data is found in a separate document.
		unsigned long		lcbDggInfo;					// 0x022E: length in bytes of the office art object table data
		unsigned long		fcSttbfRMark;				// 0x0232: offset in table stream to STTBF that records the author abbreviations for authors who have made revisions in the document
		unsigned long		lcbSttbfRMark;				// 0x0236: ?
		unsigned long		fcSttbCaption;				// 0x023A: offset in table stream to STTBF that records caption titles used in the document.
		unsigned long		lcbSttbCaption;				// 0x023E: ?
		unsigned long		fcSttbAutoCaption;			// 0x0242: offset in table stream to the STTBF that records the object names and indices into the caption STTBF for objects which get auto captions.
		unsigned long		lcbSttbAutoCaption;			// 0x0246: ?
		unsigned long		fcPlcfwkb;					// 0x024A: offset in table stream to WKB PLCF that describes the boundaries of contributing documents in a master document
		unsigned long		lcbPlcfwkb;					// 0x024E: ?
		unsigned long		fcPlcfspl;					// 0x0252: offset in table stream of PLCF (of SPLS structures) that records spell check state
		unsigned long		lcbPlcfspl;					// 0x0256: ?
		unsigned long		fcPlcftxbxTxt;				// 0x025A: offset in table stream of PLCF that records the beginning CP in the text box subdoc of the text of individual text box entries. No structure is stored in this PLCF
		unsigned long		lcbPlcftxbxTxt;				// 0x025E: ?
		unsigned long		fcPlcffldTxbx;				// 0x0262: offset in table stream of the FLD PLCF that records field boundaries recorded in the textbox subdoc.
		unsigned long		lcbPlcffldTxbx;				// 0x0266: ?
		unsigned long		fcPlcfhdrtxbxTxt;			// 0x026A: offset in table stream of PLCF that records the beginning CP in the header text box subdoc of the text of individual header text box entries. No structure is stored in this PLC.
		unsigned long		lcbPlcfhdrtxbxTxt;			// 0x026E: ?
		unsigned long		fcPlcffldHdrTxbx;			// 0x0272: offset in table stream of the FLD PLCF that records field boundaries recorded in the header textbox subdoc.
		unsigned long		lcbPlcffldHdrTxbx;			// 0x0276: ?
		unsigned long		fcStwUser;					// 0x027A: Macro User storage
		unsigned long		lcbStwUser;					// 0x027E: ?
		unsigned long		fcSttbttmbd;				// 0x0282: offset in table stream of embedded true type font data.
		unsigned long		cbSttbttmbd;				// 0x0286: ?
		unsigned long		fcUnused;					// 0x028A: ?
		unsigned long		lcbUnused;					// 0x028E: ?
		union {
			unsigned long	rgpgdbkd;					// 0x0292: beginning of array of fcPgd / fcBkd pairs
			unsigned long	fcPgdMother;				// 0x0292: beginning of array of fcPgd / fcBkd pairs
		} u0292;
		unsigned long		lcbPgdMother;				// 0x0296: ?
		unsigned long		fcBkdMother;				// 0x029A: offset in table stream of the PLCF that records the break descriptors for the main text of the doc.
		unsigned long		lcbBkdMother;				// 0x029E: ?
		unsigned long		fcPgdFtn;					// 0x02A2: offset in table stream of the PLF that records the page descriptors for the footnote text of the doc.
		unsigned long		lcbPgdFtn;					// 0x02A6: ?
		unsigned long		fcBkdFtn;					// 0x02AA: offset in table stream of the PLCF that records the break descriptors for the footnote text of the doc.
		unsigned long		lcbBkdFtn;					// 0x02AE: ?
		unsigned long		fcPgdEdn;					// 0x02B2: offset in table stream of the PLF that records the page descriptors for the endnote text of the doc.
		unsigned long		lcbPgdEdn;					// 0x02B6: ?
		unsigned long		fcBkdEdn;					// 0x02BA: offset in table stream of the PLCF that records the break descriptors for the endnote text of the doc.
		unsigned long		lcbBkdEdn;					// 0x02BE: ?
		unsigned long		fcSttbfIntlFld;				// 0x02C2: offset in table stream of the STTBF containing field keywords. This is only used in a small number of the international versions of word. This field is no longer written to the file for nFib >= 167.
		unsigned long		lcbSttbfIntlFld;			// 0x02C6: Always 0 for nFib >= 167.
		unsigned long		fcRouteSlip;				// 0x02CA: offset in table stream of a mailer routing slip.
		unsigned long		lcbRouteSlip;				// 0x02CE: ?
		unsigned long		fcSttbSavedBy;				// 0x02D2: offset in table stream of STTBF recording the names of the users who have saved this document alternating with the save locations.
		unsigned long		lcbSttbSavedBy;				// 0x02D6: ?
		unsigned long		fcSttbFnm;					// 0x02DA: offset in table stream of STTBF recording filenames of documents which are referenced by this document.
		unsigned long		lcbSttbFnm;					// 0x02DE: ?
		unsigned long		fcPlcfLst;					// 0x02E2: offset in the table stream of list format information.
		unsigned long		lcbPlcfLst;					// 0x02E6: ?
		unsigned long		fcPlfLfo;					// 0x02EA: offset in the table stream of list format override information.
		unsigned long		lcbPlfLfo;					// 0x02EE: ?
		unsigned long		fcPlcftxbxBkd;				// 0x02F2: offset in the table stream of the textbox break table (a PLCF of BKDs) for the main document
		unsigned long		lcbPlcftxbxBkd;				// 0x02F6: ?
		unsigned long		fcPlcftxbxHdrBkd;			// 0x02FA: offset in the table stream of the textbox break table (a PLCF of BKDs) for the header subdocument
		unsigned long		lcbPlcftxbxHdrBkd;			// 0x02FE: ?
		unsigned long		fcDocUndo;					// 0x0302: offset in main stream of undocumented undo / versioning data
		unsigned long		lcbDocUndo;					// 0x0306: ?
		unsigned long		fcRgbuse;					// 0x030A: offset in main stream of undocumented undo / versioning data
		unsigned long		lcbRgbuse;					// 0x030E: ?
		unsigned long		fcUsp;						// 0x0312: offset in main stream of undocumented undo / versioning data
		unsigned long		lcbUsp;						// 0x0316: ?
		unsigned long		fcUskf;						// 0x031A: offset in table stream of undocumented undo / versioning data
		unsigned long		lcbUskf;					// 0x031E: ?
		unsigned long		fcPlcupcRgbuse;				// 0x0322: offset in table stream of undocumented undo / versioning data
		unsigned long		lcbPlcupcRgbuse;			// 0x0326: ?
		unsigned long		fcPlcupcUsp;				// 0x032A: offset in table stream of undocumented undo / versioning data
		unsigned long		lcbPlcupcUsp;				// 0x032E: ?
		unsigned long		fcSttbGlsyStyle;			// 0x0332: offset in table stream of string table of style names for glossary entries
		unsigned long		lcbSttbGlsyStyle;			// 0x0336: ?
		unsigned long		fcPlgosl;					// 0x033A: offset in table stream of undocumented grammar options PL
		unsigned long		lcbPlgosl;					// 0x033E: ?
		unsigned long		fcPlcocx;					// 0x0342: offset in table stream of undocumented ocx data
		unsigned long		lcbPlcocx;					// 0x0346: ?
		unsigned long		fcPlcfbteLvc;				// 0x034A: offset in table stream of character property bin table.PLC. FCs in PLC are file offsets. Describes text of main document and all subdocuments.
		unsigned long		lcbPlcfbteLvc;				// 0x034E: ?
		unsigned long		dwLowDateTime;				// 0x0352: ftModified.low 32 bits
		unsigned long		dwHighDateTime;				// 0x0356: ftModified.high 32 bits
		unsigned long		fcPlcflvc;					// 0x035A: offset in table stream of LVC PLCF
		unsigned long		lcbPlcflvc;					// 0x035E: size of LVC PLCF, ==0 for non-complex files
		unsigned long		fcPlcasumy;					// 0x0362: offset in table stream of autosummary ASUMY PLCF.
		unsigned long		lcbPlcasumy;				// 0x0366: ?
		unsigned long		fcPlcfgram;					// 0x036A: offset in table stream of PLCF (of SPLS structures) which records grammar check state
		unsigned long		lcbPlcfgram;				// 0x036E: ?
		unsigned long		fcSttbListNames;			// 0x0372: offset in table stream of list names string table
		unsigned long		lcbSttbListNames;			// 0x0376: ?
		unsigned long		fcSttbfUssr;				// 0x037A: offset in table stream of undocumented undo / versioning data
		unsigned long		lcbSttbfUssr;				// 0x037E: ?
		// TODO: MORE! This header is HUGE!
	} FIB;
	// wMagicCreated values
	enum {
		wMagicCreated__MSWord = 0x6A62,
	};
	// enum: flags_0x000A
	enum {
		fDot =				0x0001,						// 1=is template document
		fGlsy =				0x0002,						// 1=is glossary
		fComplex =			0x0004,						// 1=file is in complex fast-saved format
		fHasPic =			0x0008,						// 1=file has one or more pictures
		cQuickSaves =		0x00F0,						// count of times file was quicksaved
		fEncrypted =		0x0100,						// 1=file is encrypted
		fWhichTblStm =		0x0200,						// 1=use stream "1Table" 0=use stream "0Table"
		fReadOnlyRecommended = 0x0400,					// 1=user recommends that file be readonly
		fWriteReservation =	0x0800,						// 1=file owner has made the file write reserved
		fExtChar =			0x1000,						// 1=uses extended characters
		fLoadOverride =		0x2000,						// ?
		fFarEast =			0x4000,						// ?
		fCrypto =			0x8000,						// ?
	};
	// enum: fMac
	enum {
		fMac =				0x01,						// 1=file was last saved in Macintosh environment
		fEmptySpecial =		0x02,						// ?
		fLoadOverridePage =	0x04,						// ?
		fFutureSavedUndo =	0x08,						// ?
		fWord97Saved =		0x10,						// ?
		fSpare0 =			0xFE,						// fEmptySpecial | fLoadOverridePage | fFutureSavedUndo | fWord97Saved | ???
	};
public:
	int Assign(msolereader::entity* Word,msolereader::entity* table0,msolereader::entity* table1);
	int ReadFIB(MSWord97Reader::FIB *fib);
public:
	msolereader::entity*	WordDocument;
	msolereader::entity*	Table[2];
};

MSWord97Reader::MSWord97Reader()
{
}

MSWord97Reader::~MSWord97Reader()
{
}

// WARNING: The FIB in Word 97 Documents is HUGE! (898 bytes according to MSDN docs)
int MSWord97Reader::ReadFIB(MSWord97Reader::FIB *fib)
{
	unsigned char buf[898];

	if (!WordDocument) return -1;
	if (WordDocument->seek(0) != 0) return -1;
	if (WordDocument->read(buf,sizeof(buf)) < sizeof(buf)) return -1;
	memset(fib,0,sizeof(MSWord97Reader::FIB));

	/* OK start reading the values */
	fib->wIdent =					llei16(buf + 0x0000);
	fib->nFib =						llei16(buf + 0x0002);
	fib->nProduct =					llei16(buf + 0x0004);
	fib->lid =						llei16(buf + 0x0006);
	fib->pnNext =					llei16(buf + 0x0008);
	fib->flags_0x000A =				llei16(buf + 0x000A);
	fib->nFibBack =					llei16(buf + 0x000C);
	fib->lKey =						llei32(buf + 0x000E);
	fib->envr =						       buf  [0x0012];
	fib->fMac =							   buf  [0x0013];
	fib->chs =						llei16(buf + 0x0014);
	fib->chsTables =				llei16(buf + 0x0016);
	fib->fcMin =					llei32(buf + 0x0018);		/* [comment #1] */
	fib->fcMac =					llei32(buf + 0x001C);		/* [comment #1] */
	fib->csw =						llei16(buf + 0x0020);
	fib->u0022.wMagicCreated =		llei16(buf + 0x0022);
	fib->wMagicRevised =			llei16(buf + 0x0024);
	fib->wMagicCreatedPrivate =		llei16(buf + 0x0026);
	fib->wMagicRevisedPrivate =		llei16(buf + 0x0028);
	fib->pnFbpChpFirst_W6 =			llei16(buf + 0x002A);
	fib->pnChpFirst_W6 =			llei16(buf + 0x002C);
	fib->cpnBteChp_W6 =				llei16(buf + 0x002E);
	fib->pnFbpPapFirst_W6 =			llei16(buf + 0x0030);
	fib->pnPapFirst_W6 =			llei16(buf + 0x0032);
	fib->cpnBtePap_W6 =				llei16(buf + 0x0034);
	fib->pnFbpLvcFirst_W6 =			llei16(buf + 0x0036);
	fib->pnLvcFirst_W6 =			llei16(buf + 0x0038);
	fib->cpnBteLvc_W6 =				llei16(buf + 0x003A);
	fib->lidFE =					llei16(buf + 0x003C);
	fib->clw =						llei16(buf + 0x003E);
	fib->u0040.rglw =				llei32(buf + 0x0040);
	fib->lProductCreated =			llei32(buf + 0x0044);
	fib->lProductRevised =			llei32(buf + 0x0048);
	fib->ccpText =					llei32(buf + 0x004C);
	fib->ccpFtn =					llei32(buf + 0x0050);
	fib->ccpHdd =					llei32(buf + 0x0054);
	fib->ccpMcr =					llei32(buf + 0x0058);
	fib->ccpAtn =					llei32(buf + 0x005C);
	fib->ccpEdn =					llei32(buf + 0x0060);
	fib->ccpTxbx =					llei32(buf + 0x0064);
	fib->ccpHdrTxbx =				llei32(buf + 0x0068);
	fib->pnFbpChpFirst =			llei32(buf + 0x006C);
	fib->pnChpFirst =				llei32(buf + 0x0070);
	fib->cpnBteChp =				llei32(buf + 0x0074);
	fib->pnFbpPapFirst =			llei32(buf + 0x0078);
	fib->pnPapFirst =				llei32(buf + 0x007C);
	fib->cpnBtePap =				llei32(buf + 0x0080);
	fib->pnFbpLvcFirst =			llei32(buf + 0x0084);
	fib->pnLvcFirst =				llei32(buf + 0x0088);
	fib->cpnBteLvc =				llei32(buf + 0x008C);
	fib->fcIslandFirst =			llei32(buf + 0x0090);
	fib->fcIslandLim =				llei32(buf + 0x0094);
	fib->cfclcb =					llei16(buf + 0x0098);
	fib->u009A.rgfclcb =			llei32(buf + 0x009A);
	fib->lcbStshfOrig =				llei32(buf + 0x009E);
	fib->fcStshf =					llei32(buf + 0x00A2);
	fib->lcbStshf =					llei32(buf + 0x00A6);
	fib->fcPlcffndRef =				llei32(buf + 0x00AA);
	fib->lcbPlcffndRef =			llei32(buf + 0x00AE);
	fib->fcPlcffndTxt =				llei32(buf + 0x00B2);
	fib->lcbPlcffndTxt =			llei32(buf + 0x00B6);
	fib->fcPlcfandRef =				llei32(buf + 0x00BA);
	fib->lcbPlcfandRef =			llei32(buf + 0x00BE);
	fib->fcPlcfandTxt =				llei32(buf + 0x00C2);
	fib->lcbPlcfandTxt =			llei32(buf + 0x00C6);
	fib->fcPlcfsed =				llei32(buf + 0x00CA);
	fib->lcbPlcfsed =				llei32(buf + 0x00CE);
	fib->fcPlcpad =					llei32(buf + 0x00D2);
	fib->lcbPlcpad =				llei32(buf + 0x00D6);
	fib->fcPlcfphe =				llei32(buf + 0x00DA);
	fib->lcbPlcfphe =				llei32(buf + 0x00DE);
	fib->fcSttbfglsy =				llei32(buf + 0x00E2);
	fib->lcbSttbfglsy =				llei32(buf + 0x00E6);
	fib->fcPlcfglsy =				llei32(buf + 0x00EA);
	fib->lcbPlcfglsy =				llei32(buf + 0x00EE);
	fib->fcPlcfhdd =				llei32(buf + 0x00F2);
	fib->lcbPlcfhdd =				llei32(buf + 0x00F6);
	fib->fcPlcfbteChpx =			llei32(buf + 0x00FA);
	fib->lcbPlcfbteChpx =			llei32(buf + 0x00FE);
	fib->fcPlcfbtePapx =			llei32(buf + 0x0102);
	fib->lcbPlcfbtePapx =			llei32(buf + 0x0106);
	fib->fcPlcfsea =				llei32(buf + 0x010A);
	fib->lcbPlcfsea =				llei32(buf + 0x010E);
	fib->fcSttbfffn =				llei32(buf + 0x0112);
	fib->lcbSttbfffn =				llei32(buf + 0x0116);
	fib->fcPlcffldMom =				llei32(buf + 0x011A);
	fib->lcbPlcffldMom =			llei32(buf + 0x011E);
	fib->fcPlcffldHdr =				llei32(buf + 0x0122);
	fib->lcbPlcffldHdr =			llei32(buf + 0x0126);
	fib->fcPlcffldFtn =				llei32(buf + 0x012A);
	fib->lcbPlcffldFtn =			llei32(buf + 0x012E);
	fib->fcPlcffldAtn =				llei32(buf + 0x0132);
	fib->lcbPlcffldAtn =			llei32(buf + 0x0136);
	fib->fcPlcffldMcr =				llei32(buf + 0x013A);
	fib->lcbPlcffldMcr =			llei32(buf + 0x013E);
	fib->fcSttbfbkmk =				llei32(buf + 0x0142);
	fib->lcbSttbfbkmk =				llei32(buf + 0x0146);
	fib->fcPlcfbkf =				llei32(buf + 0x014A);
	fib->lcbPlcfbkf =				llei32(buf + 0x014E);
	fib->fcPlcfbkl =				llei32(buf + 0x0152);
	fib->lcbPlcfbkl =				llei32(buf + 0x0156);
	fib->fcCmds =					llei32(buf + 0x015A);
	fib->lcbCmds =					llei32(buf + 0x015E);
	fib->fcPlcmcr =					llei32(buf + 0x0162);
	fib->lcbPlcmcr =				llei32(buf + 0x0166);
	fib->fcSttbfmcr =				llei32(buf + 0x016A);
	fib->lcbSttbfmcr =				llei32(buf + 0x016E);
	fib->fcPrDrvr =					llei32(buf + 0x0172);
	fib->lcbPrDrvr =				llei32(buf + 0x0176);
	fib->fcPrEnvPort =				llei32(buf + 0x017A);
	fib->lcbPrEnvPort =				llei32(buf + 0x017E);
	fib->fcPrEnvLand =				llei32(buf + 0x0182);
	fib->lcbPrEnvLand =				llei32(buf + 0x0186);
	fib->fcWss =					llei32(buf + 0x018A);
	fib->lcbWss =					llei32(buf + 0x018E);
	fib->fcDop =					llei32(buf + 0x0192);
	fib->lcbDop =					llei32(buf + 0x0196);
	fib->fcSttbfAssoc =				llei32(buf + 0x019A);
	fib->lcbSttbfAssoc =			llei32(buf + 0x019E);
	fib->fcClx =					llei32(buf + 0x01A2);
	fib->lcbClx =					llei32(buf + 0x01A6);
	fib->fcPlcfpgdFtn =				llei32(buf + 0x01AA);
	fib->lcbPlcfpgdFtn =			llei32(buf + 0x01AE);
	fib->fcAutosaveSource =			llei32(buf + 0x01B2);
	fib->lcbAutosaveSource =		llei32(buf + 0x01B6);
	fib->fcGrpXstAtnOwners =		llei32(buf + 0x01BA);
	fib->lcbGrpXstAtnOwners =		llei32(buf + 0x01BE);
	fib->fcSttbfAtnbkmk =			llei32(buf + 0x01C2);
	fib->lcbSttbfAtnbkmk =			llei32(buf + 0x01C6);
	fib->fcPlcdoaMom =				llei32(buf + 0x01CA);
	fib->lcbPlcdoaMom =				llei32(buf + 0x01CE);
	fib->fcPlcdoaHdr =				llei32(buf + 0x01D2);
	fib->lcbPlcdoaHdr =				llei32(buf + 0x01D6);
	fib->fcPlcspaMom =				llei32(buf + 0x01DA);
	fib->lcbPlcspaMom =				llei32(buf + 0x01DE);
	fib->fcPlcspaHdr =				llei32(buf + 0x01E2);
	fib->lcbPlcspaHdr =				llei32(buf + 0x01E6);
	fib->fcPlcfAtnbkf =				llei32(buf + 0x01EA);
	fib->lcbPlcfAtnbkf =			llei32(buf + 0x01EE);
	fib->fcPlcfAtnbkl =				llei32(buf + 0x01F2);
	fib->lcbPlcfAtnbkl =			llei32(buf + 0x01F6);
	fib->fcPms =					llei32(buf + 0x01FA);
	fib->lcbPms =					llei32(buf + 0x01FE);
	fib->fcFormFldSttbs =			llei32(buf + 0x0202);
	fib->lcbFormFldSttbs =			llei32(buf + 0x0206);
	fib->fcPlcfendRef =				llei32(buf + 0x020A);
	fib->lcbPlcfendRef =			llei32(buf + 0x020E);
	fib->fcPlcfendTxt =				llei32(buf + 0x0212);
	fib->lcbPlcfendTxt =			llei32(buf + 0x0216);
	fib->fcPlcffldEdn =				llei32(buf + 0x021A);
	fib->lcbPlcffldEdn =			llei32(buf + 0x021E);
	fib->fcPlcfpgdEdn =				llei32(buf + 0x0222);
	fib->lcbPlcfpgdEdn =			llei32(buf + 0x0226);
	fib->fcDggInfo =				llei32(buf + 0x022A);
	fib->lcbDggInfo =				llei32(buf + 0x022E);
	fib->fcSttbfRMark =				llei32(buf + 0x0232);
	fib->lcbSttbfRMark =			llei32(buf + 0x0236);
	fib->fcSttbCaption =			llei32(buf + 0x023A);
	fib->lcbSttbCaption =			llei32(buf + 0x023E);
	fib->fcSttbAutoCaption =		llei32(buf + 0x0242);
	fib->lcbSttbAutoCaption =		llei32(buf + 0x0246);
	fib->fcPlcfwkb =				llei32(buf + 0x024A);
	fib->lcbPlcfwkb =				llei32(buf + 0x024E);
	fib->fcPlcfspl =				llei32(buf + 0x0252);
	fib->lcbPlcfspl =				llei32(buf + 0x0256);
	fib->fcPlcftxbxTxt =			llei32(buf + 0x025A);
	fib->lcbPlcftxbxTxt =			llei32(buf + 0x025E);
	fib->fcPlcffldTxbx =			llei32(buf + 0x0262);
	fib->lcbPlcffldTxbx =			llei32(buf + 0x0266);
	fib->fcPlcfhdrtxbxTxt =			llei32(buf + 0x026A);
	fib->lcbPlcfhdrtxbxTxt =		llei32(buf + 0x026E);
	fib->fcPlcffldHdrTxbx =			llei32(buf + 0x0272);
	fib->lcbPlcffldHdrTxbx =		llei32(buf + 0x0276);
	fib->fcStwUser =				llei32(buf + 0x027A);
	fib->lcbStwUser =				llei32(buf + 0x027E);
	fib->fcSttbttmbd =				llei32(buf + 0x0282);
	fib->cbSttbttmbd =				llei32(buf + 0x0286);
	fib->fcUnused =					llei32(buf + 0x028A);
	fib->lcbUnused =				llei32(buf + 0x028E);
	fib->u0292.rgpgdbkd =			llei32(buf + 0x0292);
	fib->lcbPgdMother =				llei32(buf + 0x0296);
	fib->fcBkdMother =				llei32(buf + 0x029A);
	fib->lcbBkdMother =				llei32(buf + 0x029E);
	fib->fcPgdFtn =					llei32(buf + 0x02A2);
	fib->lcbPgdFtn =				llei32(buf + 0x02A6);
	fib->fcBkdFtn =					llei32(buf + 0x02AA);
	fib->lcbBkdFtn =				llei32(buf + 0x02AE);
	fib->fcPgdEdn =					llei32(buf + 0x02B2);
	fib->lcbPgdEdn =				llei32(buf + 0x02B6);
	fib->fcBkdEdn =					llei32(buf + 0x02BA);
	fib->lcbBkdEdn =				llei32(buf + 0x02BE);
	fib->fcSttbfIntlFld =			llei32(buf + 0x02C2);
	fib->lcbSttbfIntlFld =			llei32(buf + 0x02C6);
	fib->fcRouteSlip =				llei32(buf + 0x02CA);
	fib->lcbRouteSlip =				llei32(buf + 0x02CE);
	fib->fcSttbSavedBy =			llei32(buf + 0x02D2);
	fib->lcbSttbSavedBy =			llei32(buf + 0x02D6);
	fib->fcSttbFnm =				llei32(buf + 0x02DA);
	fib->lcbSttbFnm =				llei32(buf + 0x02DE);
	fib->fcPlcfLst =				llei32(buf + 0x02E2);
	fib->lcbPlcfLst =				llei32(buf + 0x02E6);
	fib->fcPlfLfo =					llei32(buf + 0x02EA);
	fib->lcbPlfLfo =				llei32(buf + 0x02EE);
	fib->fcPlcftxbxBkd =			llei32(buf + 0x02F2);
	fib->lcbPlcftxbxBkd =			llei32(buf + 0x02F6);
	fib->fcPlcftxbxHdrBkd =			llei32(buf + 0x02FA);
	fib->lcbPlcftxbxHdrBkd =		llei32(buf + 0x02FE);
	fib->fcDocUndo =				llei32(buf + 0x0302);
	fib->lcbDocUndo =				llei32(buf + 0x0306);
	fib->fcRgbuse =					llei32(buf + 0x030A);
	fib->lcbRgbuse =				llei32(buf + 0x030E);
	fib->fcUsp =					llei32(buf + 0x0312);
	fib->lcbUsp =					llei32(buf + 0x0316);
	fib->fcUskf =					llei32(buf + 0x031A);
	fib->lcbUskf =					llei32(buf + 0x031E);
	fib->fcPlcupcRgbuse =			llei32(buf + 0x0322);
	fib->lcbPlcupcRgbuse =			llei32(buf + 0x0326);
	fib->fcPlcupcUsp =				llei32(buf + 0x032A);
	fib->lcbPlcupcUsp =				llei32(buf + 0x032E);
	fib->fcSttbGlsyStyle =			llei32(buf + 0x0332);
	fib->lcbSttbGlsyStyle =			llei32(buf + 0x0336);
	fib->fcPlgosl =					llei32(buf + 0x033A);
	fib->lcbPlgosl =				llei32(buf + 0x033E);
	fib->fcPlcocx =					llei32(buf + 0x0342);
	fib->lcbPlcocx =				llei32(buf + 0x0346);
	fib->fcPlcfbteLvc =				llei32(buf + 0x034A);
	fib->lcbPlcfbteLvc =			llei32(buf + 0x034E);
	fib->dwLowDateTime =			llei32(buf + 0x0352);
	fib->dwHighDateTime =			llei32(buf + 0x0356);
	fib->fcPlcflvc =				llei32(buf + 0x035A);
	fib->lcbPlcflvc =				llei32(buf + 0x035E);
	fib->fcPlcasumy =				llei32(buf + 0x0362);
	fib->lcbPlcasumy =				llei32(buf + 0x0366);
	fib->fcPlcfgram =				llei32(buf + 0x036A);
	fib->lcbPlcfgram =				llei32(buf + 0x036E);
	fib->fcSttbListNames =			llei32(buf + 0x0372);
	fib->lcbSttbListNames =			llei32(buf + 0x0376);
	fib->fcSttbfUssr =				llei32(buf + 0x037A);
	fib->lcbSttbfUssr =				llei32(buf + 0x037E);

/* [comment #1]: Office 2003: These values fcMin and fcMac are ignored. For some reason though Word sets
                 fcMin = 0x600 and fcMac = 0x805 where fcMac is correct but fcMin is off by 0x200 bytes.
				 Changing these values doesn't really do anything to Word or Wordpad's ability to display
				 the text, suggesting that these values are ignored and that the REAL values are somewhere
				 else? */

	return 0;
}

int MSWord97Reader::Assign(msolereader::entity* Word,msolereader::entity* table0,msolereader::entity* table1)
{
	/* the Word parameter must be != NULL */
	if (Word == NULL) return -1;

	/* either table0 or table1 can be == NULL but not both */
	if (table0 == NULL && table1 == NULL) return -1;

	WordDocument = Word;
	Table[0] = table0;
	Table[1] = table1;

	return 0;
}

void ExamineFile(MSWord97Reader *word97)
{
	MSWord97Reader::FIB fib;

	if (word97->ReadFIB(&fib) < 0) {
		printf("This Word document carries a bad FIB!\n");
		return;
	}

#define I_AM_LAZYD(x,y) printf("  " x " = %u\n",fib.##y);
#define I_AM_LAZYX(x,y) printf("  " x " = 0x%08X\n",fib.##y);

	printf("FIB:\n");
	I_AM_LAZYX("wIdent",wIdent);
	I_AM_LAZYD("nFib",nFib);
	I_AM_LAZYX("nProduct",nProduct);
	I_AM_LAZYX("lid",lid);
	I_AM_LAZYX("pnNext",pnNext);
	I_AM_LAZYX("flags_0x000A",flags_0x000A);
	I_AM_LAZYX("nFibBack",nFibBack);
	I_AM_LAZYX("lKey",lKey);
	I_AM_LAZYX("envr",envr);
#if 0
	fib->fMac
	fib->chs
	fib->chsTables
	fib->fcMin
	fib->fcMac
	fib->csw
	fib->u0022.wMagicCreated
	fib->wMagicRevised
	fib->wMagicCreatedPrivate
	fib->wMagicRevisedPrivate
	fib->pnFbpChpFirst_W6
	fib->pnChpFirst_W6
	fib->cpnBteChp_W6
	fib->pnFbpPapFirst_W6
	fib->pnPapFirst_W6
	fib->cpnBtePap_W6
	fib->pnFbpLvcFirst_W6
	fib->pnLvcFirst_W6
	fib->cpnBteLvc_W6
	fib->lidFE
	fib->clw
	fib->u0040.rglw
	fib->lProductCreated
	fib->lProductRevised
	fib->ccpText
#endif

#undef I_AM_LAZY
}

int main(int argc,char **argv)
{
	MSWord97Reader word97;
	msolereader::entity* WordDocument = NULL;
	msolereader::entity* Table[2];
	msolereader::entity* Root = NULL;
	msolereader::entity* ActiveTable = NULL;
	SomeFileStdio sfs;
	msolereader r;

	if (argc < 2) {
		printf("%s <OLE container file>\n");
		return 1;
	}

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n");
		return 1;
	}

	r.sf = &sfs;
	if (!r.init()) return 1;
	if (!r.mount()) return 1;

	/* first open all Microsoft Word related streams */
	Root = r.file("Root Entry",NULL,0,msolereader::entity::ATTR_SUBDIRECTORY);
	if (!Root) return 1;

	WordDocument = r.file("WordDocument",Root,0,0);
	Table[0] = r.file("0Table",Root,0,0);
	Table[1] = r.file("1Table",Root,0,0);

	if (word97.Assign(WordDocument,Table[0],Table[1]) >= 0)
		ExamineFile(&word97);
	else
		printf("msword97::assign() problem\n");

	if (WordDocument) delete WordDocument;
	if (Table[0]) delete Table[0];
	if (Table[1]) delete Table[1];
	if (Root) delete Root;
	r.umount();
	r.free();
	return 0;
}
