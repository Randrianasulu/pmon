/*
 * This file is part of UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 *
 * (C) Copyright 2008-2010
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Artem Bityutskiy (Битюцкий Артём)
 *          Adrian Hunter
 */

#include "ubifs.h"
#include <u-boot/zlib.h>
//#include "../../lib/lzo/lzodefs.h"

DECLARE_GLOBAL_DATA_PTR;

/* compress.c */

/*
 * We need a wrapper for zunzip() because the parameters are
 * incompatible with the lzo decompressor.
 */
static int gzip_decompress(const unsigned char *in, size_t in_len,
		unsigned char *out, size_t *out_len)
{
	return zunzip(out, *out_len, (unsigned char *)in,
			(unsigned long *)out_len, 0, 0);
}

#if 0
int lzo1x_decompress_safe(const unsigned char *in, size_t in_len,
		unsigned char *out, size_t *out_len)
{
	const unsigned char * const ip_end = in + in_len;
	unsigned char * const op_end = out + *out_len;
	const unsigned char *ip = in, *m_pos;
	unsigned char *op = out;
	size_t t;

	*out_len = 0;

	if (*ip > 17) {
		t = *ip++ - 17; 
		if (t < 4)
			goto match_next;
		if (HAVE_OP(t, op_end, op))
			goto output_overrun;
		int lzo1x_decompress_safe(const unsigned char *in, size_t in_len,
				unsigned char *out, size_t *out_len)
		{
			const unsigned char * const ip_end = in + in_len;
			unsigned char * const op_end = out + *out_len;
			const unsigned char *ip = in, *m_pos;
			unsigned char *op = out;
			size_t t;

			*out_len = 0;

			if (*ip > 17) {
				t = *ip++ - 17; 
				if (t < 4)
					goto match_next;
				if (HAVE_OP(t, op_end, op))
					goto output_overrun;
				if (t >= 4) {
					do {
						COPY4(op, ip);
						op += 4;
						ip += 4;
						t -= 4;
					} while (t >= 4);
					if (t > 0) {
						do {
							*op++ = *ip++;
						} while (--t > 0);
					}
				} else {
					do {
						*op++ = *ip++;
					} while (--t > 0);
				}
			}
first_literal_run:
			t = *ip++;
			if (t >= 16)
				goto match;
			m_pos = op - (1 + M2_MAX_OFFSET);
			m_pos -= t >> 2;
			m_pos -= *ip++ << 2;

			if (HAVE_LB(m_pos, out, op))
				goto lookbehind_overrun;

			if (HAVE_OP(3, op_end, op))
				goto output_overrun;
			*op++ = *m_pos++;
			*op++ = *m_pos++;
			*op++ = *m_pos;

			goto match_done;

			do {
match:
				if (t >= 64) {
					m_pos = op - 1;
					m_pos -= (t >> 2) & 7;
					m_pos -= *ip++ << 3;
					t = (t >> 5) - 1;
					if (HAVE_LB(m_pos, out, op))
						goto lookbehind_overrun;
					if (HAVE_OP(t + 3 - 1, op_end, op))
						goto output_overrun;
					goto copy_match;
				} else if (t >= 32) {
					t &= 31;
					if (t == 0) {
						if (HAVE_IP(1, ip_end, ip))
							goto input_overrun;
						while (*ip == 0) {
							t += 255;
							ip++;
							if (HAVE_IP(1, ip_end, ip))
								goto input_overrun;
						}
						t += 31 + *ip++;
					}
					m_pos = op - 1;
					m_pos -= get_unaligned_le16(ip) >> 2;
					ip += 2;
				} else if (t >= 16) {
					m_pos = op;
					m_pos -= (t & 8) << 11;

					t &= 7;
					if (t == 0) {
						if (HAVE_IP(1, ip_end, ip))
							goto input_overrun;
						while (*ip == 0) {
							t += 255;
							ip++;
							if (HAVE_IP(1, ip_end, ip))
								goto input_overrun;
						}
						t += 7 + *ip++;
					}
					m_pos -= get_unaligned_le16(ip) >> 2;
					ip += 2;
					if (m_pos == op)
						goto eof_found;
					m_pos -= 0x4000;
				} else {
					m_pos = op - 1;
					m_pos -= t >> 2;
					m_pos -= *ip++ << 2;

					if (HAVE_LB(m_pos, out, op))
						goto lookbehind_overrun;
					if (HAVE_OP(2, op_end, op))
						goto output_overrun;

					*op++ = *m_pos++;
					*op++ = *m_pos;
				}

				if (HAVE_LB(m_pos, out, op))
					goto lookbehind_overrun;
				if (HAVE_OP(t + 3 - 1, op_end, op))
					goto output_overrun;

				if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4) {
					COPY4(op, m_pos);
					op += 4;
					m_pos += 4;
					t -= 4 - (3 - 1);
					do {
						COPY4(op, m_pos);
						op += 4;
						m_pos += 4;
						t -= 4;
					} while (t >= 4);
					if (t > 0)
						do {
							*op++ = *m_pos++;
						} while (--t > 0);
				} else {
copy_match:
					*op++ = *m_pos++;
					*op++ = *m_pos++;
					do {
						*op++ = *m_pos++;
					} while (--t > 0);
				}
match_done:
				t = ip[-2] & 3;
				if (t == 0)
					break;
match_next:
				if (HAVE_OP(t, op_end, op))
					goto output_overrun;
				if (HAVE_IP(t + 1, ip_end, ip))
					goto input_overrun;

				*op++ = *ip++;
				if (t > 1) {
					*op++ = *ip++;
					if (t > 2)
						*op++ = *ip++;
				}

				t = *ip++;
			} while (ip < ip_end);
		}
		*out_len = op - out;
		return LZO_E_EOF_NOT_FOUND;

eof_found:
		*out_len = op - out;
		return (ip == ip_end ? LZO_E_OK :
				(ip < ip_end ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));
input_overrun:
		*out_len = op - out;
		return LZO_E_INPUT_OVERRUN;

output_overrun:
		*out_len = op - out;
		return LZO_E_OUTPUT_OVERRUN;
lookbehind_overrun:
		*out_len = op - out;
		return LZO_E_LOOKBEHIND_OVERRUN;
	}
