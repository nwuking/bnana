#include "./bplusTree.h"

#include <string.h>

#include <algorithm>

/// META_T _meta;                                           // B+树的元信息
/// std::string _path;                                      // 储存文件的路径
/// FILE *_fp;
/// bool _opening;                                          // 记录是否以打开文件


BplusTree::BplusTree(const std::string &path, bool empty)
    : _path(path), _fp(nullptr), _opening(false)
{
    /* 打开一个文件，获取其元信息
     * 如果文件不存在或者文件为空，先为其添加一些基本的元信息
     */

    //_path = path;

    if(!empty) {
        // 默认所打开的文件不为空，读取文件的meta
        open_file();
        if(read_file(&_meta, META_OFF) != 1) {
            // 读取失败，文件为空
            empty = true;
            //close_file();
        }
        close_file();
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

off_t BplusTree::search_none_leaf(const KEY_T &key) {
    // 查找包含key的最后一个非叶子节点所在的位置
    int height = _meta.height;
    off_t res = _meta.root_node;

    while(height > 1) {
        NODE_T node;
        read_file(&node, res);
        bool op = true;

        for(int i = 0; i < node.n; ++i) {
            if(key_cmp(key, node.children[i].key) <= 0) {
                op = false;
                res = node.children[i].child;
                break;
            }
        }
        if(op) {
            res = node.children[node.n-1].child;
        }
        --height;
    }
    return res;
}

off_t BplusTree::search_leaf(const KEY_T &key, off_t offset) {
    NODE_T node;
    read_file(&node, offset);
    off_t res;

    bool op = true;

    for(int i = 0; i < node.n; ++i) {
        if(key_cmp(key, node.children[i].key) <= 0) {
            op = false;
            res = node.children[i].child;
            break;
        }
    }

    if(op) {
        res = node.children[node.n-1].child;
    }
    return res;
}

off_t BplusTree::search_leaf(const KEY_T &key) {
    off_t res = search_leaf(key, search_none_leaf(key));
    return res;
}

void BplusTree::search(const KEY_T &key, VALUE_T *value) {
    // 查找关键字为key的value, 不存在就返回nullptr
    LEAF_NODE_T leaf;

    open_file();
    
    // 查找包含key的叶子节点
    off_t leaf_off = search_leaf(key);
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

    close_file();
}

/*
template<typename T>
void BplusTree::create_node(T *node, T *next, off_t offset) {
    // 创建一个新节点
    // 将node 的一些信息复制到next

    next->parent = node->parent;
    next->next = node->next;
    next->prev = offset;
    node->next = allocate(*next);

    if(next->next != 0) {
        T old_next;
        read_file(&old_next, next->next, SIZE_NO_CHILDREN);
        old_next.prev = node->next;
        write_file(old_next, next->next, SIZE_NO_CHILDREN);
    }
    write_file(&_meta, META_OFF);
}
*/

void BplusTree::insert_record_to_leaf(LEAF_NODE_T *node, const KEY_T &key, const VALUE_T &value) {
    // 叶子节点的关键字没满

    size_t new_record = node->n;
    for(size_t i = 0; i < node->n; ++i) {
        if(key_cmp(node->record[i].key, key) > 0) {
            new_record = i;
            break;
        }
    }

    size_t op = node->n-1;
    while(op >= new_record) {
        node->record[op+1] = node->record[op];
        --op;
    }

    node->record[new_record].key = key;
    node->record[new_record].value = value;
    node->n++;
}

void BplusTree::insert_first_key_value(const KEY_T &key, const VALUE_T &value) {
    // 插入第一个（key/value)
    NODE_T root;
    read_file(&root, _meta.root_node);

    root.n = 1;
    root.children[0].key = key;
    
    LEAF_NODE_T leaf;
    read_file(&leaf, root.children[0].child);
    leaf.n = 1;
    leaf.record[0].key = key;
    leaf.record[0].value = value;

    // save
    write_file(&root, _meta.root_node);
    write_file(&leaf, _meta.leaf_node);

    _meta.height = 1;
    write_file(&_meta, META_OFF);
}

void BplusTree::create_leaf_node(LEAF_NODE_T *old_leaf, LEAF_NODE_T *new_leaf, off_t offset) {
    new_leaf->parent = old_leaf->parent;
    new_leaf->next = offset;
    new_leaf->prev = old_leaf->prev;

    off_t new_offset = allocate(*new_leaf);
    _meta.leaf_node_n++;

    if(offset == _meta.leaf_node) {
        _meta.leaf_node = new_offset;
        write_file(&_meta, META_OFF);
    }
    else {
        LEAF_NODE_T node;
        read_file(&node, new_leaf->prev);
        node.next = new_offset;
        write_file(&node, new_leaf->prev);
    }
    old_leaf->prev = new_offset;
}

void BplusTree::insert_key_to_node(NODE_T *node, const KEY_T &key, off_t offset) {
    // node非叶子节点， 没满，直接插入
    size_t insert_point = node->n;
    size_t n = node->n;

    while(--n >= 0) {
        if(key_cmp(key, node->children[n].key) < 0) {
            node->children[insert_point] = node->children[n];
            --insert_point;
        }
        else
        {
            break;
        }
    }

    node->n++;
    node->children[insert_point].key = key;
    node->children[insert_point].child = offset;
}

off_t BplusTree::create_node(NODE_T *old_node, NODE_T *new_node) {
    // 创建一个新的非叶子节点
    new_node->parent = old_node->parent;
    new_node->n = 0;
    bzero(new_node->children, sizeof(new_node->children));
    return allocate(*new_node);
}

void BplusTree::update_children(NODE_T &node, off_t offset) {
    // 更新node节点的孩子节点指向的父节点offset
    for(int i = 0; i < node.n; ++i) {
        PARENT_T parent;
        read_file(&parent, node.children[i].child);
        parent.parent = offset;
        write_file(&parent, node.children[i].child);
    }
}

off_t BplusTree::create_new_root_node(NODE_T &node) {
    off_t offset = allocate(node);
    _meta.root_node = offset;
    _meta.height++;
    node.n = 2;
    node.parent = 0;
    write_file(&_meta, META_OFF);
    return offset;
}

void BplusTree::update_parent(const KEY_T &key, off_t parent_offset, off_t offset) {
    // 
    NODE_T parent_node;
    read_file(&parent_node, parent_offset);


    if(parent_node.n != _meta.order) {
        insert_key_to_node(&parent_node, key, offset);
        write_file(&parent_node, parent_offset);
        return;
    }
    else {
        // 递归更新
        NODE_T new_node;
        off_t new_off;
        new_off = create_node(&parent_node, &new_node);

        // 分裂非叶子节点
        size_t point = parent_node.n / 2;
        bool place_right = key_cmp(key, parent_node.children[point].key) <= 0;
        if(place_right)
            --point;

        std::copy(parent_node.children, parent_node.children+point+1, new_node.children);
        std::copy(parent_node.children+point+1, parent_node.children+parent_node.n, parent_node.children);
        parent_node.n = parent_node.n - point - 1;
        new_node.n = point + 1;

        if(place_right) {
            insert_key_to_node(&new_node, key, offset);
        }
        else {
            insert_key_to_node(&parent_node, key, offset);
        }

        // save
        write_file(&new_node, new_off);
        write_file(&parent_node, parent_offset);

        update_children(new_node, new_off);

        if(parent_offset == _meta.root_node) {
            NODE_T new_root_node;
            off_t offset = create_new_root_node(new_root_node);
            new_node.parent = offset;
            parent_node.parent = offset;
            new_root_node.children[0].key = new_node.children[new_node.n-1].key;
            new_root_node.children[0].child = new_off;
            new_root_node.children[1].key = parent_node.children[parent_node.n-1].key;
            new_root_node.children[1].child = parent_offset;
            return;
        }
        else
            // 递归更新上一个父节点
            update_parent(new_node.children[new_node.n-1].key, new_node.parent, new_off);
    }
}

void BplusTree::update_parent_no_split(const KEY_T &key, off_t parent) {
    // 当插入的record中的key在插入节点中最大时
    // 更新相应父节点的key
    NODE_T node;
    read_file(&node, parent);
    node.children[node.n-1].key = key;
    write_file(&node, parent);
    if(parent == _meta.root_node)
        return;
    update_parent_no_split(key, node.parent);
}

bool BplusTree::insert(const KEY_T &key, VALUE_T value) {
    // 插入一对(key/value)
    open_file();

    std::cout << "start insert\n";

    if(_meta.height == 0) {
        // 此时为空
        insert_first_key_value(key, value);
        close_file();
        return true;
    }

    off_t parent = search_none_leaf(key);                                   // 
    off_t offset = search_leaf(key, parent);

    LEAF_NODE_T leaf_node;
    read_file(&leaf_node, offset);

    // 查找是否以存在相同的关键字
    int op = -1;
    for(int i = 0; i < leaf_node.n; ++i) {
        if(key_cmp(key, leaf_node.record[i].key) == 0) {
            op = 0;
            break;
        }
        /*
        else if(key_cmp(key, leaf_node.record[i].key) > 0) {
            break;
        }
        */
    }
    if(op == 0) {
        // 以存在关键字，插入失败
        return false;
    }

    if(leaf_node.n == _meta.order) {
        // 该节点中的关键字已满
        // 需要对其进行分裂

        LEAF_NODE_T new_leaf_node;
        //create_node(&leaf_node, &new_leaf_node, offset);
        create_leaf_node(&leaf_node, &new_leaf_node, offset);

        size_t point = leaf_node.n / 2;
        bool place_right = key_cmp(key, leaf_node.record[point].key) < 0;
        if(place_right) 
            --point;
        
        //std::copy(leaf_node.record+point, leaf_node.record+leaf_node.n, new_leaf_node.record);
        std::copy(leaf_node.record, leaf_node.record+point+1, new_leaf_node.record);
        std::copy(leaf_node.record+point+1, leaf_node.record+leaf_node.n, leaf_node.record);
        //new_leaf_node.n = leaf_node.n - point;
        //leaf_node.n = point;
        new_leaf_node.n = point + 1;
        leaf_node.n = leaf_node.n - point-1;

        if(place_right) {
            insert_record_to_leaf(&new_leaf_node, key, value);
        }
        else {
            insert_record_to_leaf(&leaf_node, key, value);
        }

        // save leaf
        write_file(&leaf_node, offset);
        write_file(&new_leaf_node, leaf_node.prev);

        // 更新父节点
        update_parent(new_leaf_node.record[new_leaf_node.n-1].key, new_leaf_node.parent, leaf_node.prev);

        if(key_cmp(key, leaf_node.record[leaf_node.n-1].key) == 0) {
            update_parent_no_split(key, parent);
        }
    }
    else {
        // 关键字没满，直接插入
        int insert_point = leaf_node.n;
        for(int i = 0; i < leaf_node.n; ++i) {
            if(key_cmp(key, leaf_node.record[i].key) < 0) {
                insert_point = i;
                break;
            }
        }
        for(int i = leaf_node.n-1; i >= insert_point; --i) {
            leaf_node.record[i+1].key = leaf_node.record[i].key;
            leaf_node.record[i+1].value = leaf_node.record[i].value;
        }
        leaf_node.record[insert_point].key = key;
        leaf_node.record[insert_point].value = value;
        leaf_node.n++;

        if(insert_point == leaf_node.n-1) {
            //update_parent(key, parent, offset);
            update_parent_no_split(key, parent);
        }

        // save
        write_file(&leaf_node, offset);
    }

    close_file();
    return true;
}


off_t BplusTree::allocate(size_t size) {
    off_t slot = _meta.slot;
    _meta.slot += size;
    return slot;
}

off_t BplusTree::allocate(NODE_T &node) {
    // 给一个NODE_T指定储存位置
    node.n = 0;
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
    bzero(&_meta, sizeof(META_T));
    
    // 初始化元信息
    _meta.order = BP_ORDER;
    //_meta.key_type = NONE;
    _meta.slot = BLOCK_OFF;
    _meta.node_n = 0;
    _meta.leaf_node_n = 0;

    // init tree root
    NODE_T root_node;
    root_node.parent = 0;
    _meta.root_node = allocate(root_node);
    _meta.height = 0;
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

    //fflush(_fp);
}