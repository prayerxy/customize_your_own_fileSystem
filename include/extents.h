#ifndef _XCRAFT_EXTENTS_H
#define _XCRAFT_EXTENTS_H
#include "XCraft.h"
#include "bitmap.h"
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>

/*扩展树幻数*/
#define XCRAFT_EXT_MAGIC cpu_to_le16(0xf30b)
/*最大深度*/
#define XCRAFT_MAX_EXTENT_DEPTH 5
/*一个extent有几个物理磁盘块*/
#define XCRAFT_EXT_BLOCKS_NUM 5

/*一个XCraft_extent的ee_len最大值*/
#define XCRAFT_INIT_MAX_LEN 100

#define EFSBADCRC EBADMSG    /* Bad CRC detected */
#define EFSCORRUPTED EUCLEAN /* Filesystem is corrupted */
#define XCRAFT_EXT_MAX_BLOCKS 0xffffffff

// extent和idx的一系列宏定义
// 获取第一个extent
#define XCRAFT_FIRST_EXTENT(__hdr__)                \
    ((struct XCraft_extent *)(((char *)(__hdr__)) + \
                              sizeof(struct XCraft_extent_header)))
// 获取第一个index
#define XCRAFT_FIRST_INDEX(__hdr__)                     \
    ((struct XCraft_extent_idx *)(((char *)(__hdr__)) + \
                                  sizeof(struct XCraft_extent_header)))

// 是否有空闲的index
#define XCRAFT_HAS_FREE_INDEX(__path__) \
    (le16_to_cpu((__path__)->p_hdr->eh_entries) < le16_to_cpu((__path__)->p_hdr->eh_max))
// 获取最后一个可用的extent
#define XCRAFT_LAST_EXTENT(__hdr__) \
    (XCRAFT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
// 获取最后一个可用的index
#define XCRAFT_LAST_INDEX(__hdr__) \
    (XCRAFT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_entries) - 1)
// 获取最大的extent
#define XCRAFT_MAX_EXTENT(__hdr__)                                                                              \
    ((le16_to_cpu((__hdr__)->eh_max)) ? ((XCRAFT_FIRST_EXTENT((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)) \
                                      : NULL)
// 获取最大的index
#define XCRAFT_MAX_INDEX(__hdr__)                                                                              \
    ((le16_to_cpu((__hdr__)->eh_max)) ? ((XCRAFT_FIRST_INDEX((__hdr__)) + le16_to_cpu((__hdr__)->eh_max) - 1)) \
                                      : NULL)

// 获取根节点的header
static struct XCraft_extent_header *get_ext_inode_hdr(struct XCraft_inode_info *xi)
{
    return (struct XCraft_extent_header *)xi->i_block;
}

// 获取扩展树深度
static int get_ext_tree_depth(struct inode *inode)
{
    struct XCraft_inode_info *xi;
    xi = XCRAFT_I(inode);
    return le16_to_cpu(get_ext_inode_hdr(xi)->eh_depth);
}

// 根节点最多容纳几个extent
static int XCraft_ext_space_root(struct XCraft_inode_info *xi)
{
    int size;

    size = sizeof(xi->i_block);
    size -= sizeof(struct XCraft_extent_header);
    size /= sizeof(struct XCraft_extent);

    return size;
}

// 根节点最多容纳几个索引idx项
static int XCraft_ext_space_root_idx(struct XCraft_inode_info *xi)
{
    int size;

    size = sizeof(xi->i_block);
    size -= sizeof(struct XCraft_extent_header);
    size /= sizeof(struct XCraft_extent_idx);

    return size;
}

// 叶子节点最多容纳多少个extent
static int XCraft_ext_space_leaf(struct inode *inode)
{
    int size;
    size = (inode->i_sb->s_blocksize - sizeof(struct XCraft_extent_header)) / sizeof(struct XCraft_extent);
    return size;
}

// 索引节点最多容纳多少个idx
static int XCraft_ext_space_idx(struct inode *inode)
{
    int size;
    size = (inode->i_sb->s_blocksize - sizeof(struct XCraft_extent_header)) / sizeof(struct XCraft_extent_idx);
    return size;
}

// extent tree初始化
static void XCraft_ext_tree_init(struct inode *inode)
{
    struct XCraft_extent_header *eh;
    struct XCraft_inode_info *xi = XCRAFT_I(inode);
    eh = get_ext_inode_hdr(xi);

    eh->eh_magic = XCRAFT_EXT_MAGIC;
    eh->eh_entries = 0;
    eh->eh_max = cpu_to_le16(XCraft_ext_space_root(xi));
    eh->eh_depth = 0;
    eh->eh_unused = 0;
    mark_inode_dirty(inode);
}

// ##连续物理块的获取，返回起始物理块号
static int XCraft_get_new_ext_blocks(struct XCraft_superblock_info *sbi, int len)
{
    int newblock = get_free_blocks(sbi, len);
    return newblock;
}

// 搜索路径path释放
static void XCraft_ext_drop_refs(struct XCraft_ext_path *path)
{
    int depth, i;
    if (!path)
        return;
    depth = path->p_depth;
    // 释放路径中的每一层的buffer_head
    for (i = 0; i <= depth; i++, path++)
    {
        if (path->p_bh)
            brelse(path->p_bh);
        path->p_bh = NULL;
    }
}

// 索引节点中二分查找
static void XCraft_ext_binsearch_idx(struct inode *inode,
                              struct XCraft_ext_path *path, unsigned int block)
{
    // get header
    struct XCraft_extent_header *eh = path->p_hdr;
    // 二分查找右，左，中
    struct XCraft_extent_idx *r, *l, *m;
    // 进行二分查找
    l = XCRAFT_FIRST_INDEX(eh) + 1;
    r = XCRAFT_LAST_INDEX(eh);
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (block < le32_to_cpu(m->ei_block))
            r = m - 1;
        else
            l = m + 1;
    }

    path->p_idx = l - 1;
}

// 叶子节点中二分查找
static void XCraft_ext_binsearch(struct inode *inode, struct XCraft_ext_path *path, unsigned int block)
{
    // get header
    struct XCraft_extent_header *eh = path->p_hdr;
    struct XCraft_extent *r, *l, *m;

    // 通过计数来判断是否为空
    if (eh->eh_entries == 0)
        return;

    l = XCRAFT_FIRST_EXTENT(eh) + 1;
    r = XCRAFT_LAST_EXTENT(eh);
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (block < le32_to_cpu(m->ee_block))
            r = m - 1;
        else
            l = m + 1;
    }
    path->p_ext = l - 1;
}

