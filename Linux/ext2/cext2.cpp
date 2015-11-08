
/* a C++ class for reading the Linux ext2/ext3 filesystem
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 */

#include "CEXT2.h"
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

// BEGIN CEXT2 namespace
namespace CEXT2 {
////////////////////////////////////////////////

int reader::init()
{
	mounted=0;
	tmpe2block=NULL;
	root_dir=NULL;
	return 1;
}

int reader::freeMe()
{
	if (mounted) umount();
	return 1;
}

int reader::GetSuperBlock(unsigned char *tmp,int sz,int sbn)
{
	super_block* sb = (super_block*)tmp;
	int i,descbi,block;

	if (CEXT2_16(sb->s_magic) != EXT2_SUPER_MAGIC) {
		comment(C_ERROR,"Invalid/missing superblock: Magic value unknown");
		return 0;
	}

	theSuperBlock.s_inodes_count =							CEXT2_32(sb->s_inodes_count);
	theSuperBlock.s_blocks_count =							CEXT2_32(sb->s_blocks_count);
	theSuperBlock.s_r_blocks_count =						CEXT2_32(sb->s_r_blocks_count);
	theSuperBlock.s_free_blocks_count =						CEXT2_32(sb->s_free_blocks_count);
	theSuperBlock.s_free_inodes_count =						CEXT2_32(sb->s_free_inodes_count);
	theSuperBlock.s_first_data_block =						CEXT2_32(sb->s_first_data_block);
	theSuperBlock.s_log_block_size =						CEXT2_32(sb->s_log_block_size);
	theSuperBlock.s_log_frag_size =							CEXT2_32(sb->s_log_frag_size);
	theSuperBlock.s_blocks_per_group =						CEXT2_32(sb->s_blocks_per_group);
	theSuperBlock.s_frags_per_group =						CEXT2_32(sb->s_frags_per_group);
	theSuperBlock.s_inodes_per_group =						CEXT2_32(sb->s_inodes_per_group);
	theSuperBlock.s_mtime =									CEXT2_32(sb->s_mtime);
	theSuperBlock.s_wtime =									CEXT2_32(sb->s_wtime);
	theSuperBlock.s_mnt_count =								CEXT2_16(sb->s_mnt_count);
	theSuperBlock.s_max_mnt_count =							CEXT2_16(sb->s_max_mnt_count);
	theSuperBlock.s_magic =									CEXT2_16(sb->s_magic);
	theSuperBlock.s_state =									CEXT2_16(sb->s_state);
	theSuperBlock.s_errors =								CEXT2_16(sb->s_errors);
	theSuperBlock.s_minor_rev_level =						CEXT2_16(sb->s_minor_rev_level);
	theSuperBlock.s_lastcheck =								CEXT2_32(sb->s_lastcheck);
	theSuperBlock.s_checkinterval =							CEXT2_32(sb->s_checkinterval);
	theSuperBlock.s_creator_os =							CEXT2_32(sb->s_creator_os);
	theSuperBlock.s_rev_level =								CEXT2_32(sb->s_rev_level);
	theSuperBlock.s_def_resuid =							CEXT2_16(sb->s_def_resuid);
	theSuperBlock.s_def_resgid =							CEXT2_16(sb->s_def_resgid);
	theSuperBlock.s_first_ino =								CEXT2_32(sb->s_first_ino);
	theSuperBlock.s_inode_size =							CEXT2_16(sb->s_inode_size);
	theSuperBlock.s_block_group_nr =						CEXT2_16(sb->s_block_group_nr);
	theSuperBlock.s_feature_compat =						CEXT2_32(sb->s_feature_compat);
	theSuperBlock.s_feature_incompat =						CEXT2_32(sb->s_feature_incompat);
	theSuperBlock.s_feature_ro_compat =						CEXT2_32(sb->s_feature_ro_compat);
	memcpy(theSuperBlock.s_uuid,									 sb->s_uuid,16);
	memcpy(theSuperBlock.s_volume_name,								 sb->s_volume_name,16);
	memcpy(theSuperBlock.s_last_mounted,							 sb->s_last_mounted,64);
	theSuperBlock.s_algorithm_usage_bitmap =				CEXT2_32(sb->s_algorithm_usage_bitmap);
	theSuperBlock.s_prealloc_blocks =								 sb->s_prealloc_blocks;
	theSuperBlock.s_prealloc_dir_blocks =							 sb->s_prealloc_dir_blocks;
	theSuperBlock.s_padding1 =								CEXT2_16(sb->s_padding1);
	memcpy(theSuperBlock.s_journal_uuid,							 sb->s_journal_uuid,16);
	theSuperBlock.s_journal_inum =							CEXT2_32(sb->s_journal_inum);
	theSuperBlock.s_journal_dev =							CEXT2_32(sb->s_journal_dev);
	theSuperBlock.s_last_orphan =							CEXT2_32(sb->s_last_orphan);
	for (i=0;i < 4;i++) theSuperBlock.s_hash_seed[i] =		CEXT2_32(sb->s_hash_seed[i]);
	theSuperBlock.s_def_hash_version =								 sb->s_def_hash_version;
	theSuperBlock.s_reserved_char_pad =								 sb->s_reserved_char_pad;
	theSuperBlock.s_reserved_word_pad =						CEXT2_16(sb->s_reserved_word_pad);
	theSuperBlock.s_default_mount_opts =					CEXT2_32(sb->s_default_mount_opts);
	theSuperBlock.s_first_meta_bg =							CEXT2_32(sb->s_first_meta_bg);
	for (i=0;i < 190;i++) theSuperBlock.s_reserved[i] =		CEXT2_32(sb->s_reserved[i]);

	if (theSuperBlock.s_log_block_size > 2) {
		comment(C_ERROR,"Bad block size");
		return 0;
	}
	if (theSuperBlock.s_log_frag_size > 2) {
		comment(C_ERROR,"Bad frag size");
		return 0;
	}

	// various warnings
	if (theSuperBlock.s_rev_level == 0) {
		if (theSuperBlock.s_feature_compat != 0 ||
			theSuperBlock.s_feature_incompat != 0 ||
			theSuperBlock.s_feature_ro_compat != 0) {
			comment(C_WARNING,"This ext2 filesystem has revision 0 but compatibile/incompatible/ro-compatible flags set?");
		}
	}

	e2blocksize = 1024 << theSuperBlock.s_log_block_size;
	e2fs_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
	e2fs_first_inode = EXT2_GOOD_OLD_FIRST_INO;
	if (theSuperBlock.s_rev_level != 0) {
		e2fs_inode_size = theSuperBlock.s_inode_size;
		e2fs_first_inode = theSuperBlock.s_first_ino;
		if (e2fs_inode_size < EXT2_GOOD_OLD_INODE_SIZE ||
			(!IsPow2(e2fs_inode_size)) ||
			e2fs_inode_size >= e2blocksize) {
			comment(C_ERROR,"Invalid inode+first node values");
			return 0;
		}
	}

	tmpe2block = new unsigned char[e2blocksize];
	if (!tmpe2block) {
		comment(C_ERROR,"Not enough memory");
		return 0;
	}

	e2fsfragsize = 1024 << theSuperBlock.s_log_frag_size;
	e2fs_frags_per_block = e2blocksize / e2fsfragsize;
	e2fs_inodes_per_block = e2blocksize / e2fs_inode_size;
	e2fs_itb_per_group = theSuperBlock.s_inodes_per_group / e2fs_inodes_per_block;
	e2fs_desc_per_block = e2blocksize / 32;	// 32 = sizeof(group_desc)
	e2fs_addr_per_block_bits = 10 + theSuperBlock.s_log_block_size;
	e2fs_desc_per_block_bits = 10 + theSuperBlock.s_log_frag_size;

	if (e2blocksize != e2fsfragsize) {
		comment(C_ERROR,"frag size != blocksize condition not supported");
		return 0;
	}
 
	if (theSuperBlock.s_blocks_per_group > (e2blocksize * 8)) {
		comment(C_ERROR,"Too many blocks/group");
		return 0;
	}
	if (theSuperBlock.s_frags_per_group > (e2blocksize * 8)) {
		comment(C_ERROR,"Too many blocks/group");
		return 0;
	}
	if (theSuperBlock.s_inodes_per_group > (e2blocksize * 8)) {
		comment(C_ERROR,"Too many blocks/group");
		return 0;
	}
	if (theSuperBlock.s_blocks_per_group == 0) {
		comment(C_ERROR,"No blocks/group");
		return 0;
	}
	if (e2fs_desc_per_block == 0) {
		comment(C_ERROR,"No blocks/group");
		return 0;
	}

	// HACK: This need to make non-1024 block size fs work?
	if (theSuperBlock.s_first_data_block == 0)
		e2fs_first_data_block = 1;
	else
		e2fs_first_data_block = theSuperBlock.s_first_data_block;

	e2fs_groups_count = theSuperBlock.s_blocks_count - theSuperBlock.s_first_data_block;
	e2fs_groups_count = ((e2fs_groups_count + theSuperBlock.s_blocks_per_group) - 1) / theSuperBlock.s_blocks_per_group;
	e2fs_db_count = ((e2fs_groups_count + e2fs_desc_per_block) - 1) / e2fs_desc_per_block;
	esuperblockN1k = sbn;
	esuperblockN = (sbn * 1024) / e2blocksize;

	block = theSuperBlock.s_first_data_block;
	for (i=0,descbi=0;i < e2fs_groups_count;i++) {
		group_desc *groupie;
		unsigned char *buf;

		if ((i % e2fs_desc_per_block) == 0) {
			buf = read_group_descriptor_sector(descbi++);
			if (!buf) {
				comment(C_ERROR,"Cannot read group descriptors");
				return 0;
			}
		}

		groupie = (group_desc*)buf;
		buf += 32;

		if (CEXT2_32(groupie->bg_block_bitmap) < block || CEXT2_32(groupie->bg_block_bitmap) >= (block + theSuperBlock.s_blocks_per_group)) {
			comment(C_ERROR,"Block descriptor out of range");
			return 0;
		}
		if (CEXT2_32(groupie->bg_inode_bitmap) < block || CEXT2_32(groupie->bg_inode_bitmap) >= (block + theSuperBlock.s_blocks_per_group)) {
			comment(C_ERROR,"Inode descriptor out of range");
			return 0;
		}
		if (CEXT2_32(groupie->bg_inode_table) < block || CEXT2_32(groupie->bg_inode_table) >= (block + theSuperBlock.s_blocks_per_group)) {
			comment(C_ERROR,"Inode descriptor out of range");
			return 0;
		}

		block += theSuperBlock.s_blocks_per_group;
		groupie++;
	}

	return 1;
}

int reader::GetGroupDesc(uint64 N,group_desc *g)
{
	int i;

	if (!e2blockread((N / e2fs_desc_per_block) + e2fs_first_data_block + esuperblockN,1,tmpe2block)) return 0;
	group_desc *ng = (group_desc *)(tmpe2block + ((N % e2fs_desc_per_block) * 32));

	g->bg_block_bitmap =			CEXT2_32(ng->bg_block_bitmap);
	g->bg_inode_bitmap =			CEXT2_32(ng->bg_inode_bitmap);
	g->bg_inode_table =				CEXT2_32(ng->bg_inode_table);

	g->bg_free_blocks_count =		CEXT2_16(ng->bg_free_blocks_count);
	g->bg_free_inodes_count =		CEXT2_16(ng->bg_free_inodes_count);
	g->bg_used_dirs_count =			CEXT2_16(ng->bg_used_dirs_count);
	g->bg_pad =						CEXT2_16(ng->bg_pad);

	for (i=0;i < 3;i++)
		g->bg_reserved[i] =			CEXT2_32(ng->bg_reserved[i]);

	return 1;
}

int reader::GetInode(uint64 N,inode *i)
{
	if (N == 0) return 0;
	if (N != EXT2_ROOT_INO && N < e2fs_first_inode) return 0;
	N--;
	if (N >= theSuperBlock.s_inodes_count) return 0;

	int block_group = (int)(N / theSuperBlock.s_inodes_per_group);
	group_desc group;
	if (!GetGroupDesc(block_group,&group)) return 0;
	uint64 offset = (N % theSuperBlock.s_inodes_per_group) * e2fs_inode_size;
	uint64 block = group.bg_inode_table + (offset / e2blocksize);

	if (!e2blockread(block,1,tmpe2block)) return 0;
	inode *src = (inode*)(tmpe2block + (offset % e2blocksize));
	memcpy(i,src,sizeof(inode));
	return 1;
}

static inline int test_root(int a, int b)
{
        int num = b;

        while (a > num)
                num *= b;
        return num == a;
}

static int ext2_group_sparse(int group)
{
        if (group <= 1)
                return 1;
        return (test_root(group, 3) || test_root(group, 5) ||
                test_root(group, 7));
}

/**
 *      ext2_bg_has_super - number of blocks used by the superblock in group
 *      @sb: superblock for filesystem
 *      @group: group number to check
 *
 *      Return the number of blocks used by the superblock (primary or backup)
 *      in this group.  Currently this will be only 0 or 1.
 */
int reader::ext2_bg_has_super(int group)
{
	if ((theSuperBlock.s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) && !ext2_group_sparse(group))
		return 0;

	return 1;
}

unsigned char* reader::read_group_descriptor_sector(uint64 sector)
{
	if (sector >= e2fs_db_count) return NULL;

	if (!(theSuperBlock.s_feature_incompat & EXT2_FEATURE_INCOMPAT_META_BG) || sector < theSuperBlock.s_first_meta_bg) {
		sector += esuperblockN + 1;
	}
	else {
		uint64 bg = e2fs_desc_per_block * sector;
		int has_super = ext2_bg_has_super((int)sector);
		sector = e2fs_first_data_block + has_super + (bg * theSuperBlock.s_blocks_per_group);
	}

	if (!e2blockread(sector,1,tmpe2block)) return NULL;
	return tmpe2block;
}

// Argh there seems to be a slight inconsistiency about
// block size from place to place in fs/ext2/super.c, Linux kernel 2.6.13.2.
// in some places it apparently means 1024, in other places it means
// 1024 << log_block_size!
int	reader::e2blockread(uint64 sector,int N,unsigned char *buffer)
{
	return blockread((sector * e2blocksize) >> 9,(N * e2blocksize) >> 9,buffer);
}

int	reader::e2blockread1k(uint64 sector,int N,unsigned char *buffer)
{
	return blockread(sector << 1,N << 1,buffer);
}

int reader::mount()
{
	if (mounted) {
		comment(C_ERROR,"Already mounted!");
		return 0;
	}

	sectsize=blocksize();
	e2blocksize=1024;
	if (sectsize != 512) {
		comment(C_ERROR,"Block sizes != 512 not supported yet!");
		return 0;
	}

	tmpe2block = new unsigned char[4096];
	if (!tmpe2block) {
		comment(C_ERROR,"Out of memory");
		return 0;
	}

// FIXME: the superblock is found at block 1. the fact that the ext2/ext3 driver
//        in Linux takes sb= parameter suggests that you can have this anywhere?
	mounted=1;
	if (!e2blockread(1,1,tmpe2block)) {
		comment(C_ERROR,"Unable to read first sector!");
		umount();
		return 0;
	}

	if (!GetSuperBlock(tmpe2block,e2blocksize,1)) {
		comment(C_ERROR,"Not an ext2/ext3 filesystem or invalid superblock!");
		umount();
		return 0;
	}

	root_dir = new readdir(this);
	if (!root_dir) return 0;
	if (!root_dir->get_inode(EXT2_ROOT_INO)) return 0;
	return 1;
}

int reader::umount()
{
	if (!mounted) {
		comment(C_ERROR,"Already not mounted!");
		return 0;
	}

	if (root_dir) {
		delete root_dir;
		root_dir=NULL;
	}

	if (tmpe2block) {
		delete tmpe2block;
		tmpe2block=NULL;
	}

	mounted=0;
	return 1;
}

reader::readdir::readdir(reader *r)
{
	 indirect_map = NULL;			 indirect_map_fence = NULL;
	dindirect_map = NULL;			dindirect_map_fence = NULL;
	tindirect_map = NULL;			tindirect_map_fence = NULL;
	the_inode_yes = 0;
	mom = r;
	buffer = NULL;
	bufferfence = NULL;
	ptr = NULL;
}

reader::readdir::~readdir()
{
	if (indirect_map)	delete indirect_map;	 indirect_map = NULL;			 indirect_map_fence = NULL;
	if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;			dindirect_map_fence = NULL;
	if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;			tindirect_map_fence = NULL;
	if (buffer) delete buffer;
	buffer=NULL;
}

int	reader::readdir::reset()
{
	if (buffer) delete buffer;
	buffer=NULL;

	if (!mom->GetInode(the_inode_N,&the_inode)) return 0;

	buffer = new unsigned char[mom->e2blocksize];
	if (!buffer) return 0;
	bufferfence = buffer + mom->e2blocksize;
	ptr = bufferfence;
	block_n = 0;

	if ((cur_block = the_inode.i_block[block_n++]) == 0)
		return 0;

	if (!mom->e2blockread(cur_block,1,buffer))
		return 0;

	if (indirect_map)	delete indirect_map;	 indirect_map = NULL;			 indirect_map_fence = NULL;
	if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;			dindirect_map_fence = NULL;
	if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;			tindirect_map_fence = NULL;

	ptr = buffer;
	pblock_n = 0;
	the_inode_yes = 1;
	ent_raw_max = 8;
	ent_raw_n = 0;
	return 1;
}

int	reader::readdir::get_inode(uint64 in)
{
	the_inode_N=in;
	return reset();
}

int reader::readdir::read()
{
again:
	int esz = ent_raw_max - ent_raw_n;
	int sz = bufferfence - ptr;
	if (sz > esz) sz = esz;
	char do_again=0;

	if (ent_raw_max > sizeof(ent_raw))
		return 0;

	if (!the_inode_yes)
		return 0;

	if (sz > 0) {
		memcpy(ent_raw + ent_raw_n,ptr,sz);
		ent_raw_n += sz;
		ptr += sz;
	}

	if (ent_raw_max == 8 && ent_raw_n == 8) {
		memset(&cur_dirent,0,sizeof(cur_dirent));
		cur_dirent.inode =					llei32(	ent_raw +  0);
		cur_dirent.rec_len =				llei16(	ent_raw +  4);
		cur_dirent.name_len =						ent_raw[   6];
		cur_dirent.file_type =						ent_raw[   7];

		/* sanity checks */
		if (cur_dirent.inode != 0 &&
			cur_dirent.rec_len >= 8 &&
			cur_dirent.rec_len <= 4096) {
			ent_raw_max = cur_dirent.rec_len;
			do_again = 1;
		}
		else {
			the_inode_yes = 0;
			return 0;
		}
	}
	else if (ent_raw_n >= ent_raw_max && ent_raw_max >= 8) {
		if (ent_raw_max > (255+8)) ent_raw_max = 255+8;
		memcpy(cur_dirent.name,ent_raw+8,ent_raw_max-8);
		cur_dirent.null_end = 0;
		ent_raw_n = 0;
		ent_raw_max = 8;
		return 1;
	}

	if (ptr >= bufferfence) {
again_adv:
		int ard=1;
		char badv=1;
		memset(buffer,0,mom->e2blocksize);

		if (indirect_map)
			if (llei32(indirect_map_ptr) == 0)
				indirect_map_ptr = indirect_map_fence;

		if (dindirect_map)
			if (llei32(dindirect_map_ptr) == 0)
				dindirect_map_ptr = dindirect_map_fence;

		if (tindirect_map)
			if (llei32(tindirect_map_ptr) == 0)
				tindirect_map_ptr = tindirect_map_fence;

		if (indirect_map)
			if (indirect_map_ptr < indirect_map_fence)
				badv=0;

		if (dindirect_map)
			if (dindirect_map_ptr < dindirect_map_fence)
				badv=0;

		if (tindirect_map)
			if (tindirect_map_ptr < tindirect_map_fence)
				badv=0;

		if (badv) {
			if (block_n < EXT2_N_BLOCKS) {
				if ( indirect_map)	delete  indirect_map;	 indirect_map = NULL;	 indirect_map_fence = NULL;	 indirect_map_ptr = NULL;
				if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;	dindirect_map_fence = NULL;	dindirect_map_ptr = NULL;
				if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;	tindirect_map_fence = NULL;	tindirect_map_ptr = NULL;

				if ((cur_block = the_inode.i_block[pblock_n = block_n++]) != 0)		ard = mom->e2blockread(cur_block,1,buffer);
				else																ard = 0;
			}
			else {
				ard = 0;
			}
		}

		if (ard) {
			if (pblock_n == EXT2_TIND_BLOCK) {
				if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map) && (tindirect_map_ptr >= tindirect_map_fence || !tindirect_map)) {
					// what we just read is an indirect map, so copy it and load the first one
					if (!tindirect_map) tindirect_map = new unsigned char[mom->e2blocksize];
					if (!tindirect_map) return 0;
					tindirect_map_fence = tindirect_map + mom->e2blocksize;
					memcpy(tindirect_map,buffer,mom->e2blocksize);
					tindirect_map_ptr = tindirect_map;
					// load the first into the buffer, then let the next if statement do it's thing
					memcpy(tindirect_map,buffer,mom->e2blocksize);
				}
				if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map) && tindirect_map) {
					cur_block = llei32(tindirect_map_ptr);
					if (cur_block == 0) goto again_adv;
					tindirect_map_ptr += 4;
					memset(buffer,0,mom->e2blocksize);
					mom->e2blockread(cur_block,1,buffer);
				}
			}
			if (pblock_n >= EXT2_DIND_BLOCK) {
				if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map)) {
					// what we just read is an indirect map, so copy it and load the first one
					if (!dindirect_map) dindirect_map = new unsigned char[mom->e2blocksize];
					if (!dindirect_map) return 0;
					dindirect_map_fence = dindirect_map + mom->e2blocksize;
					dindirect_map_ptr = dindirect_map;
					// load the first into the buffer, then let the next if statement do it's thing
					memcpy(dindirect_map,buffer,mom->e2blocksize);
				}
				if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && dindirect_map) {
					cur_block = llei32(dindirect_map_ptr);
					if (cur_block == 0) goto again_adv;
					dindirect_map_ptr += 4;
					memset(buffer,0,mom->e2blocksize);
					mom->e2blockread(cur_block,1,buffer);
				}
			}
			if (pblock_n >= EXT2_IND_BLOCK) {
				if (indirect_map_ptr >= indirect_map_fence || !indirect_map) {
					// what we just read is an indirect map, so copy it and load the first one
					if (!indirect_map) indirect_map = new unsigned char[mom->e2blocksize];
					if (!indirect_map) return 0;
					indirect_map_fence = indirect_map + mom->e2blocksize;
					indirect_map_ptr = indirect_map;
					// load the first into the buffer, then let the next if statement do it's thing
					memcpy(indirect_map,buffer,mom->e2blocksize);
				}
				if (indirect_map) {
					cur_block = llei32(indirect_map_ptr);
					if (cur_block == 0) goto again_adv;
					indirect_map_ptr += 4;
					memset(buffer,0,mom->e2blocksize);
					mom->e2blockread(cur_block,1,buffer);
				}
			}
		}

		ptr = buffer;
	}

	if (do_again || 1)
		goto again;

	return 0;
}

