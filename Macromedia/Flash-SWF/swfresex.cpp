/* SWF resource extraction tool.
   can be used to extract images, sounds, etc. from a SWF movie */

// TODO: Support exporting audio in formats other than MP3
// TODO: Support exporting non-JPEG images (DefineBitsLossless tags)

#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "common/zlib/zlib.h"
#include "Macromedia/Flash-SWF/cmfswftags.h"
#include <string.h>

class StreamingExport {
public:
	StreamingExport(char *name,MacromediaFlashSWF::SoundStreamHead_Header *ssh);
	~StreamingExport();
	void OneBlock(unsigned long ofs,unsigned long sz,SomeFileStdio *sfs);
private:
	void OneBlockMP3(unsigned long ofs,unsigned long sz,SomeFileStdio *sfs);
public:
	SomeFileStdio								of;
	MacromediaFlashSWF::SoundStreamHead_Header	ssh;
};

StreamingExport::StreamingExport(char *name,MacromediaFlashSWF::SoundStreamHead_Header *_ssh)
{
	memcpy(&ssh,_ssh,sizeof(MacromediaFlashSWF::SoundStreamHead_Header));
	if (of.Create(name) < 0)
		printf("Error creating %s\n",name);
}

StreamingExport::~StreamingExport()
{
}

void StreamingExport::OneBlock(unsigned long ofs,unsigned long sz,SomeFileStdio *sfs)
{
	if (ssh.StreamSoundCompression == MacromediaFlashSWF::DefineSound_Format::MP3)
		OneBlockMP3(ofs,sz,sfs);
}

void StreamingExport::OneBlockMP3(unsigned long ofs,unsigned long sz,SomeFileStdio *sfs)
{
	unsigned char buf[1024];
	int r;

	/* skip the header */
	if (sz <= 2) return;
	ofs += 2;
	sz -= 2;

	if (sfs->Seek(ofs) != ofs) return;

	/* Flash movies actually have 4 bytes of stuff before the MP3 header,
	   though Macromedia documents two bytes. We could export it verbatim
	   but then many MP3 players will not play the file because they don't
	   see the MPEG sync byte. */
	if (sfs->Read(buf,4) < 4) return;
	for (r=0;r < 4 && buf[r] != 0xFF;) r++;
	if (sz <= r) return;
	ofs += r;
	sz -= r;
	if (sfs->Seek(ofs) != ofs) return;

	while (sz >= sizeof(buf)) {
		r = sfs->Read(buf,sizeof(buf));
		if (r > 0) {
			of.Write(buf,r);
			sz -= r;
		}
		else {
			sz = 0;
			break;
		}
	}

	if (sz > 0) {
		r = sfs->Read(buf,sz);
		if (r > 0) {
			of.Write(buf,r);
			sz -= r;
		}
	}
}

void ExportMP3Data(char *name,unsigned long ofs,unsigned long sz,MacromediaFlashSWF::DefineSound_Header* dsh,SomeFileStdio *sfs)
{
	unsigned char buf[1024];
	SomeFileStdio of;
	int r;

	if (sfs->Seek(ofs) != ofs) return;

	if (of.Create(name) < 0) {
		printf("Cannot create %s!\n",name);
		return;
	}

	while (sz >= sizeof(buf)) {
		r = sfs->Read(buf,sizeof(buf));
		if (r > 0) {
			of.Write(buf,r);
			sz -= r;
		}
		else {
			sz = 0;
			break;
		}
	}

	if (sz > 0) {
		r = sfs->Read(buf,sz);
		if (r > 0) {
			of.Write(buf,r);
			sz -= r;
		}
	}
}