// 搜索路径获取   搜索逻辑块号block 存储路径至orig_path
static struct XCraft_ext_path *
XCraft_find_extent(struct inode *inode, unsigned int block,
                   struct XCraft_ext_path **orig_path)
{
    struct XCraft_extent_header *eh;
    struct buffer_head *bh;
    struct XCraft_ext_path *path = orig_path ? *orig_path : NULL;
    struct XCraft_inode_info *inode_info = XCRAFT_I(inode);
    // 超级块
    struct super_block *sb = inode->i_sb;
    short int depth, i, pos = 0;
    int ret;
    ret = 0;
    // 获取根节点的header
    eh = get_ext_inode_hdr(inode_info);
    // 获取树的深度
    depth = get_ext_tree_depth(inode);
    // 检查深度是否合理
    if (depth < 0 || depth > XCRAFT_MAX_EXTENT_DEPTH)
    {
        ret = -EFSCORRUPTED;
        goto err;
    }

    if (path)
    {
        XCraft_ext_drop_refs(path);
        if (depth > path[0].p_maxdepth)
        {
            kfree(path);
            *orig_path = path = NULL;
        }
    }

    if (!path)
    {
        path = kcalloc(depth + 2, sizeof(struct XCraft_ext_path), GFP_NOFS);
        // path = kzalloc(sizeof(struct XCraft_ext_path) * (depth + 2), GFP_KERNEL);
        if (unlikely(!path))
            return ERR_PTR(-ENOMEM);
        path[0].p_maxdepth = depth + 1;
    }
    path[0].p_hdr = eh;
    path[0].p_bh = NULL;

    i = depth;
    // 逐层遍历，二分查找
    while (i)
    {
        // 二分查找确定idx
        XCraft_ext_binsearch_idx(inode, path + pos, block);
        path[pos].p_block = le32_to_cpu(path[pos].p_idx->ei_leaf);
        path[pos].p_depth = i;
        path[pos].p_ext = NULL;

        --i;
        // 读下一层内容
        bh = sb_bread(sb, path[pos].p_block);
        if (IS_ERR(bh))
        {
            ret = PTR_ERR(bh);
            goto err;
        }

        eh = (struct XCraft_extent_header *)bh->b_data;
        pos++;
        path[pos].p_bh = bh;
        path[pos].p_hdr = eh;
    }

    // 叶子节点层
    path[pos].p_depth = i;
    path[pos].p_ext = NULL;
    path[pos].p_idx = NULL;

    // 叶子节点层进行二分查找
    XCraft_ext_binsearch(inode, path + pos, block);

    // 分两种情况：1. 叶子节点为空；2. 叶子节点非空
    if (path[pos].p_ext)
        path[pos].p_block = le32_to_cpu(path[pos].p_ext->ee_start);

    return path;

err:
    XCraft_ext_drop_refs(path);
    kfree(path);
    if (orig_path)
        *orig_path = NULL;

    return ERR_PTR(ret);
}