reader::readdir* reader::GetRoot()
{
	return root_dir;
}

uint64 reader::GetFileSize(inode *i)
{
	uint64 res;

	res = i->i_size;
	if ((i->i_mode & EXT2__S_IFMT) == EXT2__S_IFREG)
		res |= ((uint64)i->i_dir_acl) << ((uint64)32);

	return res;
}

reader::file::file(reader *r)
{
	 indirect_map = NULL;			 indirect_map_fence = NULL;
	dindirect_map = NULL;			dindirect_map_fence = NULL;
	tindirect_map = NULL;			tindirect_map_fence = NULL;
	the_inode_yes = 0;
	mom = r;
	buffer = NULL;
	bufferfence = NULL;
	ptr = NULL;
}

reader::file::~file()
{
	if (indirect_map)	delete indirect_map;	 indirect_map = NULL;			 indirect_map_fence = NULL;
	if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;			dindirect_map_fence = NULL;
	if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;			tindirect_map_fence = NULL;
	if (buffer) delete buffer;
	buffer=NULL;
}

int	reader::file::reset()
{
	if (buffer) delete buffer;
	buffer=NULL;

	if (!mom->GetInode(the_inode_N,&the_inode)) return 0;

	buffer = new unsigned char[mom->e2blocksize];
	if (!buffer) return 0;
	bufferfence = buffer + mom->e2blocksize;
	ptr = bufferfence;
	block_n = 0;

	if ((cur_block = the_inode.i_block[block_n++]) == 0)
		return 0;

	if (the_inode.i_block[12] != 0)
		the_inode.i_block[12] = the_inode.i_block[12];

	buffer_block = 0;
	if (!mom->e2blockread(cur_block,1,buffer))
		return 0;

	if (indirect_map)	delete indirect_map;	 indirect_map = NULL;			 indirect_map_fence = NULL;
	if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;			dindirect_map_fence = NULL;
	if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;			tindirect_map_fence = NULL;

	ptr = buffer;
	pblock_n = 0;
	file_pointer = 0;
	the_inode_yes = 1;
	file_pointer_max = the_inode.i_size;
	if ((the_inode.i_mode & EXT2__S_IFMT) == EXT2__S_IFREG)
		file_pointer_max |= ((uint64)the_inode.i_dir_acl) << ((uint64)32);

	return 1;
}

