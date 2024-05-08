#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>


#include "../include/XCraft.h"
#include "../include/hash.h"
#include "../include/bitmap.h"


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
	struct buffer_head *bh;
	*block=get_free_blocks(XCRAFT_SB(dir->i_sb),1);
	bh=sb_bread(dir->i_sb,*block);
	
	if(!bh){
		printk(KERN_ERR "XCraft: append: no memory\n");
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
	struct XCraft_hash_info *h=hinfo;
	char *base = bh->b_data;
	uint16_t rec_len;

	while((char*)de<base+buflen){
		if(de->inode&&de->name_len){
			//计算Hash
			XCraft_dirhash(de->name,de->name_len,h);
			map_tail--;
			map_tail->hash=h->hash;
			map_tail->offs=((char*)de-base)>>2;
			map_tail->size=le16_to_cpu(de->rec_len);
			count++;
			//存疑
			cond_resched();
		}
		//读取此目录项的长度
		rec_len=le16_to_cpu(de->rec_len);
		//de++是指向下一个目录项
		de=(struct XCraft_dir_entry*)((char*)de+rec_len);
	}
	return count;
}


static void dx_sort_map (struct dx_map_entry *map, unsigned count){
	struct dx_map_entry *p,*q,*top=map+count-1;
	struct dx_map_entry tmp;
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
	int more;
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
	while(count--){
		//找到与to里面对应的from的目录项
		struct XCraft_dir_entry *de=(struct XCraft_dir_entry*)(from+(map->offs<<2));
		rec_len=le16_to_cpu(de->rec_len);

		memcpy(to,de,rec_len);
		//除了rec_len的部分，其他部分清零
		de->inode=0;
		memset(&de->name_len,0,rec_len-offsetof(struct XCraft_dir_entry,name_len));
		//更新map
		map++;
		to+=rec_len;
	}
	return (struct XCraft_dir_entry*)(to-rec_len);
}


/*清理base中的无用目录项，使其变得紧凑
* 返回最后一个entry
*/

static struct XCraft_dir_entry *dx_pack_dirents(struct inode *dir, char *base, unsigned blocksize)
{
	struct XCraft_dir_entry *next, *to, *prev, *de = (struct XCraft_dir_entry *) base;
	unsigned rec_len=0;
	prev=to=de;

	while((char*)de<base+blocksize){
		next = (struct XCraft_dir_entry*)((char*)de + le16_to_cpu(de->rec_len));
		if(de->inode&&de->name_len){
			rec_len=le16_to_cpu(de->rec_len);
			if(de>to)
				memmove(to,de,rec_len);
			prev=to;
			to=(struct XCraft_dir_entry*)((char*)to+rec_len);
		}
		de=next;
	}
	return prev;
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
		while((char *)de <= top && count<XCRAFT_dentry_LIMIT){
			// 检查是否已经存在该目录项
			// 比较是否有同名现象
			if(strncmp(de->name,name,namelen) == 0)
				brelse(bh);
				return -EEXIST;

			// 跳出条件为找到一个目录项的inode号为0，此目录项可用
			if(!de->inode)
				break;
			reclen = le16_to_cpu(de->rec_len);
			// 移动到下一个目录项
			de = (struct XCraft_dir_entry *)((char *)de + reclen);
			// 不是哈希树时我们才受XCRAFT_dentry_LIMIT限制
			if(!flag)
				count++;
		}

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
		memcpy(de->name, name, namelen);
		// 最后一个字符置为空字符
		de->name[namelen] = '\0';
		if(namelen>XCRAFT_NAME_LEN){
			printk("add_dirent_to_buf: name too long\n");
			de->name[XCRAFT_NAME_LEN] = '\0';
			namelen = XCRAFT_NAME_LEN;
			dentry->d_name.len = namelen;
		}
		de->name_len = namelen;
	}

	// dir访问时间更新
	dir->i_atime = dir->i_mtime = dir->i_ctime = current_time(dir);
	mark_inode_dirty(dir);
	// 更新bh
	mark_buffer_dirty(bh);
	brelse(bh);
	return 0;
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

	struct dx_root	*root;
	struct dx_frame frames[XCRAFT_HTREE_LEVEL],*frame;
	struct dx_entry *entries;
	struct buffer_head *bh2;
	struct XCraft_dir_entry*de;
	struct XCraft_hash_info *hinfo;

	struct qstr *qstr = &dentry->d_name;
	uint32_t bno;
	int retval;


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
	// root->info.count=cpu_to_le16(1);//目录项数目
	root->info.indirect_levels=0;
	// root->info.limit=cpu_to_le16(dx_root_limit());
	entries=root->entries;
	dx_set_block(entries,bno);//bno是物理块号
	dx_set_count(entries,1);
	dx_set_limit(entries,dx_root_limit());


	memset(frames, 0, sizeof(frames));
	frame=frames;
	frame->entries=entries;
	frame->at=entries;
	frame->bh=bh;
	//计算新增目录项的hash值
	XCraft_dirhash(qstr->name,qstr->len,hinfo);
	//将bh2分裂 bh2最后是两个块中应该插入新目录项的块
	de=do_split(dir,&bh2,frame,hinfo);
	if(!de){
		retval=-ENOMEM;
		goto out_frames;
	}
	//将新目录项插入到目录中
	retval = add_dirent_to_buf(dentry,inode,de,bh2);




out_frames:
	if(retval)
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
			struct XCraft_hash_info *hinfo)
{
    unsigned blocksize =XCRAFT_BLOCK_SIZE;
	unsigned continued;
	uint32_t bno;
	uint32_t hash2;
	struct buffer_head *bh2;
	char*data1=(*bh)->b_data,*data2;
	struct dx_map_entry *map;
	unsigned split, move, size;
	struct XCraft_dir_entry *de = NULL, *de2;
	int count;
	int i;

	//第三个块
	bh2=XCraft_append(dir,&bno);
	if(!bh2){
		printk(KERN_ERR "XCraft: do_split: no memory\n");
		return NULL;
	}
	data2=bh2->b_data;
	map = (struct dx_map_entry *) (data2 + blocksize);
	count=dx_make_map(dir,*bh,hinfo,map);
	if(count<0){
		printk(KERN_ERR "XCraft: do_split: no memory\n");
		return NULL;
	}
	map-=count;
	dx_sort_map(map, count);

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
	hash2=map[split].hash;
	continued = hash2 == map[split - 1].hash;
	printk("Split block%lu at %x, %i/%i\n", 
			(unsigned long)dx_get_block(frame->at),hash2,split, count-split);
	/*如果split块和split-1的Hash相同，那么split与split-1要在一个块中，所以加上continued*/
	de2=dx_move_dirents(dir,data1,data2,map+split+continued,count-split,blocksize);
	de=dx_pack_dirents(dir,data1,blocksize);
	if(hinfo->hash>=hash2){
		swap(*bh,bh2);
		de=de2;//bh,de始终是要新加目录项的那一个块的信息
	}
	dx_insert_block(frame,hash2+continued,bno);
	brelse(bh2);
	return de;
}