// 寻找下一个分配了extent的起始逻辑块号
static unsigned int find_next_allocated_block(struct XCraft_ext_path *path)
{
    int depth;
    // path不能为空
    BUG_ON(path == NULL);
    depth = path->p_depth;

    // 深度为0并且此时没有任何extent
    if (depth == 0 && path->p_ext == NULL)
        return XCRAFT_EXT_MAX_BLOCKS;

    while (depth >= 0)
    {
        struct XCraft_ext_path *p = &path[depth];

        if (depth == path->p_depth)
        {
            /* leaf */
            /*不是最后一个extent找下一个*/
            if (p->p_ext && p->p_ext != XCRAFT_LAST_EXTENT(p->p_hdr))
                return le32_to_cpu(p->p_ext[1].ee_block);//p_ext[1]找的是右边的extent
        }
        else
        {
            /* index */
            //   当前path中的extent是叶子节点中的最后一个，所以回溯上层索引节点
            if (p->p_idx != XCRAFT_LAST_INDEX(p->p_hdr))
                return le32_to_cpu(p->p_idx[1].ei_block);
        }
        depth--;
    }

    return XCRAFT_EXT_MAX_BLOCKS;
}

// 检查重复区域并去除
// inode:文件inode   newext:要插入的extent   path:搜索路径
static unsigned int XCraft_check_overlap(struct inode *inode,
                                  struct XCraft_extent *newext,
                                  struct XCraft_ext_path *path)
{
    // b1为新的extent的起始逻辑块号，b2为extent起始逻辑块号
    unsigned int b1, b2;
    unsigned int depth, len1;
    unsigned int ret = 0;

    b1 = le32_to_cpu(newext->ee_block);
    len1 = le32_to_cpu(newext->ee_len);
    depth = get_ext_tree_depth(inode);

    // 最后一层没有extent则不用检查是否重叠
    if (!path[depth].p_ext)
        goto out;//说明是一开始插入的extent
    b2 = le32_to_cpu(path[depth].p_ext->ee_block);

    // 说明此时extent与新的extent没有重叠区域，另外一种情况函数外面已经判断
    if (b2 < b1)
    {
        // 寻找下一个分配的起始逻辑块号
        b2 = find_next_allocated_block(path);
        if (b2 == XCRAFT_EXT_MAX_BLOCKS)
            goto out;
    }

    // 检查逻辑起始块号是否溢出
    if (b1 + len1 < b1)
    {
        len1 = XCRAFT_EXT_MAX_BLOCKS - b1;
        newext->ee_len = cpu_to_le32(len1);
        ret = 1;
    }

    // 检查是否重叠，如果重叠需要进行更新
    if (b1 + len1 > b2)
    {
        // 重叠区域需要去除
        newext->ee_len = cpu_to_le32(b2 - b1);
        ret = 1;
    }
out:
    return ret;
}

// 检查extent是否可以合并，其中ex1在ex2前面
static int XCraft_ext_can_merge(struct XCraft_extent *ex1, struct XCraft_extent *ex2)
{
    // 获取extent的长度
    unsigned int ex1_len, ex2_len, ex1_block, ex2_block;
    unsigned int ex1_pblock, ex2_pblock;
    ex1_block = le32_to_cpu(ex1->ee_block);
    ex2_block = le32_to_cpu(ex2->ee_block);
    ex1_len = le32_to_cpu(ex1->ee_len);
    ex2_len = le32_to_cpu(ex2->ee_len);
    ex1_pblock = le32_to_cpu(ex1->ee_start);
    ex2_pblock = le32_to_cpu(ex2->ee_start);

    // 合并条件：物理块和逻辑块都要连续,长度叠加不溢出
    if (ex1_block + ex1_len == ex2_block && ex1_len + ex2_len <= XCRAFT_INIT_MAX_LEN && ex1_pblock + ex1_len == ex2_pblock)
        return 1;
    // 合并不成功
    return 0;
}

// 向右合并函数
static int XCraft_merge_ext_right(struct inode *inode, struct XCraft_ext_path *path, struct XCraft_extent *ex)
{
    struct XCraft_extent_header *eh;
    unsigned int depth, len;
    // 是否经过合并
    int merge_done = 0;
    // 深度
    depth = get_ext_tree_depth(inode);
    struct buffer_head *bh;
    BUG_ON(path[depth].p_hdr == NULL);
    eh = path[depth].p_hdr;
    bh = path[depth].p_bh;

    while (ex < XCRAFT_LAST_EXTENT(eh)) {
        if (!XCraft_ext_can_merge(ex, ex+1))
            break;
        // 可以向后合并
        ex->ee_len = cpu_to_le32(le32_to_cpu(ex->ee_len) + le32_to_cpu((ex+1)->ee_len));
        if (ex + 1 < XCRAFT_LAST_EXTENT(eh)){
            // 后面的extent前移
            len = (XCRAFT_LAST_EXTENT(eh) - ex - 1) * sizeof(struct XCraft_extent);
            memmove(ex + 1, ex + 2, len);
        }
        // 更新header计数
        eh->eh_entries = cpu_to_le16(le16_to_cpu(eh->eh_entries)-1);
        merge_done = 1;
    }
    if (bh){
        if (merge_done){
            mark_buffer_dirty(bh);
            brelse(bh);
        }
    }
    return merge_done;
}

