#include "./bplusTree.h"

#include <string.h>

BplusTree::BplusTree(std::string &path, bool empty)
    : _path(nullptr), _fp(nullptr)
{
    /* 打开一个文件，获取其元信息
     * 如果文件不存在或者文件为空，先为其添加一些基本的元信息
     */

    _path = path;

    if(!empty) {
        // 默认所打开的文件不为空，读取文件的meta
        if(read_file(&_meta, META_OFF) != 1) {
            // 读取失败，文件为空
            empty = true;
            close_file();
        }
    }
    if(empty) {
        // 文件为空， 初始化一个文件
        open_file();

        // 初始化一个空文件，即初始化一些元信息
        init_empty_file();
    }
}

off_t BplusTree::allocate(size_t size) {
    off_t slot = _meta.slot;
    _meta.slot += size;
    return slot;
}

off_t BplusTree::allocate(NODE_T &node) {
    // 给一个NODE_T指定储存位置
    node.n = 1;
    _meta.node_n++;
    return allocate(sizeof(NODE_T));
}

off_t BplusTree::allocate(LEAF_NODE_T &node) {
    node.n = 0;
    _meta.leaf_node_n++;
    return allocate(sizeof(LEAF_NODE_T));
}

void BplusTree::init_empty_file() {
    // 初始化文件的元信息，写入文件开始处
    bzero(&_meta, sizeof(_meta));
    
    // 初始化元信息
    _meta.order = BP_ORDER;
    _meta.key_type = NONE;
    _meta.slot = BLOCK_OFF;

    // init tree root
    NODE_T root_node;
    root_node.parent = 0;
    _meta.root_node = allocate(root_node);
    _meta.height = 1;

    //init leaf
    LEAF_NODE_T leaf_node;
    leaf_node.next = 0;
    leaf_node.prev = 0;
    leaf_node.parent = _meta.root_node;
    _meta.leaf_node = root_node.children[0].child = allocate(leaf_node);

    // 写入文件
    
}