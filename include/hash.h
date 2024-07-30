#ifndef _XCRAFT_HASH_H
#define _XCRAFT_HASH_H
// #include <linux/sched.h>
#include "XCraft.h"
#include "bitmap.h"
static __u32 dx_hack_hash_unsigned(const char *name, int len)
{
	__u32 hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const unsigned char *ucp = (const unsigned char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *ucp++) * 7152373));

		if (hash & 0x80000000)
			hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}


// hash value to caculate 
// name:文件名 len:文件名长度  hinfo:hash值(储存hash值的结构体)
static int XCraft_dirhash(const char *name, int len, struct XCraft_hash_info *hinfo){
	__u32 hash;

	if(hinfo->hash_version != XCRAFT_HTREE_VERSION){
		hinfo->hash = 0;
		return -1;
	}	

	hash = dx_hack_hash_unsigned(name, len);
	hash = hash & ~1;
	if(hash == (0x7fffffff << 1))
		hash = (0x7fffffff - 1) << 1;
	hinfo->hash = hash;
    return 0;
}

static inline uint32_t dx_node_limit(void)
{
	// unsigned int limit = (XCRAFT_BLOCK_SIZE - sizeof(__le16))  / sizeof(struct dx_entry);
	// return limit;
	return 3;
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
	// unsigned int limit = (XCRAFT_BLOCK_SIZE - sizeof(struct dx_root_info)) / sizeof(struct dx_entry);
	// return limit;
	return 3;
}

static inline void dx_set_count(struct dx_entry *entries, unsigned value)
{
	((struct dx_countlimit *) entries)->count = cpu_to_le16(value);
}
static inline void dx_set_limit(struct dx_entry *entries, unsigned value)
{
	((struct dx_countlimit *) entries)->limit = cpu_to_le16(value);
}
static inline unsigned dx_get_count(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->count);
}

static inline unsigned dx_get_limit(struct dx_entry *entries)
{
	return le16_to_cpu(((struct dx_countlimit *) entries)->limit);
}



// static void dx_release(struct dx_frame *frames);

// static int XCraft_set_inode_flag(struct inode*dir,uint32_t flag);

// static struct buffer_head *XCraft_append(struct inode*dir,uint32_t*block);

// static int dx_make_map(struct inode*dir,struct buffer_head*bh,struct XCraft_hash_info *hinfo,
// 						struct dx_map_entry *map_tail);


// static void dx_sort_map(struct dx_map_entry *map, unsigned count);


// static struct XCraft_dir_entry*
// dx_move_dirents(struct inode *dir, char *from, char *to,
// 		struct dx_map_entry *map, int count,
// 		unsigned blocksize);

// static struct XCraft_dir_entry *dx_pack_dirents(struct inode *dir, char *base, unsigned blocksize);

// static void dx_insert_block(struct dx_frame *frame,uint32_t hash,uint32_t bno);




// // 在bh对应的磁盘块中插入目录项
// // de记录最后的目录项位置, bh为插入该目录项所在的磁盘块缓冲区
// // de 一般传入null，不是null就表明已经是找到了目录项
// static int add_dirent_to_buf(struct dentry *dentry,
// 			     struct inode *inode, struct XCraft_dir_entry *de,
// 			     struct buffer_head * bh);


// static struct XCraft_dir_entry *do_split(struct inode *dir,
// 			struct buffer_head **bh,struct dx_frame *frame,
// 			struct XCraft_hash_info *hinfo);

// static int XCraft_make_hash_tree(struct dentry *dentry,
// 			    struct inode *dir,
// 			    struct inode *inode, struct buffer_head *bh);



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

// 用于delete_hash_block
static void dx_del_release(struct del_dx_frame *frames)
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
	// 获取inode_info
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	dir_info->i_flags|=flag;
	return 0;
}

static struct buffer_head *XCraft_append(struct inode*dir,uint32_t*block){
	struct buffer_head *bh;
	struct super_block *sb = dir->i_sb;
	struct XCraft_superblock_info *sb_info = XCRAFT_SB(sb);
	*block=get_free_blocks(sb_info,1);
	bh=sb_bread(sb,*block);
	