// 进行合并操作函数
static void XCraft_merge_ext(struct inode *inode, struct XCraft_ext_path *path, struct XCraft_extent *ex)
{
    struct XCraft_extent_header *hdr;
    unsigned int depth;

    int merge_done = 0;

    depth = get_ext_tree_depth(inode);
    BUG_ON(path[depth].p_hdr == NULL);
    hdr = path[depth].p_hdr;

    if (ex > XCRAFT_FIRST_EXTENT(hdr))
        // 起始与左边合并
        merge_done = XCraft_merge_ext_right(inode, path, ex-1);
    if (!merge_done)
        merge_done = XCraft_merge_ext_right(inode, path, ex);
}

// 自下而上更新索引
static int XCraft_correct_indexes(struct inode *inode, struct XCraft_ext_path *path)
{
    struct XCraft_extent_header *eh;
    int depth = get_ext_tree_depth(inode);
    struct XCraft_extent *ex;
    __le32 border;
    int k;

    eh = path[depth].p_hdr;
    ex = path[depth].p_ext;

    if (unlikely(ex == NULL || eh == NULL))
        return -EFSCORRUPTED;
    
    if (depth == 0) {
        return 0;
    }

    // 必须保证插入的extent是叶子节点的第一个extent
    if (ex != XCRAFT_FIRST_EXTENT(eh)) {
        return 0;
    }
    // 顺次向上调整
    k = depth - 1;
    border = path[depth].p_ext->ee_block;
    path[k].p_idx->ei_block = border;
    if (path[k].p_bh)
        mark_buffer_dirty(path[k].p_bh);

    while(k--){
        if (path[k+1].p_idx != XCRAFT_FIRST_INDEX(path[k+1].p_hdr))
            break;
        // 不是第一个就不用继续向上
        path[k].p_idx->ei_block = border;
        if (path[k].p_bh)
            // 更新
            mark_buffer_dirty(path[k].p_bh);
    }
    return 0;

}

// 寻找下一个叶子节点中的第一个extent的起始逻辑块号
static unsigned int XCraft_ext_next_leaf_block(struct XCraft_ext_path *path)
{
    int depth;

    BUG_ON(path == NULL);
    depth = path->p_depth;

    if (depth == 0)
        // 最底层本来就没有了
        return XCRAFT_EXT_MAX_BLOCKS;

    depth--;
    // 从最底层索引块开始找
    while (depth >= 0)
    {
        if (path[depth].p_idx != XCRAFT_LAST_INDEX(path[depth].p_hdr))
            return le32_to_cpu(path[depth].p_idx[1].ei_block);
        depth--;
    }
    return XCRAFT_EXT_MAX_BLOCKS;
}

// 插入子树到扩展树
static int XCraft_ext_insert_index(struct inode *inode, struct XCraft_ext_path *curp, unsigned int lblk, unsigned int pblk)
{
    struct XCraft_extent_idx *ix;
    int len, err;
    err = 0;

    if (lblk > le32_to_cpu(curp->p_idx->ei_block))
    {
        ix = curp->p_idx + 1;
    }
    else
    {
        ix = curp->p_idx;
    }

    len = XCRAFT_LAST_INDEX(curp->p_hdr) - ix + 1;
    BUG_ON(len < 0);
    if (len > 0)
        // 插入位置后面的后移一位操作
        memmove(ix + 1, ix, len * sizeof(struct XCraft_extent_idx));

    // 判断
    if (unlikely(ix > XCRAFT_MAX_INDEX(curp->p_hdr)))
        return -EFSCORRUPTED;

    ix->ei_block = cpu_to_le32(lblk);
    ix->ei_leaf = cpu_to_le32(pblk);
    curp->p_hdr->eh_entries = cpu_to_le16(le16_to_cpu(curp->p_hdr->eh_entries) + 1);

    if (unlikely(ix > XCRAFT_LAST_INDEX(curp->p_hdr)))
        return -EFSCORRUPTED;

    if (curp->p_bh)
        // 回写
        mark_buffer_dirty(curp->p_bh);
    return err;
}

// 不增加深度情况，创建子树
static int XCraft_ext_split(struct inode *inode, struct XCraft_ext_path *path,
                     struct XCraft_extent *newext, int at)
{
    struct buffer_head *bh = NULL;
    struct super_block *sb = inode->i_sb;
    struct XCraft_superblock_info *sbi = XCRAFT_SB(sb);
    struct XCraft_extent_header *neh = NULL;
    struct XCraft_extent_idx *fidx;
    __le32 border;
    int depth = get_ext_tree_depth(inode);
    unsigned int *alloc_blocks = NULL; /*分配物理块数组*/
    int err = 0;
    int i = at, k, a, m;
    size_t ext_size = 0;

    unsigned int newblock, oldblock;