#endif




/* Fake description object for the "none" compressor */
static struct ubifs_compressor none_compr = {
	.compr_type = UBIFS_COMPR_NONE,
	.name = "no compression",
	.capi_name = "",
	.decompress = NULL,
};
#if 0
static struct ubifs_compressor lzo_compr = {
	.compr_type = UBIFS_COMPR_LZO,
	.name = "LZO",
	.capi_name = "lzo",
	.decompress = lzo1x_decompress_safe,
};
#endif
static struct ubifs_compressor zlib_compr = {
	.compr_type = UBIFS_COMPR_ZLIB,
	.name = "zlib",
	.capi_name = "deflate",
	.decompress = gzip_decompress,
};

/* All UBIFS compressors */
struct ubifs_compressor *ubifs_compressors[UBIFS_COMPR_TYPES_CNT];

/**
 * ubifs_decompress - decompress data.
 * @in_buf: data to decompress
 * @in_len: length of the data to decompress
 * @out_buf: output buffer where decompressed data should
 * @out_len: output length is returned here
 * @compr_type: type of compression
 *
 * This function decompresses data from buffer @in_buf into buffer @out_buf.
 * The length of the uncompressed data is returned in @out_len. This functions
 * returns %0 on success or a negative error code on failure.
 */
int ubifs_decompress(const void *in_buf, int in_len, void *out_buf,
		     int *out_len, int compr_type)
{
	int err;
	struct ubifs_compressor *compr;

	if (unlikely(compr_type < 0 || compr_type >= UBIFS_COMPR_TYPES_CNT)) {
		ubifs_err("invalid compression type %d", compr_type);
		return -EINVAL;
	}

	compr = ubifs_compressors[compr_type];

	if (unlikely(!compr->capi_name)) {
		ubifs_err("%s compression is not compiled in", compr->name);
		return -EINVAL;
	}

	if (compr_type == UBIFS_COMPR_NONE) {
		memcpy(out_buf, in_buf, in_len);
		*out_len = in_len;
		return 0;
	}

	err = compr->decompress(in_buf, in_len, out_buf, (size_t *)out_len);
	if (err)
		ubifs_err("cannot decompress %d bytes, compressor %s, "
			  "error %d", in_len, compr->name, err);

	return err;
}

/**
 * compr_init - initialize a compressor.
 * @compr: compressor description object
 *
 * This function initializes the requested compressor and returns zero in case
 * of success or a negative error code in case of failure.
 */