int	reader::file::get_inode(uint64 in)
{
	the_inode_N=in;
	return reset();
}

uint64 reader::file::seek(uint64 ofs)
{
	if (ofs > file_pointer_max)
		ofs = file_pointer_max;

	uint64 o = ofs - (ofs % mom->e2blocksize);
	uint64 c = file_pointer - (file_pointer % mom->e2blocksize);
	int of = ofs % mom->e2blocksize;

	if (o < c) {
		reset();
		o=c=0;
	}

	ofs = c + of;
	while (c < o) {
		nextblock();
		ofs += mom->e2blocksize;
	}

	file_pointer = ofs;
	return file_pointer;
}

int reader::file::read(unsigned char *buf,int len)
{
	int rd=0;

	while (len > 0) {
		uint64 filemax = file_pointer_max - file_pointer;
		int blk = file_pointer / mom->e2blocksize;
		int blko = file_pointer % mom->e2blocksize;
		int blksz = mom->e2blocksize - blko;
		if (blksz > len) blksz = len;
		if (blksz > filemax) blksz = filemax;

		if (blksz == 0)
			return rd;

		if (blk == buffer_block) {
			memcpy(buf,buffer + blko,blksz);
			file_pointer += blksz;
			len -= blksz;
			buf += blksz;
			rd += blksz;
		}
		else {
			if (nextblock()) {
				memcpy(buf,buffer + blko,blksz);
			}
			else {
				memset(buf,0,blksz);
			}
			file_pointer += blksz;
			len -= blksz;
			buf += blksz;
			rd += blksz;
		}
	}

	return rd;
}