    // 判断path中的extent是否已经是最大位置处的extent
    if (path[depth].p_ext != XCRAFT_MAX_EXTENT(path[depth].p_hdr))
        // 后续涉及到一些分裂操作
        border = path[depth].p_ext[1].ee_block;
    else
        // 后面直接插即可
        border = newext->ee_block;

    // alloc_blocks = kzalloc(sizeof(unsigned int) * depth, GFP_KERNEL);
    alloc_blocks = kcalloc(depth, sizeof(unsigned int), GFP_NOFS);
    if (!alloc_blocks)
        return -ENOMEM;
    // 初始化为全0
    memset(alloc_blocks, 0, sizeof(alloc_blocks));
    for (a = 0; a < depth - at; a++)
    {
        newblock = get_free_blocks(sbi, 1);
        if (newblock == 0)
            goto cleanup;
        alloc_blocks[a] = newblock;
    }
    // a = depth - at
    newblock = alloc_blocks[--a];
    if (unlikely(newblock == 0))
    {
        err = -EFSCORRUPTED;
        goto cleanup;
    }

    // 令bh映射newblock代表的物理块
    bh = sb_bread(sb, newblock);
    if (unlikely(!bh))
    {
        err = -ENOMEM;
        goto cleanup;
    }

    // 新创建的叶子节点块进行内容赋值
    neh = (struct XCraft_extent_header *)bh->b_data;
    neh->eh_entries = 0;
    neh->eh_max = cpu_to_le16(XCraft_ext_space_leaf(inode));
    neh->eh_magic = XCRAFT_EXT_MAGIC;
    neh->eh_depth = 0;
    neh->eh_unused = 0;

    // 如果extent不是最后一个extent，那么对新块进行内容copy一部分
    m = XCRAFT_LAST_EXTENT(path[depth].p_hdr) - path[depth].p_ext++;
    if (m)
    {
        // 进行copy
        struct XCraft_extent *ex;
        ex = XCRAFT_FIRST_EXTENT(neh);
        memmove(ex, path[depth].p_ext, sizeof(struct XCraft_extent) * m);
        neh->eh_entries = cpu_to_le16(le16_to_cpu(neh->eh_entries) + m);
    }

    // 新的叶子节点块剩余部分置0
    ext_size = sizeof(struct XCraft_extent_header) + sizeof(struct XCraft_extent) * le16_to_cpu(neh->eh_entries);
    memset(bh->b_data + ext_size, 0, sb->s_blocksize - ext_size);

    // 将bh标记为脏，回写
    mark_buffer_dirty(bh);
    brelse(bh);
    bh = NULL;

    /*对旧的叶子节点进行修正*/
    if (m)
    {
        path[depth].p_hdr->eh_entries = cpu_to_le16(le16_to_cpu(path[depth].p_hdr->eh_entries) - m);
        // 回写
        mark_buffer_dirty(path[depth].p_bh);
    }

    // 创建索引节点
    // 需要创建的索引节点个数为上述减去叶子节点1
    k = depth - at - 1;
    if (unlikely(k < 0))
    {
        err = -EFSCORRUPTED;
        goto cleanup;
    }

    i = depth - 1; // 最底层的索引节点
    while (k--)
    {
        oldblock = newblock;
        newblock = alloc_blocks[--a];
        bh = sb_bread(sb, newblock);
        if (unlikely(!bh))
        {
            err = -ENOMEM;
            goto cleanup;
        }

        if (err)
            goto cleanup;

        neh = (struct XCraft_extent_header *)bh->b_data;
        neh->eh_entries = cpu_to_le16(1);
        neh->eh_magic = XCRAFT_EXT_MAGIC;
        neh->eh_max = cpu_to_le16(XCraft_ext_space_idx(inode));
        neh->eh_depth = cpu_to_le16(depth - i);
        neh->eh_unused = 0;
        // 获取第一个index，对其信息进行赋值
        fidx = XCRAFT_FIRST_INDEX(neh);
        fidx->ei_block = border;
        fidx->ei_leaf = cpu_to_le32(oldblock);
        fidx->ei_unused = 0;

        // index 信息进行copy
        m = XCRAFT_LAST_INDEX(path[i].p_hdr) - path[i].p_idx++;
        if (m)
        {
            memmove(++fidx, path[i].p_idx, sizeof(struct XCraft_extent_idx) * m);
            neh->eh_entries = cpu_to_le16(le16_to_cpu(neh->eh_entries) + m);
        }

        // 剩余部分置0
        ext_size = sizeof(struct XCraft_extent_header) +
                   (sizeof(struct XCraft_extent_idx) * le16_to_cpu(neh->eh_entries));

        memset(bh->b_data + ext_size, 0, sb->s_blocksize - ext_size);
        // 进行回写
        mark_buffer_dirty(bh);
        brelse(bh);
        bh = NULL;

        // 对旧的index信息进行更新
        if (m)
        {
            path[i].p_hdr->eh_entries = cpu_to_le16(le16_to_cpu(path[i].p_hdr->eh_entries) - m);
            mark_buffer_dirty(path[i].p_bh);
        }
        i--;
    }