static int __init compr_init(struct ubifs_compressor *compr)
{
	ubifs_compressors[compr->compr_type] = compr;

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	ubifs_compressors[compr->compr_type]->name += gd->reloc_off;
	ubifs_compressors[compr->compr_type]->capi_name += gd->reloc_off;
	ubifs_compressors[compr->compr_type]->decompress += gd->reloc_off;
#endif

	return 0;
}

/**
 * ubifs_compressors_init - initialize UBIFS compressors.
 *
 * This function initializes the compressor which were compiled in. Returns
 * zero in case of success and a negative error code in case of failure.
 */
int __init ubifs_compressors_init(void)
{
	int err;
#if 0
	err = compr_init(&lzo_compr);
	if (err)
		return err;
#endif
	err = compr_init(&zlib_compr);
	if (err)
		return err;

	err = compr_init(&none_compr);
	if (err)
		return err;

	return 0;
}

/*
 * ubifsls...
 */

static int filldir(struct ubifs_info *c, const char *name, int namlen,
		   u64 ino, unsigned int d_type)
{
	struct inode *inode;
	char filetime[32];

	switch (d_type) {
	case UBIFS_ITYPE_REG:
		printf("\t");
		break;
	case UBIFS_ITYPE_DIR:
		printf("<DIR>\t");
		break;
	case UBIFS_ITYPE_LNK:
		printf("<LNK>\t");
		break;
	default:
		printf("other\t");
		break;
	}

	inode = ubifs_iget(c->vfs_sb, ino);
	if (IS_ERR(inode)) {
		printf("%s: Error in ubifs_iget(), ino=%lld ret=%p!\n",
		       __func__, ino, inode);
		return -1;
	}
	ctime_r((time_t *)&inode->i_mtime, filetime);
	printf("%9lld  %24.24s  ", inode->i_size, filetime);
	ubifs_iput(inode);

	printf("%s\n", name);

	return 0;
}

static int ubifs_printdir(struct file *file, void *dirent)
{
	int err, over = 0;
	struct qstr nm;
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct inode *dir = file->f_path.dentry->d_inode;
	struct ubifs_info *c = dir->i_sb->s_fs_info;

	dbg_gen("dir ino %lu, f_pos %#llx", dir->i_ino, file->f_pos);

	if (file->f_pos > UBIFS_S_KEY_HASH_MASK || file->f_pos == 2)
		/*
		 * The directory was seek'ed to a senseless position or there
		 * are no more entries.
		 */
		return 0;

	if (file->f_pos == 1) {
		/* Find the first entry in TNC and save it */
		lowest_dent_key(c, &key, dir->i_ino);
		nm.name = NULL;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		file->f_pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
	}

	dent = file->private_data;
	if (!dent) {
		/*
		 * The directory was seek'ed to and is now readdir'ed.
		 * Find the entry corresponding to @file->f_pos or the
		 * closest one.
		 */
		dent_key_init_hash(c, &key, dir->i_ino, file->f_pos);
		nm.name = NULL;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}
		file->f_pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
	}

	while (1) {
		dbg_gen("feed '%s', ino %llu, new f_pos %#x",
			dent->name, (unsigned long long)le64_to_cpu(dent->inum),
			key_hash_flash(c, &dent->key));
		ubifs_assert(le64_to_cpu(dent->ch.sqnum) > ubifs_inode(dir)->creat_sqnum);

		nm.len = le16_to_cpu(dent->nlen);
		over = filldir(c, (char *)dent->name, nm.len,
			       le64_to_cpu(dent->inum), dent->type);
		if (over)
			return 0;

		/* Switch to the next entry */
		key_read(c, &dent->key, &key);
		nm.name = (char *)dent->name;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		kfree(file->private_data);
		file->f_pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
		cond_resched();
	}

out:
	if (err != -ENOENT) {
		ubifs_err("cannot find next direntry, error %d", err);
		return err;
	}

//	kfree(file->private_data);
	file->private_data = NULL;
	file->f_pos = 2;
	return 0;
}