int main(int argc,char **argv)
{
	MacromediaFlashSWF::FileHeader swfhdr;
	unsigned char *JPEGTable=NULL;
	StreamingExport* mp3ex=NULL;
	unsigned long TagOfs,TagWas;
	unsigned int JPEGTableSz=0;
	unsigned char buf[512];
	char baseName[512];
	SomeFileStdio sfs;
	int r;

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n",argv[1]);
		return 1;
	}

	if (sfs.Seek(0) != 0 || sfs.Read(buf,4) < 4) {
		printf("file problem\n");
		return 1;
	}

	if ((r=MacromediaFlashSWF::ReadHeader(buf,4,&swfhdr)) < 0) {
		printf("Not a SWF file\n");
		return 1;
	}
	if (r == 1) {
		printf("The SWF file is compressed. Use swfunzip to decompress it.\n");
		return 1;
	}

	if ((r=sfs.Read(buf+4,26)) < 9) {
		printf("file problem\n");
		return 1;
	}

	if (MacromediaFlashSWF::ReadFileHeader(buf,r+4,&swfhdr) < 0) {
		printf("Problem reading the file header\n");
		return 1;
	}

	TagOfs = swfhdr.FirstTagOffset;
	while (sfs.Seek(TagOfs) == TagOfs && (r=sfs.Read(buf,6)) >= 2) {
		MacromediaFlashSWF::BitsNBytes swfbnb(buf,r);
		MacromediaFlashSWF::TagReader swftag(&swfbnb);

		if (swftag.ReadTag() < 0)
			break;

		TagWas = TagOfs;
		TagOfs += swftag.TotalLength;
		if (TagOfs > sfs.GetSize())
			break;

		if (swftag.Code == MacromediaFlashSWF::Tag::Protect) {
			printf("*** Import Protect detected. The author of this movie very likely\n");
			printf("    does not want you to export resources from his work. Please do\n");
			printf("    not plagurize other people's work!\n");
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::DefineSprite) {
			unsigned long subTagOfs = TagWas + swftag.TagHeaderLength;
			unsigned long subTagEnd = subTagOfs + swftag.Length;
			unsigned long subTagWas;
			unsigned short int SpriteID;
			unsigned short int FrameCount;
			StreamingExport* smp3ex=NULL;

			if (sfs.Seek(subTagOfs) == subTagOfs && (r=sfs.Read(buf,4)) == 4) {
				swfbnb.Assign(buf,4);
				SpriteID = swfbnb.UI16();
				FrameCount = swfbnb.UI16();
				subTagOfs += 4;
				printf("Found a sprite or movie clip: ID %u FrameCount %u\n",SpriteID,FrameCount);
				while (subTagOfs <= (subTagEnd-2) && sfs.Seek(subTagOfs) == subTagOfs && (r=sfs.Read(buf,6)) >= 2) {
					MacromediaFlashSWF::BitsNBytes subswfbnb(buf,r);
					MacromediaFlashSWF::TagReader subswftag(&subswfbnb);

					if (subswftag.ReadTag() < 0)
						break;

					subTagWas = subTagOfs;
					subTagOfs += subswftag.TotalLength;
					if (subTagOfs > sfs.GetSize())
						break;

					if (subswftag.Code == MacromediaFlashSWF::Tag::SoundStreamHead ||
						subswftag.Code == MacromediaFlashSWF::Tag::SoundStreamHead2) {
						unsigned long o = TagWas + subswftag.TagHeaderLength;

						if (!smp3ex) {
							if (sfs.Seek(o) == o && (r=sfs.Read(buf,6)) == 6) {
								MacromediaFlashSWF::BitsNBytes swfbnb(buf,r);
								MacromediaFlashSWF::SoundStreamHead_Header ssh;

								if (MacromediaFlashSWF::ReadSoundStreamHead_Header(&swfbnb,&ssh) >= 0) {
									if (ssh.StreamSoundCompression == MacromediaFlashSWF::DefineSound_Format::MP3) {
										sprintf(baseName,"%s--Sprite-%u-Stream.mp3",argv[1],SpriteID);
										smp3ex = new StreamingExport(baseName,&ssh);
									}
								}
							}
						}
					}
					else if (subswftag.Code == MacromediaFlashSWF::Tag::SoundStreamBlock) {
						unsigned long o = TagWas + subswftag.TagHeaderLength;
						if (smp3ex) smp3ex->OneBlock(o,subswftag.Length,&sfs);
					}
				}

				SpriteID++;
				if (smp3ex) delete smp3ex;
				smp3ex=NULL;
			}
		}
		else if (	swftag.Code == MacromediaFlashSWF::Tag::SoundStreamHead ||
					swftag.Code == MacromediaFlashSWF::Tag::SoundStreamHead2) {
			unsigned long o = TagWas + swftag.TagHeaderLength;

			if (!mp3ex) {
				if (sfs.Seek(o) == o && (r=sfs.Read(buf,6)) == 6) {
					MacromediaFlashSWF::BitsNBytes swfbnb(buf,r);
					MacromediaFlashSWF::SoundStreamHead_Header ssh;

					if (MacromediaFlashSWF::ReadSoundStreamHead_Header(&swfbnb,&ssh) >= 0) {
						if (ssh.StreamSoundCompression == MacromediaFlashSWF::DefineSound_Format::MP3) {
							sprintf(baseName,"%s--Stream.mp3",argv[1]);
							mp3ex = new StreamingExport(baseName,&ssh);
						}
					}
				}
			}
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::SoundStreamBlock) {
			unsigned long o = TagWas + swftag.TagHeaderLength;
			if (mp3ex) mp3ex->OneBlock(o,swftag.Length,&sfs);
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::DefineSound) {
			unsigned long o = TagWas + swftag.TagHeaderLength;
			unsigned long SoundData = o + 7,SoundDataSize;

			if (sfs.Seek(o) == o && (r=sfs.Read(buf,7)) == 7) {
				MacromediaFlashSWF::BitsNBytes swfbnb(buf,r);
				MacromediaFlashSWF::DefineSound_Header dsh;

				if (MacromediaFlashSWF::ReadDefineSound_Header(&swfbnb,&dsh) >= 0) {
					SoundDataSize = swftag.Length;

					if (dsh.SoundFormat == MacromediaFlashSWF::DefineSound_Format::Uncompressed) {
						printf("DefineSound SoundID=%u Uncompressed (platform specific), not supported currently\n",dsh.SoundId);
						// possibly platform-dependent uncompressed PCM.
						// ignore for now.
					}
					else if (dsh.SoundFormat == MacromediaFlashSWF::DefineSound_Format::ADPCM) {
						printf("DefineSound SoundID=%u ADPCM, not supported currently\n",dsh.SoundId);
						// ADPCM audio.
						// TODO
					}
					else if (dsh.SoundFormat == MacromediaFlashSWF::DefineSound_Format::MP3) {
						printf("DefineSound SoundID=%u MP3\n",dsh.SoundId);
						// MP3
						sprintf(baseName,"%s--DefineSound--%u--%u.mp3",argv[1],dsh.SoundId,SoundData);
						ExportMP3Data(baseName,SoundData,SoundDataSize,&dsh,&sfs);
					}
					else if (dsh.SoundFormat == MacromediaFlashSWF::DefineSound_Format::UncompressedLE) {
						printf("DefineSound SoundID=%u Uncompressed, not supported currently\n",dsh.SoundId);
						// Uncompressed PCM little endian
						// TODO
					}
					else if (dsh.SoundFormat == MacromediaFlashSWF::DefineSound_Format::Nellymoser) {
						printf("DefineSound SoundID=%u Nellymoser, not supported currently\n",dsh.SoundId);
						// Nellymoser
						// ignore for now. Does anyone know how to parse this?
					}
					else {
						printf("DefineSound SoundID=%u tag offset=%u: Unknown SoundFormat\n",dsh.SoundId,TagWas);
					}
				}
			}
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::JPEGTables) {
			if (!JPEGTable) {
				JPEGTableSz = swftag.Length;
				JPEGTable = new unsigned char[JPEGTableSz];

				if (JPEGTable) {
					unsigned long o = TagWas + swftag.TagHeaderLength;
					if (sfs.Seek(o) == o && (r=sfs.Read(JPEGTable,JPEGTableSz)) >= 2) {
						/* remove the EOI marker at the end so we can catencate this and the data
						   in DefineBits to make a complete JPEG */
						int x = r - 2;
						while (x >= 2 && !(JPEGTable[x] == 0xFF && JPEGTable[x+1] == 0xD9)) x--;
						JPEGTableSz = x;
					}
				}
			}
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::DefineBits) {
			unsigned long o = TagWas + swftag.TagHeaderLength;
			unsigned long sz = swftag.Length;

			if (JPEGTable) {
				if (sfs.Seek(o) == o && (r=sfs.Read(buf,2)) == 2) {
					MacromediaFlashSWF::BitsNBytes subswfbnb(buf,r);
					unsigned short CharId;
					SomeFileStdio of;
					int i=0;

					CharId = subswfbnb.UI16();
					sprintf(baseName,"%s--DefineBits--%u--%u.jpg",argv[1],CharId,o);
					of.Create(baseName);
					of.Write(JPEGTable,JPEGTableSz);
					printf("DefineBits character ID=%u\n",CharId);
					o += 2;

					if (CharId == 31) {
						CharId--;
						CharId++;
					}

					if (sz >= 2) {
						unsigned char buf[1024];
						sz -= 2;

						if (sfs.Seek(o) == o) {
							while (sz > 0) {
								r = sz > sizeof(buf) ? sizeof(buf) : sz;
								r = sfs.Read(buf,r);
								if (r > 0) {
									sz -= r;

									if (i == 0) {
										/* filter out EOI marker(s) */
										while (r >= 2 && buf[0] == 0xFF && buf[1] == 0xD9) {
											r -= 2;
											memmove(buf,buf+2,r);
										}
										/* filter out SOI marker */
										if (buf[0] == 0xFF && buf[1] == 0xD8) {
											r -= 2;
											memmove(buf,buf+2,r);
										}
									}

									of.Write(buf,r);
								}

								i++;
							}
						}
					}
				}
			}
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::DefineBitsJPEG2) {
			unsigned long o = TagWas + swftag.TagHeaderLength;
			unsigned long sz = swftag.Length;

			if (JPEGTable) {
				if (sfs.Seek(o) == o && (r=sfs.Read(buf,2)) == 2) {
					MacromediaFlashSWF::BitsNBytes subswfbnb(buf,r);
					unsigned short CharId;
					SomeFileStdio of;
					int i=0;

					CharId = subswfbnb.UI16();
					sprintf(baseName,"%s--DefineBitsJPEG2--%u--%u.jpg",argv[1],CharId,o);
					of.Create(baseName);
					printf("DefineBitsJPEG2 character ID=%u\n",CharId);
					o += 2;
					if (sz >= 2) {
						unsigned char buf[4096];
						sz -= 2;

						if (sfs.Seek(o) == o) {
							while (sz > 0) {
								r = sz > sizeof(buf) ? sizeof(buf) : sz;
								r = sfs.Read(buf,r);
								if (r > 0) {
									sz -= r;

									if (i == 0) {
										/* remove extra EOI+SOI between JPEG table and image data */
										int x;

										for (x=0;x < (sz-4);x++) {
											if (buf[x  ] == 0xFF && buf[x+1] == 0xD9 &&
												buf[x+2] == 0xFF && buf[x+3] == 0xD8) {
												memmove(buf+x,buf+x+4,r-(x+4));
												r -= 4;
												break;
											}
										}
									}

									of.Write(buf,r);
								}

								i++;
							}
						}
					}
				}
			}
		}
		else if (swftag.Code == MacromediaFlashSWF::Tag::DefineBitsJPEG3) {
			unsigned long o = TagWas + swftag.TagHeaderLength;
			unsigned long sz = swftag.Length;

			if (JPEGTable) {
				if (sfs.Seek(o) == o && (r=sfs.Read(buf,6)) == 6) {
					MacromediaFlashSWF::BitsNBytes subswfbnb(buf,r);
					unsigned short CharId;
					SomeFileStdio of;
					int i=0;

					CharId = subswfbnb.UI16();
					subswfbnb.UI32(); // skip alpha offset
					sprintf(baseName,"%s--DefineBitsJPEG3--%u--%u.jpg",argv[1],CharId,o);
					of.Create(baseName);
					printf("DefineBitsJPEG2 character ID=%u\n",CharId);
					o += 6;
					if (sz >= 6) {
						unsigned char buf[4096];
						sz -= 6;

						if (sfs.Seek(o) == o) {
							while (sz > 0) {
								r = sz > sizeof(buf) ? sizeof(buf) : sz;
								r = sfs.Read(buf,r);
								if (r > 0) {
									sz -= r;

									if (i == 0) {
										/* remove extra EOI+SOI between JPEG table and image data */
										int x;

										for (x=0;x < (sz-4);x++) {
											if (buf[x  ] == 0xFF && buf[x+1] == 0xD9 &&
												buf[x+2] == 0xFF && buf[x+3] == 0xD8) {
												memmove(buf+x,buf+x+4,r-(x+4));
												r -= 4;
												break;
											}
										}
									}

									of.Write(buf,r);
								}

								i++;
							}
						}
					}
				}
			}
		}
	}

	if (JPEGTable) delete JPEGTable;
	JPEGTable=NULL;
	if (mp3ex) delete mp3ex;
	mp3ex=NULL;
}