    /*将子树插入到扩展树中*/
    err = XCraft_ext_insert_index(inode, path + at,
                                  le32_to_cpu(border), newblock);
cleanup:
    if (bh)
        brelse(bh);

    if (err)
    {
        for (i = 0; i < depth; i++)
        {
            if (!alloc_blocks[i])
                continue;
            put_blocks(sbi, alloc_blocks[i], 1);
        }
    }
    if (alloc_blocks)
        kfree(alloc_blocks);
    return err;
}

// 需要增加深度此时，增加一层索引节点
static int XCraft_grow_indepth(struct inode *inode)
{
    struct XCraft_extent_header *neh;
    struct XCraft_extent_idx *fidx;
    struct buffer_head *bh;
    // 增加深度时给新的索引节点分配的物理块号
    unsigned int newblock;
    struct super_block *sb = inode->i_sb;
    struct XCraft_superblock_info *sbi = XCRAFT_SB(sb);
    struct XCraft_inode_info *inode_info = XCRAFT_I(inode);
    size_t ext_size = 0;
    int err, depth;

    // 两种情况:
    // 1. 根节点此时是叶子节点 叶子节点变为索引节点
    // 2. 根节点此时是索引节点 索引节点变为索引节点
    err = 0;
    // 获取深度
    depth = get_ext_tree_depth(inode);

    // 分配物理块
    newblock = get_free_blocks(sbi, 1);
    if (!newblock)
    {
        err = -ENOSPC;
        goto end;
    }

    // 读取新块
    bh = sb_bread(sb, newblock);
    if (!bh)
    {
        err = -EIO;
        goto end;
    }

    ext_size = sizeof(inode_info->i_block);
    // 复制内容
    memmove(bh->b_data, inode_info->i_block, ext_size);
    // 新块剩余部分置为0
    memset(bh->b_data + ext_size, 0, sb->s_blocksize - ext_size);

    // 新块header内容进行设置，特别是eh_max发生了变化
    neh = (struct XCraft_extent_header *)bh->b_data;

    if (depth) // 复制的是idx
        neh->eh_max = cpu_to_le16(XCraft_ext_space_idx(inode));
    else // 复制的是extent
        neh->eh_max = cpu_to_le16(XCraft_ext_space_leaf(inode));

    neh->eh_magic = XCRAFT_EXT_MAGIC;
    mark_buffer_dirty(bh);
    brelse(bh);

    // 然后对根节点信息进行更新
    neh = get_ext_inode_hdr(inode_info);
    // 重置根节点entries数目
    neh->eh_entries = cpu_to_le16(1);
    // 获取第一个idx
    fidx = XCRAFT_FIRST_INDEX(neh);
    // 物理块号赋值
    fidx->ei_leaf = cpu_to_le32(newblock);

    // 考虑ei_block赋值
    if (neh->eh_depth == 0)
    {
        // 此时之前是extent
        neh->eh_max = cpu_to_le16(XCraft_ext_space_root_idx(inode_info));
        // 起始逻辑地址为原来叶子节点第一个extent结构的逻辑地址
        fidx->ei_block = XCRAFT_FIRST_EXTENT(neh)->ee_block;
    }

    // 更新深度，表示成功加了一层
    neh->eh_depth = cpu_to_le16(le16_to_cpu(neh->eh_depth) + 1);
    // 更新inode
    mark_inode_dirty(inode);
end:
    return err;
}

// 创建新的叶子节点
static int XCraft_create_new_leaf(struct inode *inode, struct XCraft_ext_path **ppath, struct XCraft_extent *newext)
{
    pr_debug("begin create_new_leaf\n");
    struct XCraft_ext_path *path = *ppath;
    struct XCraft_ext_path *curp;
    int depth, i, err = 0;

repeat:
    i = depth = get_ext_tree_depth(inode);

    // 向上找空闲的索引节点
    curp = path + depth;
    while (i > 0 && !XCRAFT_HAS_FREE_INDEX(curp))
    {
        i--;
        curp--;
    }

    // 判断找到的curp中是否有空闲的idx
    // 向上减完就是根节点
    if (XCRAFT_HAS_FREE_INDEX(curp))
    {
        pr_debug("begin ext_split\n");
        // 创建子树，不需要增加深度
        err = XCraft_ext_split(inode, path, newext, i);
        if (err)
            goto out;
        pr_debug("over XCraft_ext_split\n");

        // 此时更新path
        path = XCraft_find_extent(inode, le32_to_cpu(newext->ee_block), ppath);
        if (IS_ERR(path))
            err = PTR_ERR(path);
    }
    else
    {
        // 树满了，需要增加深度
        // 增加深度要看现在是否达到最大深度
        if (depth == XCRAFT_MAX_EXTENT_DEPTH){
            // 达到最大深度不能继续增加深度
            err = -EFSCORRUPTED;
            goto out;
        }
        pr_debug("begin grow_indepth, lblk is %d\n", le32_to_cpu(newext->ee_block));
        err = XCraft_grow_indepth(inode);
        if (err)
            goto out;
        pr_debug("over grow_indepth\n");
        path = XCraft_find_extent(inode, le32_to_cpu(newext->ee_block), ppath);
        if (IS_ERR(path))
        {
            err = PTR_ERR(path);
            goto out;
        }

        depth = get_ext_tree_depth(inode);
        // 最后一层叶子节点中extent仍然是满的话，需要重新来生成叶子节点
        if (path[depth].p_hdr->eh_entries == path[depth].p_hdr->eh_max)
        {
            goto repeat;
        }
    }

out:
    return err;
}

