

#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>


#include "../include/XCraft.h"
#include "../include/hash.h"
#include "../include/bitmap.h"
#include "../include/bitmap.h"
// hash value to caculate 
// name:文件名 len:文件名长度  hinfo:hash值(储存hash值的结构体)
int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo){

    return 0;
}

static inline void dx_set_block(struct dx_entry *entry, uint32_t value)
{
	entry->block = cpu_to_le32(value);
}

static inline uint32_t dx_get_block(struct dx_entry *entry)
{
	return le32_to_cpu(entry->block);
}

static inline void dx_set_hash(struct dx_entry *entry, uint32_t value)
{
	entry->hash = cpu_to_le32(value);
}

static inline uint32_t dx_get_hash(struct dx_entry *entry)
{
	return le32_to_cpu(entry->hash);
}

static inline uint32_t dx_root_limit(void)
{
	unsigned int limit = (XCRAFT_BLOCK_SIZE - sizeof(struct dx_root_info)) / sizeof(struct dx_entry);
	return limit;
}


static void dx_release(struct dx_frame *frames)
{
	struct dx_root_info *info;
	int i;
	unsigned int indirect_levels;

	if (frames[0].bh == NULL)
		return;

	info = &((struct dx_root *)frames[0].bh->b_data)->info;
	/* save local copy, "info" may be freed after brelse() */
	indirect_levels = info->indirect_levels;
	for (i = 0; i <= indirect_levels; i++) {
		if (frames[i].bh == NULL)
			break;
		brelse(frames[i].bh);
		frames[i].bh = NULL;
	}
}

static int XCraft_set_inode_flag(struct inode*dir,uint32_t flag){
	dir->i_flags|=flag;
	return 0;
}

static struct buffer_head *XCraft_append(struct inode*dir,uint32_t*block){
	struct  buffer_head *bh;
	*block=get_free_blocks(XCRAFT_SB(dir->i_sb),1);
	bh=sb_bread(dir->i_sb,*block);
	if(!bh){
		printk(KERN_ERR "XCraft: append: no memory\n");
		return NULL;
	}
	return bh;
}

static int dx_make_map(struct inode*dir,struct buffer_head*bh,struct XCraft_hash_info *hinfo,
						struct dx_map_entry *map_tail)
{
	int count=0;
	struct XCraft_dir_entry *de=(struct XCraft_dir_entry *)bh->b_data;
	unsigned int buflen = bh->b_size;
	struct XCraft_hash_info *h=hinfo;
	char *base = bh->b_data;
	uint16_t rec_len;

	while((char*)de<base+buflen){
		if(de->inode&&de->name_len){
			dx_set_hash(&map_tail->hash,h->hash);
			dx_set_block(&map_tail->block,de->inode);
			XCraft_dirhash(de->name,de->name_len,h);
			map_tail--;
			map_tail->hash=h.hash;
			map_tail->offs=(u16)((char*)de-base);
			map_tail->size=le16_to_cpu(de->rec_len);
			count++;
		}
		//读取此目录项的长度
		rec_len=le16_to_cpu(de->rec_len);
		//de++是指向下一个目录项
		de=(struct XCraft_dir_entry*)((char*)de+rec_len);
	}
	return count;
}

// hash tree to build
// handle:事务 qstr:文件名 dir:目录 inode:文件 bh:缓冲区头
// 在dir下建一个dir_entry   inode是我们目录项索引的文件
// inode->i_ino是ino号
// inode对应的块存放目录项，如果d_entry存满一个块，开始make_hash_tree
//  bh是撑爆的第一个块 进行分裂
/*
 * This converts a one block unindexed directory to a 3 block indexed
 * directory, and adds the dentry to the indexed directory.
 */