int reader::file::nextblock()
{
again_adv:
	int ard=1;
	char badv=1;
	memset(buffer,0,mom->e2blocksize);

	if (indirect_map)
		if (llei32(indirect_map_ptr) == 0)
			indirect_map_ptr = indirect_map_fence;

	if (dindirect_map)
		if (llei32(dindirect_map_ptr) == 0)
			dindirect_map_ptr = dindirect_map_fence;

	if (tindirect_map)
		if (llei32(tindirect_map_ptr) == 0)
			tindirect_map_ptr = tindirect_map_fence;

	if (indirect_map)
		if (indirect_map_ptr < indirect_map_fence)
			badv=0;

	if (dindirect_map)
		if (dindirect_map_ptr < dindirect_map_fence)
			badv=0;

	if (tindirect_map)
		if (tindirect_map_ptr < tindirect_map_fence)
			badv=0;

	if (badv) {
		if (block_n < EXT2_N_BLOCKS) {
			if ( indirect_map)	delete  indirect_map;	 indirect_map = NULL;	 indirect_map_fence = NULL;	 indirect_map_ptr = NULL;
			if (dindirect_map)	delete dindirect_map;	dindirect_map = NULL;	dindirect_map_fence = NULL;	dindirect_map_ptr = NULL;
			if (tindirect_map)	delete tindirect_map;	tindirect_map = NULL;	tindirect_map_fence = NULL;	tindirect_map_ptr = NULL;

			if ((cur_block = the_inode.i_block[pblock_n = block_n++]) != 0)		ard = mom->e2blockread(cur_block,1,buffer);
			else																ard = 0;
		}
		else {
			ard = 0;
		}
	}

	if (ard) {
		if (pblock_n == EXT2_TIND_BLOCK) {
			if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map) && (tindirect_map_ptr >= tindirect_map_fence || !tindirect_map)) {
				// what we just read is an indirect map, so copy it and load the first one
				if (!tindirect_map) tindirect_map = new unsigned char[mom->e2blocksize];
				if (!tindirect_map) return 0;
				tindirect_map_fence = tindirect_map + mom->e2blocksize;
				memcpy(tindirect_map,buffer,mom->e2blocksize);
				tindirect_map_ptr = tindirect_map;
				// load the first into the buffer, then let the next if statement do it's thing
				memcpy(tindirect_map,buffer,mom->e2blocksize);
			}
			if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map) && tindirect_map) {
				cur_block = llei32(tindirect_map_ptr);
				if (cur_block == 0) goto again_adv;
				tindirect_map_ptr += 4;
				memset(buffer,0,mom->e2blocksize);
				mom->e2blockread(cur_block,1,buffer);
			}
		}
		if (pblock_n >= EXT2_DIND_BLOCK) {
			if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && (dindirect_map_ptr >= dindirect_map_fence || !dindirect_map)) {
				// what we just read is an indirect map, so copy it and load the first one
				if (!dindirect_map) dindirect_map = new unsigned char[mom->e2blocksize];
				if (!dindirect_map) return 0;
				dindirect_map_fence = dindirect_map + mom->e2blocksize;
				dindirect_map_ptr = dindirect_map;
				// load the first into the buffer, then let the next if statement do it's thing
				memcpy(dindirect_map,buffer,mom->e2blocksize);
			}
			if ((indirect_map_ptr >= indirect_map_fence || !indirect_map) && dindirect_map) {
				cur_block = llei32(dindirect_map_ptr);
				if (cur_block == 0) goto again_adv;
				dindirect_map_ptr += 4;
				memset(buffer,0,mom->e2blocksize);
				mom->e2blockread(cur_block,1,buffer);
			}
		}
		if (pblock_n >= EXT2_IND_BLOCK) {
			if (indirect_map_ptr >= indirect_map_fence || !indirect_map) {
				// what we just read is an indirect map, so copy it and load the first one
				if (!indirect_map) indirect_map = new unsigned char[mom->e2blocksize];
				if (!indirect_map) return 0;
				indirect_map_fence = indirect_map + mom->e2blocksize;
				indirect_map_ptr = indirect_map;
				// load the first into the buffer, then let the next if statement do it's thing
				memcpy(indirect_map,buffer,mom->e2blocksize);
			}
			if (indirect_map) {
				cur_block = llei32(indirect_map_ptr);
				if (cur_block == 0) goto again_adv;
				indirect_map_ptr += 4;
				memset(buffer,0,mom->e2blocksize);
				mom->e2blockread(cur_block,1,buffer);
			}
		}
	}

	ptr = buffer;
	buffer_block++;
	return 1;
}

////////////////////////////////////////////////
};
// END CEXT2 namespace