	if(!bh){
		pr_debug(KERN_ERR "XCraft: append: no memory\n");
		return NULL;
	}
	// 更新dir中的字段,追加一个块多使用了一个块
	dir->i_blocks+=1;
	mark_inode_dirty(dir);
	return bh;
}

static int dx_make_map(struct inode*dir,struct buffer_head*bh,struct XCraft_hash_info *hinfo,
						struct dx_map_entry *map_tail)
{
	int count=0;
	struct XCraft_dir_entry *de=(struct XCraft_dir_entry *)bh->b_data;
	unsigned int buflen = bh->b_size;
	// 只是单纯要里面的内容
	struct XCraft_hash_info h = *hinfo;
	char *base = bh->b_data;
	uint16_t rec_len;

	while((char*)de < base + buflen){
		//inode必须有效 不然rec_len为0 死循环
		if(de->inode&&de->name_len){
			//计算Hash
			XCraft_dirhash(de->name,de->name_len,&h);
			map_tail--;
			map_tail->hash=h.hash;
			map_tail->offs=((char*)de-base)>>2;
			map_tail->size=le16_to_cpu(de->rec_len);
			count++;
			//存疑
			// cond_resched();
			
			//读取此目录项的长度
			rec_len=le16_to_cpu(de->rec_len);
			//de++是指向下一个目录项
			de=(struct XCraft_dir_entry*)((char*)de+rec_len);
		}
		else{
			// 此时明显不是目录项，因为我们的hash树分裂时并没有撑满
			break;
		}
	}
	pr_debug("dx_make_map process correctly\n");
	return count;
}


static void dx_sort_map (struct dx_map_entry *map, unsigned count){
	struct dx_map_entry *p,*q,*top=map+count-1;
	// unsigned i,j;

	// for(i=1;i<count;i++){
	// 	tmp=map[i];
	// 	for(j=i;j>0;j--){
	// 		p=&map[j-1];
	// 		q=&map[j];
	// 		if(p->hash<=q->hash)
	// 			break;
	// 		map[j]=*p;
	// 	}
	// 	map[j]=tmp;
	// }
	int more = 1;
	/* Combsort until bubble sort doesn't suck */
	while (count > 2) {
		count = count*10/13;
		if (count - 9 < 2) /* 9, 10 -> 11 */
			count = 11;
		for (p = top, q = p - count; q >= map; p--, q--)
			if (p->hash < q->hash)
				swap(*p, *q);
	}
	/* Garden variety bubble sort */
	do {
		more = 0;
		q = top;
		while (q-- > map) {
			if (q[1].hash >= q[0].hash)
				continue;
			swap(*(q+1), *q);
			more = 1;
		}
	} while(more);

	pr_debug("dx_sort_map process correctly\n");
}

/*
* 利用to下面的map_entry，找到from中对应的目录项，然后移动到to中，最后返回的是to中最大的目录项
*/
static struct XCraft_dir_entry*
dx_move_dirents(struct inode *dir, char *from, char *to,
		struct dx_map_entry *map, int count,
		unsigned blocksize)
{
	unsigned rec_len=0;
	char *to_begin = to;
	while(count){
		//找到与to里面对应的from的目录项
		struct XCraft_dir_entry *de=(struct XCraft_dir_entry*)(from+(map->offs<<2));
		rec_len=le16_to_cpu(de->rec_len);
		pr_debug("move file_name: %s\n", de->name);
		memcpy(to,de,rec_len);
		//除了rec_len的部分，其他部分清零
		de->inode=0;
		// memset(&de->name_len,0,rec_len-offsetof(struct XCraft_dir_entry,name_len));
		// memset(&de->name_len,0,rec_len-((size_t)&((struct XCraft_dir_entry *)0)->name_len));
		de->name_len = 0;
		de->file_type = 0;
		memset(de->name, 0, XCRAFT_NAME_LEN);
		//更新map
		map++;
		to+=rec_len;
		count--;
	}
	// 新块后面有map_entry类型的东西，我们将其全部置0
	memset(to, 0, to_begin + blocksize - to);
	return (struct XCraft_dir_entry*) to;
}