// 插入extent的核心函数
// inode:文件inode   ppath:搜索路径   newext:要插入的extent
static int XCraft_ext_insert_extent(struct inode *inode, struct XCraft_ext_path **ppath, struct XCraft_extent *newext)
{
    struct XCraft_ext_path *path = *ppath;
    struct XCraft_extent_header *eh;
    struct XCraft_extent *ex, *fex;
    struct XCraft_extent *nearex; /* nearest extent */
    struct XCraft_ext_path *npath = NULL;
    int depth, len, err;
    unsigned int next;

    // 获取当前深度
    depth = get_ext_tree_depth(inode);
    // path叶子节点中的header
    eh = path[depth].p_hdr;
    // path叶子节点中的extent
    ex = path[depth].p_ext;

    // err初始化
    err = 0;
    if (unlikely(newext->ee_len == 0))
        return -EFSCORRUPTED;

    if (ex)
    // 叶子节点层不为空
    {
        // 尝试进行合并
        if (ex < XCRAFT_LAST_EXTENT(eh) &&
            (le32_to_cpu(ex->ee_block) +
                 le32_to_cpu(ex->ee_len) <
             le32_to_cpu(newext->ee_block)))
        {
            ex += 1;
            goto nextcompare;
        }

        // 再看ex和newex能否合并
        if (XCraft_ext_can_merge(ex, newext))
        {
            ex->ee_len = cpu_to_le32(le32_to_cpu(ex->ee_len) + le32_to_cpu(newext->ee_len));
            eh = path[depth].p_hdr;
            nearex = ex;
            pr_debug("the merge condition is ok between ex and newext\n");
            goto merge;
        }

    nextcompare:

        if (XCraft_ext_can_merge(newext, ex))
        {
            ex->ee_block = newext->ee_block;
            ex->ee_len = cpu_to_le32(le32_to_cpu(ex->ee_len) + le32_to_cpu(newext->ee_len));
            ex->ee_start = newext->ee_start;
            eh = path[depth].p_hdr;

            nearex = ex;
            pr_debug("the merge condition is ok between newext and ex\n");
            goto merge;
        }
    }
    // 合并不了就要在扩展树里面插入新的extent
    depth = get_ext_tree_depth(inode);
    eh = path[depth].p_hdr;
    // 判断现在的叶子节点中extent是否已经满了
    if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max))
        // 此时extent没有满，可以直接插入
        goto has_space;

    // 当前叶子节点中extent满了
    fex = XCRAFT_LAST_EXTENT(eh);
    next = XCRAFT_EXT_MAX_BLOCKS;
    if (le32_to_cpu(newext->ee_block) > le32_to_cpu(fex->ee_block))
        // 观察下一个叶子节点中是否有位置
        next = XCraft_ext_next_leaf_block(path);
    if (next != XCRAFT_EXT_MAX_BLOCKS)
    {
        // 下一个叶子节点中是否有位置     next是最下面的最小值的复写  直接找第一个extent
        npath = XCraft_find_extent(inode, next, NULL);
        if (IS_ERR(npath))
            return PTR_ERR(npath);
        eh = npath[depth].p_hdr;
        if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max))
        {
            // 有位置，路径赋值
            path = npath;
            goto has_space;
        }
    }

    // 必须创建新的叶子节点，在里面进行path路径更新
    err = XCraft_create_new_leaf(inode, &path, newext);
    if (err)
        goto cleanup;
    pr_debug("over create_new_leaf\n");
    depth = get_ext_tree_depth(inode);
    eh = path[depth].p_hdr;

has_space:
    // 此时有空间来插入新的extent
    nearex = path[depth].p_ext;
    if (!nearex)
    {
        pr_debug("########\n");
        // 此时直接插第一个
        nearex = XCRAFT_FIRST_EXTENT(eh);
    }
    else
    {
        // 新插入的extent的起始逻辑块号大于目前的块号则插入位置下一个
        // 小于或等于则不变
        if (le32_to_cpu(newext->ee_block) > le32_to_cpu(nearex->ee_block))
            nearex++;
        len = XCRAFT_LAST_EXTENT(eh) - nearex + 1;
        if (len > 0)
        {
            // 后面的向后偏移一位
            memmove(nearex + 1, nearex, len * sizeof(struct XCraft_extent));
        }
    }

    // 叶子节点层的计数需要加1
    eh->eh_entries = cpu_to_le16(le16_to_cpu(eh->eh_entries) + 1);
    path[depth].p_ext = nearex;
    // 插入位置的extent信息更新
    nearex->ee_block = newext->ee_block;
    nearex->ee_start = newext->ee_start;
    nearex->ee_len = newext->ee_len;