static int ubifs_finddir(struct super_block *sb, char *dirname,
			 unsigned long root_inum, unsigned long *inum)
{
	int err;
	struct qstr nm;
	union ubifs_key key;
	struct ubifs_dent_node *dent;
	struct ubifs_info *c;
	struct file *file;
	struct dentry *dentry;
	struct inode *dir;
	int ret = 0;

	file = kzalloc(sizeof(struct file), 0);
	dentry = kzalloc(sizeof(struct dentry), 0);
	dir = kzalloc(sizeof(struct inode), 0);
	if (!file || !dentry || !dir) {
		printf("%s: Error, no memory for malloc!\n", __func__);
		err = -ENOMEM;
		goto out;
	}

	dir->i_sb = sb;
	file->f_path.dentry = dentry;
	file->f_path.dentry->d_parent = dentry;
	file->f_path.dentry->d_inode = dir;
	file->f_path.dentry->d_inode->i_ino = root_inum;
	c = sb->s_fs_info;

	dbg_gen("dir ino %lu, f_pos %#llx", dir->i_ino, file->f_pos);

	/* Find the first entry in TNC and save it */
	lowest_dent_key(c, &key, dir->i_ino);
	nm.name = NULL;
	dent = ubifs_tnc_next_ent(c, &key, &nm);
	if (IS_ERR(dent)) {
		err = PTR_ERR(dent);
		goto out;
	}

	file->f_pos = key_hash_flash(c, &dent->key);
	file->private_data = dent;

	while (1) {
		dbg_gen("feed '%s', ino %llu, new f_pos %#x",
			dent->name, (unsigned long long)le64_to_cpu(dent->inum),
			key_hash_flash(c, &dent->key));
		ubifs_assert(le64_to_cpu(dent->ch.sqnum) > ubifs_inode(dir)->creat_sqnum);

		nm.len = le16_to_cpu(dent->nlen);
		if ((strncmp(dirname, (char *)dent->name, nm.len) == 0) &&
		    (strlen(dirname) == nm.len)) {
			*inum = le64_to_cpu(dent->inum);
			ret = 1;
			goto out_free;
		}

		/* Switch to the next entry */
		key_read(c, &dent->key, &key);
		nm.name = (char *)dent->name;
		dent = ubifs_tnc_next_ent(c, &key, &nm);
		if (IS_ERR(dent)) {
			err = PTR_ERR(dent);
			goto out;
		}

		kfree(file->private_data);
		file->f_pos = key_hash_flash(c, &dent->key);
		file->private_data = dent;
		cond_resched();
	}

out:
	if (err != -ENOENT)
		ubifs_err("cannot find next direntry, error %d", err);

out_free:
	if (file->private_data)
		kfree(file->private_data);
	if (file)
		free(file);
	if (dentry)
		free(dentry);
	if (dir)
		free(dir);

	return ret;
}

static unsigned long ubifs_findfile(struct super_block *sb, char *filename)
{
	int ret;
	char *next;
	char fpath[128];
	char symlinkpath[128];
	char *name = fpath;
	unsigned long root_inum = 1;
	unsigned long inum;
	int symlink_count = 0; /* Don't allow symlink recursion */
	char link_name[64];

	strcpy(fpath, filename);

	/* Remove all leading slashes */
	while (*name == '/')
		name++;

	/*
	 * Handle root-direcoty ('/')
	 */
	inum = root_inum;
	if (!name || *name == '\0')
		return inum;

	for (;;) {
		struct inode *inode;
		struct ubifs_inode *ui;

		/* Extract the actual part from the pathname.  */
		next = strchr(name, '/');
		if (next) {
			/* Remove all leading slashes.  */
			while (*next == '/')
				*(next++) = '\0';
		}

		ret = ubifs_finddir(sb, name, root_inum, &inum);
		if (!ret)
			return 0;
		inode = ubifs_iget(sb, inum);

		if (!inode)
			return 0;
		ui = ubifs_inode(inode);

		if ((inode->i_mode & S_IFMT) == S_IFLNK) {
			char buf[128];

			/* We have some sort of symlink recursion, bail out */
			if (symlink_count++ > 8) {
				printf("Symlink recursion, aborting\n");
				return 0;
			}
			memcpy(link_name, ui->data, ui->data_len);
			link_name[ui->data_len] = '\0';

			if (link_name[0] == '/') {
				/* Absolute path, redo everything without
				 * the leading slash */
				next = name = link_name + 1;
				root_inum = 1;
				continue;
			}
			/* Relative to cur dir */
			sprintf(buf, "%s/%s",
					link_name, next == NULL ? "" : next);
			memcpy(symlinkpath, buf, sizeof(buf));
			next = name = symlinkpath;
			continue;
		}

		/*
		 * Check if directory with this name exists
		 */

		/* Found the node!  */
		if (!next || *next == '\0')
			return inum;

		root_inum = inum;
		name = next;
	}

	return 0;
}