static int XCraft_make_hash_tree(const struct qstr *qstr,
static int XCraft_make_hash_tree(const struct qstr *qstr,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh)
{

	//先创立root与一个撑满的块
	//再调用do_split,对撑满的块进行分裂
	//最后将新的目录项插入到目录中 完成de->inode的赋值

	struct dx_root	*root;
	struct dx_frame frames[XCRAFT_HTREE_LEVEL],*frame;
	struct dx_entry *entries;
	struct buffer_head *bh2;
	struct XCraft_dir_entry*de;
	struct XCraft_hash_info *hinfo;
	uint32_t bno;
	struct buffer_head *bh2;
	struct XCraft_dir_entry*de;
	struct XCraft_hash_info *hinfo;
	uint32_t bno;

	printk(KERN_DEBUG "Creating index: inode %lu\n", dir->i_ino);
	/*root*/
	root=(struct  dx_root *)bh->b_data;
	bh2=XCraft_append(dir,&bno);
	if(!bh2){
		printk(KERN_ERR "XCraft: make_hash_tree: no memory\n");
		return -ENOMEM;
	}
	XCraft_set_inode_flag(dir,XCraft_INODE_HASH_TREE);
	//将bh信息复制到bh2
	memcpy((char*)bh2->b_data,(char*)bh->b_data,XCRAFT_BLOCK_SIZE);


	/*更新root*/
	memset((char*)root,0x00,XCRAFT_BLOCK_SIZE);
	root->info.hash_version=(uint8_t)XCRAFT_HTREE_VERSION;
	root->info.count=cpu_to_le16(1);//目录项数目
	root->info.indirect_levels=0;
	root->info.limit=cpu_to_le16(dx_root_limit());
	entries=root->entries;
	dx_set_block(entries,bno);//bno是物理块号


	memset(frames, 0, sizeof(frames));
	frame=frames;
	frame->entries=entries;
	frame->at=entries;
	frame->bh=bh;
	//计算新增目录项的hash值
	XCraft_dirhash(qstr->name,qstr->len,hinfo);
	//将bh2分裂 bh2最后是两个块中应该插入新目录项的块
	de=do_split(dir,&bh2,frame,hinfo);

	//将新目录项插入到目录中




out_frames:
	mark_inode_dirty(dir);
	mark_buffer_dirty(bh);
	mark_buffer_dirty(bh2);
	dx_release(frames);
	brelse(bh2);
	/*root*/
	root=(struct  dx_root *)bh->b_data;
	bh2=XCraft_append(dir,&bno);
	if(!bh2){
		printk(KERN_ERR "XCraft: make_hash_tree: no memory\n");
		return -ENOMEM;
	}
	XCraft_set_inode_flag(dir,XCraft_INODE_HASH_TREE);
	//将bh信息复制到bh2
	memcpy((char*)bh2->b_data,(char*)bh->b_data,XCRAFT_BLOCK_SIZE);


	/*更新root*/
	memset((char*)root,0x00,XCRAFT_BLOCK_SIZE);
	root->info.hash_version=(uint8_t)XCRAFT_HTREE_VERSION;
	root->info.count=cpu_to_le16(1);//目录项数目
	root->info.indirect_levels=0;
	root->info.limit=cpu_to_le16(dx_root_limit());
	entries=root->entries;
	dx_set_block(entries,bno);//bno是物理块号


	memset(frames, 0, sizeof(frames));
	frame=frames;
	frame->entries=entries;
	frame->at=entries;
	frame->bh=bh;
	//计算新增目录项的hash值
	XCraft_dirhash(qstr->name,qstr->len,hinfo);
	//将bh2分裂 bh2最后是两个块中应该插入新目录项的块
	de=do_split(dir,&bh2,frame,hinfo);

	//将新目录项插入到目录中




out_frames:
	mark_inode_dirty(dir);
	mark_buffer_dirty(bh);
	mark_buffer_dirty(bh2);
	dx_release(frames);
	brelse(bh2);
    return 0;
}

// do_split
// dir是目录inode   bh是进行分裂的块的buffer_head   frame是上一级的dx_entry位置   hinfo是提前计算目录项的Hash值,用于分裂
// 1.分裂块
static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo)
{
    unsigned blocksize =XCRAFT_BLOCK_SIZE;
	uint32_t bno;
	struct buffer_head *bh2;
	char*data1=(*bh)->b_data,*data2;
	struct dx_map_entry *map;
	int count;

	//第三个块
	bh2=XCraft_append(dir,&bno);
	if(!bh2){
		printk(KERN_ERR "XCraft: do_split: no memory\n");
		return NULL;
	}
	data2=bh2->b_data;
	map = (struct dx_map_entry *) (data2 + blocksize);
	count=dx_make_map(dir,*bh,hinfo,map);
}