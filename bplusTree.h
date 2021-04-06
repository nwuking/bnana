#ifndef BPLUSTREE_H_
#define BPLUSTREE_H_

#include "./db.h"

#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#define META_OFF 0
#define BLOCK_OFF META_OFF + sizeof(META_T)
#define SIZE_NO_CHILDREN sizeof(LEAF_NODE_T)-BP_ORDER*sizeof(RECORD_T)

class BplusTree
{
public:
    BplusTree(const std::string &path, bool empty = false);
    ~BplusTree();

    //void print() {
    //    std::cout << "height:" << _meta.height << "\n";
    //    std::cout << "node_n:" << _meta.node_n << "\n";
    //    std::cout << "order:" << _meta.order << "\n";
    //    std::cout << "leaf_node_n:" << _meta.leaf_node_n << std::endl;  
    //}

    void search(const KEY_T &key, VALUE_T *value);              // 查找
    bool insert(const KEY_T &key, const VALUE_T value);         // 插入
    void pop(KEY_T &key);                                       // 弹出
    bool update(const KEY_T &key, const VALUE_T &value);        // 更新

private:
    META_T _meta;                                           // B+树的元信息
    std::string _path;                                      // 储存文件的路径
    FILE *_fp;
    bool _opening;                                          // 记录是否以打开文件

    //enum KEY_TYPE {
    //    NONE,
    //    INT,
    //    STR
    //};

    void update_parent_no_split(const KEY_T &key, off_t parent, KEY_T &parent_key);

    off_t create_new_root_node(NODE_T &node);

    void update_children(NODE_T &node, off_t offset);

    off_t create_node(NODE_T *old_node, NODE_T *new_node);

    void update_parent(const KEY_T &key, off_t parent_offset, off_t offset);

    void create_leaf_node(LEAF_NODE_T *old_leaf, LEAF_NODE_T *new_leaf, off_t offset);

    void insert_first_key_value(const KEY_T &key, const VALUE_T &value);

    void insert_key_to_node(NODE_T *node, const KEY_T &key, off_t offset);

    void insert_record_to_leaf(LEAF_NODE_T *node, const KEY_T &key, const VALUE_T &value);

    int key_cmp(const KEY_T &key1, const KEY_T &key2);      // 比较两个关键字的大小

    off_t search_none_leaf(const KEY_T &key);               // 查找包含key的最后一个非叶子节点所在的位置

    off_t search_leaf(const KEY_T &key, off_t offset);

    off_t search_leaf(const KEY_T &key);                    // 查找包含key的叶子节点的位置

    // allocate(...), 给叶子节点、非叶子节点在文件中分配一个位置
    off_t allocate(size_t size);

    off_t allocate(NODE_T &node);

    off_t allocate(LEAF_NODE_T &node);

    void init_empty_file();                                 // 初始化一个空文件

    void close_file() {
        fflush(_fp);
        if(_opening) {
            fclose(_fp);
            _opening = false;
        }
    }

    void open_file(const char *modes = "rb+") {
        if(!_opening) {
            _fp = fopen(_path.c_str(), modes);
            if(_fp) {
               _opening = true; 
            }
        }
    }

    int read_file(void *block, off_t offset, size_t size) {
        //open_file();
        fseek(_fp, offset, SEEK_SET);
        int n = fread(block, size, 1, _fp);
        return n;
    }

    template<typename T>
    int read_file(T *block, off_t offset) {
        return read_file(block, offset, sizeof(T));
    }

    int write_file(void *block, off_t offset, size_t size) {
        fseek(_fp, offset, SEEK_SET);
        int n = fwrite(block, size, 1, _fp);
        fflush(_fp);
        return n;
    }

    template<typename T>
    int write_file(T *block, off_t offset) {
        return write_file(block, offset, sizeof(T));
    }
};

#endif