int ubifs_ls(char *filename)
{
	struct ubifs_info *c = ubifs_sb->s_fs_info;
	struct file *file;
	struct dentry *dentry;
	struct inode *dir;
	void *dirent = NULL;
	unsigned long inum;
	int ret = 0;

	c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READONLY);
	inum = ubifs_findfile(ubifs_sb, filename);
	if (!inum) {
		ret = -1;
		goto out;
	}

	file = kzalloc(sizeof(struct file), 0);
	dentry = kzalloc(sizeof(struct dentry), 0);
	dir = kzalloc(sizeof(struct inode), 0);
	if (!file || !dentry || !dir) {
		printf("%s: Error, no memory for malloc!\n", __func__);
		ret = -ENOMEM;
		goto out_mem;
	}

	dir->i_sb = ubifs_sb;
	file->f_path.dentry = dentry;
	file->f_path.dentry->d_parent = dentry;
	file->f_path.dentry->d_inode = dir;
	file->f_path.dentry->d_inode->i_ino = inum;
	file->f_pos = 1;
	file->private_data = NULL;
	ubifs_printdir(file, dirent);

out_mem:
	if (file)
		free(file);
	if (dentry)
		free(dentry);
	if (dir)
		free(dir);

out:
	ubi_close_volume(c->ubi);
	return ret;
}

/*
 * ubifsload...
 */

/* file.c */

static inline void *kmap(struct page *page)
{
	return page->addr;
}

static int read_block(struct inode *inode, void *addr, unsigned int block,
		      struct ubifs_data_node *dn)
{
	struct ubifs_info *c = inode->i_sb->s_fs_info;
	int err, len, out_len;
	union ubifs_key key;
	unsigned int dlen;

	data_key_init(c, &key, inode->i_ino, block);
	err = ubifs_tnc_lookup(c, &key, dn);
	if (err) {
		if (err == -ENOENT)
			/* Not found, so it must be a hole */
			memset(addr, 0, UBIFS_BLOCK_SIZE);
		return err;
	}

	ubifs_assert(le64_to_cpu(dn->ch.sqnum) > ubifs_inode(inode)->creat_sqnum);

	len = le32_to_cpu(dn->size);
	if (len <= 0 || len > UBIFS_BLOCK_SIZE)
		goto dump;

	dlen = le32_to_cpu(dn->ch.len) - UBIFS_DATA_NODE_SZ;
	out_len = UBIFS_BLOCK_SIZE;
	err = ubifs_decompress(&dn->data, dlen, addr, &out_len,
			       le16_to_cpu(dn->compr_type));
	if (err || len != out_len)
		goto dump;

	/*
	 * Data length can be less than a full block, even for blocks that are
	 * not the last in the file (e.g., as a result of making a hole and
	 * appending data). Ensure that the remainder is zeroed out.
	 */
	if (len < UBIFS_BLOCK_SIZE)
		memset(addr + len, 0, UBIFS_BLOCK_SIZE - len);

	return 0;

dump:
	ubifs_err("bad data node (block %u, inode %lu)",
		  block, inode->i_ino);
	dbg_dump_node(c, dn);
	return -EINVAL;
}

static int do_readpage(struct ubifs_info *c, struct inode *inode,
		       struct page *page, int last_block_size)
{
	void *addr;
	int err = 0, i;
	unsigned int block, beyond;
	struct ubifs_data_node *dn;
	loff_t i_size = inode->i_size;

	dbg_gen("ino %lu, pg %lu, i_size %lld",
		inode->i_ino, page->index, i_size);

	addr = kmap(page);

	block = page->index << UBIFS_BLOCKS_PER_PAGE_SHIFT;
	beyond = (i_size + UBIFS_BLOCK_SIZE - 1) >> UBIFS_BLOCK_SHIFT;
	if (block >= beyond) {
		/* Reading beyond inode */
		memset(addr, 0, PAGE_CACHE_SIZE);
		goto out;
	}

