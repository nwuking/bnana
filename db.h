#ifndef DB_H_
#define DB_H_

/*
 * 整个文件定义了bplusTree所需的结构信息
 */

#include <string.h>
#include <sys/types.h>

#include <string>


#define KEY_SIZE 10
#define BP_ORDER 10                                  // B+ 树中，一个节点最多拥有的关键字

/*
 * 定义key的类型，key的储存类型为char[]，大小为KEY_SIZE
 * KEY_SIZE的大小可以自定义，需要手动,然后重新编译
 */
typedef struct _key_t {
    //union _key {
    //    int int_k;
    //    char char_k[KEY_SIZE];
    //};

    char _key[KEY_SIZE];
    
    _key_t(const char *k = "") {
        bzero(_key, KEY_SIZE);
        bcopy(k, _key, sizeof(k));
    }
    //_key_t(const std::string &k) {
    //    _key_t(k.c_str());
    //}
    //_key_t(int k) {
    //    key.int_k = k;
    //}
} KEY_T;


// 定义value                     
typedef struct _value_t {
    //int len;
    char data[0];
} VALUE_T;
// VALUE_T的使用：
//      比如你想存储一个学生的基本信息，你可以：
//          struct student {
//              int id;
//              ....其它信息    
//          };
//      然后， memcpy(&value.data[0], &student, sizeof(student))


typedef struct _record_t {
    // 一条记录，由叶子节点使用，叶子节点包含了关键字和值
    KEY_T key;
    VALUE_T value;
} RECORD_T;


// 定义叶子节点
typedef struct _leaf_node_t {
    off_t parent;                   // 父节点的偏移量
    off_t next;                     // 下一个（右边）叶子节点的偏移量
    off_t prev;                     // 前一个（左边）叶子节点的偏移量
    size_t n;                       // record的数量
    RECORD_T record[BP_ORDER];
} LEAF_NODE_T;


typedef struct _index_t {
    // 
    KEY_T key;
    off_t child;
} INDEX_T;

// 定义非叶子节点
typedef struct _node_t {
    off_t parent;
    size_t n;                                   //  children的数量
    //int flag;                                   //  一个标志，1表示其孩子节点是叶节点
    INDEX_T children[BP_ORDER];
} NODE_T;


// 定义一些有关B+树的一些元信息
typedef struct _meta_t {
    size_t order;                               // B+树有几路
    //int key_type;                               // 关键字的type
    size_t node_n;                              // 非叶子节点的数量
    size_t leaf_node_n;                         // 叶子节点的数量
    size_t height;                              // 树的高度,不包括叶子节点
    off_t slot;                                 // new block to write
    off_t root_node;                            // 根节点在哪
    off_t leaf_node;                            // 最左边的一个叶子节点在哪
} META_T;

typedef struct _parent_t {
    // 用于读取叶子节点或者非叶子节点的parent字段
    off_t parent;
} PARENT_T;






#endif