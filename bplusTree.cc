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
        close_file();
    }
}

BplusTree::~BplusTree() {
    if(_opening) {
        close_file();
    }
}

int BplusTree::key_cmp(const KEY_T &key1, const KEY_T &key2) {
    // 比较两个关键字的大小
    // key1 < key2, return -1
    // key1 == key2, return 0
    // key1 > key2, return 1
    int n = strcmp(key1._key, key2._key);
    if(n < 0) {
        return -1;
    }
    else if(n > 0) {
        return 1;
    }
    else {
        return n;
    }
}

off_t BplusTree::search_leaf(const KEY_T &key) {
    // 查找包含关键字key的叶子节点的位置
    NODE_T node;
    int height = _meta.height;
    off_t res = 0;
    bool op = false;

    // 先读取跟节点
    if((read_file(&node, _meta.root_node)) != 1) {
        std::cout << "can't read bplusTree.cc " << __LINE__ << std::endl;
        exit(-1);
    }

    // 循环读取，直到最后一个非叶子节点
    while(height > 1) {
        op = false;
        for(int i = 0; i < node.n; ++i) {
            if(key_cmp(key, node.children[i].key) <= 0) {
                // key <= node.children[i].key
                read_file(&node, node.children[i].child);
                --height;
                op = true;
                break;
            }
        }
        //--height;
        if(!op) {
            // 不存在该关键字
            //std::cout << "can't find the key---bplusTree.cc" << __LINE__ << std::endl;
            res = 0;
            break;
        }
    }

    if(op) {
        // 此时，node的孩子节点便是叶子节点
        for(int i = 0; i < node.n; ++i) {
            if(key_cmp(key, node.children[i].key) <= 0) {
                res = node.children[i].child;
                break;
            }
        }
    }

    return res;
}

void BplusTree::search(const KEY_T &key, VALUE_T *value) {
    // 查找关键字为key的value, 不存在就返回nullptr
    LEAF_NODE_T leaf;

    open_file();
    
    // 查找包含key的叶子节点
    off_t leaf_off = search_leaf(key);
    if(leaf_off == 0) {
        value = nullptr;
        std::cout << "the key isn't exist--bplusTree.cc" << __LINE__ << std::endl;
    }
    else {
        read_file(&leaf, leaf_off);
        for(int i = 0; i < leaf.n; ++i) {
            // 查找资源
            int n = key_cmp(key, leaf.record[i].key);
            if(n == 0) {
                // 找到关键字
                *value = leaf.record[i].value; 
                break;
            }
            if(n == 1) {
                // 没有key的存在
                value = nullptr;
                break;
            }
        }
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
    //_meta.key_type = NONE;
    _meta.slot = BLOCK_OFF;

    // init tree root
    NODE_T root_node;
    root_node.parent = 0;
    _meta.root_node = allocate(root_node);
    _meta.height = 1;
    //root_node.flag = 1;                                         // 1表示其孩子节点是叶子节点

    //init leaf
    LEAF_NODE_T leaf_node;
    leaf_node.next = 0;
    leaf_node.prev = 0;
    leaf_node.parent = _meta.root_node;
    _meta.leaf_node = root_node.children[0].child = allocate(leaf_node);

    // 写入文件
    write_file(&_meta, META_OFF);
    write_file(&root_node, _meta.root_node);
    write_file(&leaf_node, root_node.children[0].child);
}