/*清理base中的无用目录项，使其变得紧凑
* 返回最后一个entry
*/

static struct XCraft_dir_entry *dx_pack_dirents(struct inode *dir, char *base, unsigned blocksize)
{
	struct XCraft_dir_entry *next, *to, *prev, *de = (struct XCraft_dir_entry *) base;
	unsigned rec_len=0;
	prev=to=de;

	while((char*) de< base + blocksize){
		next = (struct XCraft_dir_entry*)((char*)de + le16_to_cpu(de->rec_len));
		if(de->rec_len == 0){
			pr_debug("rec_len is 0\n");
			break;
		}
		if(de->inode && de->name_len){
			rec_len=le16_to_cpu(de->rec_len);
			if(de>to){
				memmove(to,de,rec_len);
				// 此处的de应该要重置
				memset(de, 0, rec_len);
			}
			to->rec_len = cpu_to_le16(rec_len);
			prev = to;
			to = (struct XCraft_dir_entry*)((char*) to + rec_len);
		}
		de = next;
	}
	// 最后to处应该全部置0
	memset(to, 0, sizeof(struct XCraft_dir_entry));
	return to;
}

static void dx_insert_block(struct dx_frame *frame,uint32_t hash,uint32_t bno)
{
	struct dx_entry*entries=frame->entries;
	struct dx_entry*old=frame->at;
	struct dx_entry*new=old+1;//新位置  上一级的dx_entry  建立index
	int count=dx_get_count(entries);
	ASSERT(count < dx_get_limit(entries));//现在新加1个，所以count<limit
	ASSERT(old<entries+count);
	//找到插入的位置 把old后面的entry往后移动
	memmove(new+1,new,(char*)(entries+count)-(char*)new);
	dx_set_hash(new,hash);
	dx_set_block(new,bno);
	dx_set_count(entries, count + 1);
	// pr_debug("dx_entries_count:%d\n",dx_get_count(entries));
	// pr_debug("dx_entries[1]_hash:%x",dx_get_hash(entries+1));

}

// 在bh对应的磁盘块中插入目录项
// de记录最后的目录项位置, bh为插入该目录项所在的磁盘块缓冲区
// de 一般传入null，不是null就表明已经是找到了目录项
static int add_dirent_to_buf(struct dentry *dentry,
			     struct inode *inode, struct XCraft_dir_entry *de,
			     struct buffer_head * bh)
{
	struct inode *dir = dentry->d_parent->d_inode;
	struct XCraft_inode_info *dir_info = XCRAFT_I(dir);
	struct super_block *sb = dir->i_sb;
	const char	*name = dentry->d_name.name;
	int	namelen = dentry->d_name.len;
	// 一个目录项大小
	unsigned short reclen;
	char *top;
	int count = 0;

	// 判断是否是哈希树
	unsigned long flag;
	flag = XCraft_INODE_ISHASH_TREE(dir_info->i_flags);
	
	reclen = sizeof(struct XCraft_dir_entry);
	if(!de){
		de = (struct XCraft_dir_entry *)bh->b_data;
		// 锁定最大能插入的位置
		top = bh->b_data + sb->s_blocksize - reclen;

		// 循环遍历
		while((char *)de <= top ){
			// 跳出条件为找到一个目录项的inode号为0，此目录项可用
			if(!de->inode)
				break;
			// 检查是否已经存在该目录项
			// 比较是否有同名现象
			if(!strncmp(de->name,name,XCRAFT_NAME_LEN)){
				pr_debug("same name in add_dirent\n");
				return -EEXIST;
			}

			// 此时才是目录项
			reclen = le16_to_cpu(de->rec_len);
			
			// 不是哈希树时我们才受XCRAFT_dentry_LIMIT限制
			if(!flag){
				count++;
				if(count>=XCRAFT_dentry_LIMIT){
					pr_debug("add_dirent_to_buf: too many entries in a block\n");
					break;
				}
			}
			// 移动到下一个目录项
			de = (struct XCraft_dir_entry *)((char *)de + reclen);
		}
		pr_debug("count:%d\n",count);
		// 超了
		if(count>=XCRAFT_dentry_LIMIT || (char *)de > top)
			return -ENOSPC;
	}