merge:
    // 将nearex与周围的进行合并
    XCraft_merge_ext(inode, path, nearex);

    // 如果最后更新的是叶子节点的最左边的extent，索引节点自下至上改变
    err = XCraft_correct_indexes(inode, path);
    if (err)
        goto cleanup;

    if (path[depth].p_bh)
        mark_buffer_dirty(path[depth].p_bh);
    mark_inode_dirty(inode);
    pr_debug("after update, depth is %d\n", depth);

cleanup:
    // 对path进行释放清理
    XCraft_ext_drop_refs(path);
    if (path)
        kfree(path);
    return err;
}

// 执行映射的核心函数
static int XCraft_ext_map_blocks(struct inode *inode, struct XCraft_map_blocks *map, int create)
{
    pr_debug("map->m_lblk = %d\n",map->m_lblk);
    // 搜索路径存储
    struct XCraft_ext_path *path = NULL;
    struct super_block *sb = inode->i_sb;
    struct XCraft_superblock_info *sbi = XCRAFT_SB(sb);
    
    struct XCraft_extent newex, *ex;
    int err = 0, depth;
    int newblock = 0;
    // 最终成功映射的物理块长度
    int allocated = 0;

    // 获取搜索路径
    path = XCraft_find_extent(inode, map->m_lblk, NULL);
    // path是否获取失败
    if (IS_ERR(path))
    {
        err = PTR_ERR(path);
        path = NULL;
        goto out;
    }

    // 获取深度
    depth = get_ext_tree_depth(inode);

    pr_debug("begin in ext_map_blocks, depth is %d\n", depth);
    // 叶子节点层的extent
    ex = path[depth].p_ext;
    if (ex)
    {
        unsigned int ee_block, ee_start, ee_len;
        ee_block = le32_to_cpu(ex->ee_block);
        ee_start = le32_to_cpu(ex->ee_start);
        ee_len = le32_to_cpu(ex->ee_len);
        // 是否在ex的范围里面
        if (map->m_lblk >= ee_block && map->m_lblk < ee_block + ee_len)
        {
            newblock = map->m_lblk - ee_block + ee_start;
            // 成功映射的物理块个数
            allocated = ee_len - (map->m_lblk - ee_block);

            // 填写映射信息
            map->m_pblk = newblock;
            if (allocated > map->m_len)
                allocated = map->m_len;
            map->m_len = allocated;
            goto out;
        }
    }
    // 运行到这说明此时并没有创建  或者  map->m_lblk不在ex的范围里面
    // 是否创建
    if (!create){
        err = 0;
        allocated = 0;
        goto out;
    }

    // map->m_lblk不在ex的范围里面, 需要分配新块
    // 检查map->m_len
    if (map->m_len > XCRAFT_INIT_MAX_LEN)
        map->m_len = XCRAFT_INIT_MAX_LEN;

    newex.ee_block = cpu_to_le32(map->m_lblk);
    newex.ee_len = cpu_to_le32(map->m_len);
    newex.ee_start = cpu_to_le32(0);

    // 检查重叠区域，并进行去重操作
    err = XCraft_check_overlap(inode, &newex, path);
    // 检查是否进行调整，更新allocated
    if (err)
        // 更新调整
        allocated = le32_to_cpu(newex.ee_len);
    else
        // 没有更新调整
        allocated = map->m_len;

    // 分配连续物理块
    newblock = XCraft_get_new_ext_blocks(sbi, allocated);
    pr_debug("newblock is %d\n", newblock);
    if (!newblock)
        goto out;
    
    pr_debug("newblock get is ok!\n");

    // 初步设置newex信息
    newex.ee_len = cpu_to_le32(allocated);
    newex.ee_start = cpu_to_le32(newblock);

    // 将newex插入到扩展树中
    err = XCraft_ext_insert_extent(inode, &path, &newex);
    if (err)
        // 执行失败
        goto out;
    
    pr_debug("XCraft_ext_insert_extent is ok!");
    // 完成映射信息
    map->m_pblk = newblock;
    map->m_len = allocated;
    pr_debug("XCraft_ext_map_blocks err: %d\n",err);
    pr_debug("XCraft_ext_map_blocks allocated: %d\n", allocated);
    return err ? err : allocated;

out:
    // 释放掉path中的相关内容
    XCraft_ext_drop_refs(path);
    if(path)
        kfree(path);
    pr_debug("XCraft_ext_map_blocks err: %d\n",err);
    pr_debug("XCraft_ext_map_blocks allocated: %d\n", allocated);
    return err ? err : allocated;
}

#endif