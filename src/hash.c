#include "hash.h"

// hash value to caculate 
// name:文件名 len:文件名长度  hinfo:hash值
int XCraft_dirhash(const char *name, int len, struct XCraft_hash *hinfo){

    return 0;
}


// hash tree to build
// handle:事务 qstr:文件名 dir:目录 inode:文件 bh:缓冲区头
// 在dir下建一个dir_entry   inode是我们目录项索引的文件
// inode->i_ino是ino号
// inode对应的块存放目录项，如果d_entry存满一个块，开始make_hash_tree
//  bh是撑爆的第一个块 进行分裂
static int XCraft_make_hash_tree(handle_t *handle, const struct qstr *qstr,
			    struct inode *dir,
			    struct inode *inode, struct buffer_head *bh)
{

	//先创立root与一个撑满的块
	//再调用do_split,对撑满的块进行分裂
	//最后将新的目录项插入到目录中 完成de->inode的赋值
    return 0;
}

// do_split
// dir是目录inode   bh是进行分裂的块的buffer_head   frame是上一级的dx_entry位置   hinfo是提前计算目录项的Hash值,用于分裂
// 1.分裂块
static struct XCraft_dir_entry *do_split(handle_t *handle, struct inode *dir,
			struct buffer_head **bh,struct XCraft_frame *frame,
			struct XCraft_hash *hinfo)
{
    return NULL;
}