	// 对找到的de进行更新
	// 传进来的inode必须是有效的
	if (inode){
		de->inode = cpu_to_le32(inode->i_ino);
		reclen = sizeof(struct XCraft_dir_entry);
		de->rec_len = cpu_to_le16(reclen);
		// 依据i_mode字段对file_type进行赋值
		if(S_ISDIR(inode->i_mode))
			de->file_type = XCRAFT_FT_DIR;
		else if(S_ISREG(inode->i_mode))
			de->file_type = XCRAFT_FT_REG_FILE;
		else if(S_ISLNK(inode->i_mode))	
			de->file_type = XCRAFT_FT_LINK;
		if(namelen>=XCRAFT_NAME_LEN){
			pr_debug("add_dirent_to_buf: name too long\n");
			de->name[XCRAFT_NAME_LEN] = '\0';
			namelen = XCRAFT_NAME_LEN;
			dentry->d_name.len = namelen;
		}
		memcpy(de->name, name, namelen);
		// 最后一个字符置为空字符
		de->name[namelen] = '\0';
		de->name_len = namelen;
		pr_debug("in add_dirent_to_buf, success");
	}

	// dir访问时间更新
	dir->i_atime = dir->i_mtime = dir->i_ctime = current_time(dir);
	mark_inode_dirty(dir);
	// 更新bh
	mark_buffer_dirty(bh);
	return 0;
}



// do_split
// dir是目录inode   bh是进行分裂的块的buffer_head   frame是上一级的dx_entry位置   hinfo是提前计算目录项的Hash值,用于分裂
// 1.分裂块
static struct XCraft_dir_entry *do_split(struct inode *dir,
			struct buffer_head **bh,struct dx_frame *frame,
			struct XCraft_hash_info *hinfo)
{
    unsigned blocksize =XCRAFT_BLOCK_SIZE;
	unsigned continued;
	uint32_t bno;
	uint32_t hash2;
	struct buffer_head *bh2;
	char *data1=(*bh)->b_data,*data2;
	struct dx_map_entry *map;
	unsigned split, move, size;
	struct XCraft_dir_entry *de = NULL, *de2;
	int count;
	int i;

	//第三个块
	bh2=XCraft_append(dir,&bno);
	if(!bh2){
		pr_debug(KERN_ERR "XCraft: do_split: no memory\n");
		return NULL;
	}
	data2=bh2->b_data;
	map = (struct dx_map_entry *) (data2 + blocksize);
	count=dx_make_map(dir,*bh,hinfo,map);
	pr_debug("in do_split , count :%d\n", count);
	if(count<0){
		pr_debug(KERN_ERR "XCraft: do_split: no memory\n");
		return NULL;
	}
	map-=count;
	dx_sort_map(map, count);
	pr_debug("dx_sort_map success!\n");

	/* Ensure that neither split block is over half full */
	size = 0;
	move = 0;
	for (i = count-1; i >= 0; i--) {
		/* is more than half of this entry in 2nd half of the block? */
		if (size + map[i].size/2 > blocksize/2)
			break;
		size += map[i].size;
		move++;
	}
	/*
	 * map index at which we will split
	 *
	 * If the sum of active entries didn't exceed half the block size, just
	 * split it in half by count; each resulting block will have at least
	 * half the space free.
	 */
	if (i > 0)
		split = count - move;
	else
		split = count/2;
	//split从1开始是个数 map指向第0个
	for(i=0;i<count;i++){
		pr_debug("map[%d].hash=%x\n",i,map[i].hash);
	}
	hash2=map[split].hash;
	continued = hash2 == map[split - 1].hash;
	pr_debug("Split block%lu at %x, %i/%i\n", 
			(unsigned long)dx_get_block(frame->at),hash2,split, count-split);
	/*如果split块和split-1的Hash相同，那么split与split-1要在一个块中，所以加上continued*/
	de2=dx_move_dirents(dir,data1,data2,map+split+continued,count-split-continued,blocksize);
	