	dn = kmalloc(UBIFS_MAX_DATA_NODE_SZ, GFP_NOFS);
	if (!dn)
		return -ENOMEM;

	i = 0;
	while (1) {
		int ret;

		if (block >= beyond) {
			/* Reading beyond inode */
			err = -ENOENT;
			memset(addr, 0, UBIFS_BLOCK_SIZE);
		} else {
			/*
			 * Reading last block? Make sure to not write beyond
			 * the requested size in the destination buffer.
			 */
			if (((block + 1) == beyond) || last_block_size) {
				void *buff;
				int dlen;

				/*
				 * We need to buffer the data locally for the
				 * last block. This is to not pad the
				 * destination area to a multiple of
				 * UBIFS_BLOCK_SIZE.
				 */
				buff = malloc(UBIFS_BLOCK_SIZE);
				if (!buff) {
					printf("%s: Error, malloc fails!\n",
					       __func__);
					err = -ENOMEM;
					break;
				}

				/* Read block-size into temp buffer */
				ret = read_block(inode, buff, block, dn);
				if (ret) {
					err = ret;
					if (err != -ENOENT) {
						free(buff);
						break;
					}
				}

				if (last_block_size)
					dlen = last_block_size;
				else
					dlen = le32_to_cpu(dn->size);

				/* Now copy required size back to dest */
				memcpy(addr, buff, dlen);

				free(buff);
			} else {
				ret = read_block(inode, addr, block, dn);
				if (ret) {
					err = ret;
					if (err != -ENOENT)
						break;
				}
			}
		}
		if (++i >= UBIFS_BLOCKS_PER_PAGE)
			break;
		block += 1;
		addr += UBIFS_BLOCK_SIZE;
	}
	if (err) {
		if (err == -ENOENT) {
			/* Not found, so it must be a hole */
			dbg_gen("hole");
			goto out_free;
		}
		ubifs_err("cannot read page %lu of inode %lu, error %d",
			  page->index, inode->i_ino, err);
		goto error;
	}

out_free:
	kfree(dn);
out:
	return 0;

error:
	kfree(dn);
	return err;
}

//scl added
int setenv_hex(const char *varname, ulong value)
{
	char str[17];

	sprintf(str, "%lx", value);
	return setenv(varname, str);
}

int ubifs_load(char *filename, u32 addr, u32 size)
{
	struct ubifs_info *c = ubifs_sb->s_fs_info;
	unsigned long inum;
	struct inode *inode;
	struct page page;
	int err = 0;
	int i;
	int count;
	int last_block_size = 0;

	c->ubi = ubi_open_volume(c->vi.ubi_num, c->vi.vol_id, UBI_READONLY);
	/* ubifs_findfile will resolve symlinks, so we know that we get
	 * the real file here */
	inum = ubifs_findfile(ubifs_sb, filename);
	if (!inum) {
		err = -1;
		goto out;
	}

	/*
	 * Read file inode
	 */
	inode = ubifs_iget(ubifs_sb, inum);
	if (IS_ERR(inode)) {
		printf("%s: Error reading inode %ld!\n", __func__, inum);
		err = PTR_ERR(inode);
		goto out;
	}

	/*
	 * If no size was specified or if size bigger than filesize
	 * set size to filesize
	 */
	if ((size == 0) || (size > inode->i_size))
		size = inode->i_size;

	count = (size + UBIFS_BLOCK_SIZE - 1) >> UBIFS_BLOCK_SHIFT;
	printf("Loading file '%s' to addr 0x%08x with size %d (0x%08x)...\n",
	       filename, addr, size, size);

	page.addr = (void *)addr;
	page.index = 0;
	page.inode = inode;
	for (i = 0; i < count; i++) {
		/*
		 * Make sure to not read beyond the requested size
		 */
		if (((i + 1) == count) && (size < inode->i_size))
			last_block_size = size - (i * PAGE_SIZE);

		err = do_readpage(c, inode, &page, last_block_size);
		if (err)
			break;

		page.addr += PAGE_SIZE;
		page.index++;
	}

	if (err)
		printf("Error reading file '%s'\n", filename);
	else {
		setenv_hex("filesize", size);
		printf("Done\n");
	}

	ubifs_iput(inode);

out:
	ubi_close_volume(c->ubi);
	return err;
}
