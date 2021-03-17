#ifndef BPLUSTREE_H_
#define BPLUSTREE_H_

#include "./db.h"

#include <string>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#define META_OFF 0
#define BLOCK_OFF META_OFF+sizeof(META_T)

class BplusTree
{
public:
    BplusTree(std::string &path, bool empty = false);
    ~BplusTree();

    void search(const KEY_T &key, VALUE_T *value) const;
    void insert(const KEY_T &key, const VALUE_T &value);
    void pop(const KEY_T &key);
    void update(const KEY_T &key, const VALUE_T &value);

    void open(std::string &path, const char *modes = "w+");
private:
    META_T _meta;                                           // B+树的元信息
    std::string _path;                                      // 储存文件的路径
    FILE *_fp;
    bool _opening;                                          // 记录是否以打开文件

    enum KEY_TYPE {
        NONE,
        INT,
        STR
    };

    off_t allocate(size_t size);

    off_t allocate(NODE_T &node);

    off_t allocate(LEAF_NODE_T &node);

    void init_empty_file();

    void close_file() {
        if(_opening) {
            fclose(_fp);
            _opening = true;
        }
    }

    void open_file(const char *modes = "w+") {
        if(!_opening) {
            _fp = fopen(_path.c_str(), modes);
            if(!_fp) {
                std::cout << "can't open file" << std::endl;
                exit(-1);
            }
            _opening = true;
        }
    }

    int read_file(void *block, off_t offset, size_t size) {
        open_file();
        fseek(_fp, offset, SEEK_SET);
        int n = fread(block, size, 1, _fp);
        return n;
    }

    template<typename T>
    int read_file(T *block, off_t offset) {
        return read_file(block, offset, sizeof(T));
    }

    template<typename T>
    int write_file(const T &block, off_t offset);
};

#endif