	pr_debug("dx_move_dirents success!\n");
	de=dx_pack_dirents(dir,data1,blocksize);

	pr_debug("dx_pack_dirents success!\n");
	// 统一设置两个块最后一个目录项的rec_len字段
	de->rec_len = cpu_to_le16(sizeof(struct XCraft_dir_entry));
	de2->rec_len = cpu_to_le16(sizeof(struct XCraft_dir_entry));
	
	// 判断条件需要注意
	if((continued && hinfo->hash>hash2) || (!continued && hinfo->hash>=hash2)){
		swap(*bh, bh2);
		de = de2;
	}

	dx_insert_block(frame,hash2+continued,bno);
	pr_debug("dx_insert_block success\n");
	brelse(bh2);
	return de;
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
static int XCraft_make_hash_tree(struct dentry *dentry,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh)
{

	//先创立root与一个撑满的块
	//再调用do_split,对撑满的块进行分裂
	//最后将新的目录项插入到目录中 完成de->inode的赋值
	// 获取超级块
	struct super_block * sb = dir->i_sb;
	struct dx_root	*root;
	struct dx_frame frames[XCRAFT_HTREE_LEVEL],*frame;
	struct dx_entry *entries;
	struct buffer_head *bh2;
	struct XCraft_dir_entry*de = NULL;
	struct XCraft_hash_info hinfo;

	struct qstr *qstr = &dentry->d_name;
	uint32_t bno;
	int retval;


	pr_debug(KERN_DEBUG "Creating index: inode %lu\n", dir->i_ino);
	/*root*/
	root=(struct dx_root *)bh->b_data;
	bh2=XCraft_append(dir,&bno);
	pr_debug("分配的块号%d\n", bno);
	pr_debug("hello\n");
	if(!bh2){
		pr_debug(KERN_ERR "XCraft: make_hash_tree: no memory\n");
		return -ENOMEM;
	}
	XCraft_set_inode_flag(dir,XCraft_INODE_HASH_TREE);
	//将bh信息复制到bh2
	memcpy((char*)bh2->b_data,(char*)bh->b_data,XCRAFT_BLOCK_SIZE);


	/*更新root*/
	memset((char*)root,0x00,XCRAFT_BLOCK_SIZE);
	root->info.hash_version=(uint8_t)XCRAFT_HTREE_VERSION;
	// root->info.count=cpu_to_le16(1);//目录项数目
	root->info.indirect_levels=0;
	// root->info.limit=cpu_to_le16(dx_root_limit());
	entries=root->entries;
	dx_set_block(entries,bno);//bno是物理块号
	dx_set_count(entries,1);
	dx_set_limit(entries,dx_root_limit());
	pr_debug("set dx_root success\n");


	memset(frames, 0, sizeof(frames));
	frame=frames;
	frame->entries=entries;
	frame->at=entries;
	frame->bh=bh;

	hinfo.hash = 0;
	hinfo.hash_version = XCRAFT_HTREE_VERSION;
	// memcpy(hinfo.seed, sb_info->s_hash_seed, sizeof(sb_info->s_hash_seed));
	
	pr_debug("hinfo赋值成功\n");
	//计算新增目录项的hash值
	XCraft_dirhash(qstr->name,qstr->len,&hinfo);
	pr_debug("XCraft_make_hash_tree 计算hash值成功\n");
	//将bh2分裂 bh2最后是两个块中应该插入新目录项的块
	de = do_split(dir,&bh2,frame,&hinfo);
	pr_debug("do_split成功\n");
	if(!de){
		retval=-ENOMEM;
		goto out_frames;
	}
	//将新目录项插入到目录中
	retval = add_dirent_to_buf(dentry,inode,de,bh2);

out_frames:
	mark_inode_dirty(dir);
	mark_buffer_dirty(bh);
	mark_buffer_dirty(bh2);
	dx_release(frames);
	brelse(bh2);
    return retval;
}


#endif