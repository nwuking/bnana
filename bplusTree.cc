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
    bzero(&_meta, sizeof(META_T));

    if(!empty) {
        // 默认所打开的文件不为空，读取文件的meta
        open_file();
        if(!_opening) {
            // 打开文件失败， 文件可能不存在
            empty = true;
        }
        else {
            read_file(&_meta, META_OFF);
            close_file();
        }
    }
    if(empty) {
        // 文件为空， 初始化一个文件
        open_file("w+");

        // 初始化一个空文件，即初始化一些元信息
        init_empty_file();
        close_file();
    }
}

BplusTree::~BplusTree() {
    if(_opening) {
        close_file();
    }
    else {
        fflush(_fp);
    }
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
        std::cout << "search:" << leaf.record[i].key._key << ":" << leaf.record[i].value << "\n";
        int n = key_cmp(leaf.record[i].key, key);
        std::cout << "n:" << n << "\n";
        if(n == 0) {
            // 找到关键字
            memcpy(value, &leaf.record[i].value, sizeof(VALUE_T));
            //*value = leaf.record[i].value; 
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

bool BplusTree::insert(const KEY_T &key, VALUE_T value) {
    // 插入一对(key/value)
    open_file();
    if(_meta.height == 0) {
        // 此时为空
        std::cout << "first insert!" << std::endl;
        insert_first_key_value(key, value);
        _meta.height = 1;
        write_file(&_meta, META_OFF);

        close_file();
        return true;
    }

    off_t parent = search_none_leaf(key);                                   // 
    off_t offset = search_leaf(key, parent);

    LEAF_NODE_T leaf_node;
    read_file(&leaf_node, offset);

    // 查找是否以存在相同的关键字
    for(int j = 0; j < leaf_node.n; ++j) {
        if(key_cmp(key, leaf_node.record[j].key) == 0) {
            // 存在相同的关键字， return false
            std::cout << "have the same key!" << std::endl;
            return false;
        }
    }

    if(leaf_node.n == _meta.order) {
        // 该节点中的关键字已满
        // 需要对其进行分裂
        std::cout << "full" << std::endl;
        LEAF_NODE_T new_leaf_node;
        create_leaf_node(&leaf_node, &new_leaf_node, offset);

        int point = leaf_node.n / 2;
        // 新节点的关键字数量为point+1;
        bool place_right = (key_cmp(key, leaf_node.record[point].key) < 0);
        if(place_right) 
            --point;
        
        // 将(point+1)个最小的关键字移动到新的叶子节点
        for(int i = 0; i < point+1; ++i) {
            //memcpy(&new_leaf_node.record[i], &leaf_node.record[i], sizeof(RECORD_T));
            new_leaf_node.record[i].key = leaf_node.record[i].key;
            new_leaf_node.record[i].value = leaf_node.record[i].value;
            bzero(&leaf_node.record[i], sizeof(RECORD_T));
            ++new_leaf_node.n;
        }
        //std::copy(leaf_node.record, &leaf_node.record[point+1], new_leaf_node.record);
        // 将老节点中剩余的关键字向前移动
        for(int i = point+1; i < leaf_node.n; ++i) {
            //memcpy(&leaf_node.record[i-point-1], &leaf_node.record[i], sizeof(RECORD_T));
            leaf_node.record[i-point-1].key = leaf_node.record[i].key;
            leaf_node.record[i-point-1].value = leaf_node.record[i].value;
            bzero(&leaf_node.record[i], sizeof(RECORD_T));
        }
        //std::copy(leaf_node.record+point+1, leaf_node.record+leaf_node.n, leaf_node.record);
        //new_leaf_node.n = point + 1;
        leaf_node.n = leaf_node.n - point-1;

        if(place_right) {
            insert_record_to_leaf(&new_leaf_node, key, value);
        }
        else {
            std::cout << " klkl" << std::endl;
            insert_record_to_leaf(&leaf_node, key, value);
        }

        // save leaf
        write_file(&leaf_node, offset);
        write_file(&new_leaf_node, leaf_node.prev);

        // 更新父节点
        update_parent(new_leaf_node.record[new_leaf_node.n-1].key, new_leaf_node.parent, leaf_node.prev);

        if(key_cmp(key, leaf_node.record[leaf_node.n-1].key) == 0) {
            update_parent_no_split(key, parent, leaf_node.record[leaf_node.n-1].key);
        }
    }
    else {
        std::cout << "empty!" << std::endl;
        // 关键字没满，直接插入
        int insert_point = leaf_node.n;
        for(int i = 0; i < leaf_node.n; ++i) {
            if(key_cmp(key, leaf_node.record[i].key) < 0) {
                insert_point = i;
                break;
            }
        }
        for(int i = leaf_node.n-1; i >= insert_point; --i) {
            //bzero(&leaf_node.record[i+1].key, sizeof(KEY_T));
            //memcpy(&leaf_node.record[i+1].key, &leaf_node.record[i].key, sizeof(KEY_T));
            leaf_node.record[i+1].key = leaf_node.record[i].key;
            //bzero(&leaf_node.record[i+1].value, sizeof(VALUE_T));
            //memcpy(&leaf_node.record[i+1].value, &leaf_node.record[i].value, sizeof(VALUE_T));
            leaf_node.record[i+1].value = leaf_node.record[i].value;
        }
        //bzero(&leaf_node.record[insert_point].key, sizeof(KEY_T));
        //memcpy(&leaf_node.record[insert_point].key, &key, sizeof(KEY_T));
        leaf_node.record[insert_point].key = key;
        //bzero(&leaf_node.record[insert_point].value, sizeof(VALUE_T));
        //memcpy(&leaf_node.record[insert_point].value, &value, sizeof(VALUE_T));
        leaf_node.record[insert_point].value = value;
        ++leaf_node.n;

         // save
        write_file(&leaf_node, offset);

        if(insert_point == leaf_node.n-1) {
            update_parent_no_split(key, parent, leaf_node.record[insert_point-1].key);
        }
    }

    close_file();
    return true;
}

void BplusTree::pop(KEY_T &key) {
    // 如果key存在，删除(key, value)

    open_file();
    off_t parent_offset = search_none_leaf(key);
    off_t offset = search_leaf(key, parent_offset);
    LEAF_NODE_T leaf_node;
    bool op = true;
    read_file(&leaf_node, offset);
    for(int i = 0; i < leaf_node.n; ++i) {
        if(key_cmp(key, leaf_node.record[i].key) == 0) {
            bzero(&leaf_node.record[i], sizeof(RECORD_T));
            for(int j = i+1; j < leaf_node.n; ++j) {
                op = false;
                leaf_node.record[j-1].key = leaf_node.record[j].key;
                leaf_node.record[j-1].value = leaf_node.record[j].value;
                bzero(&leaf_node.record[j], sizeof(RECORD_T));
            }
            --leaf_node.n;
            write_file(&leaf_node, offset);
            break;
        }
        else if(key_cmp(leaf_node.record[i].key, key) > 0)
            break;
    }

    if(op) {
        // 删除的key是当前最大的，需要更新父节点
        update_parent_no_split(leaf_node.record[leaf_node.n-1].key, parent_offset, key);
    }

    close_file();
}

bool BplusTree::update(const KEY_T &key, const VALUE_T &value) {
    // 如果key存在， 就更新key对应的值
    // 如果不存在， 就插入(key, value)

    off_t offset = search_leaf(key);
    LEAF_NODE_T leaf_node;

    open_file();
    read_file(&leaf_node, offset);

    int low = 0, height = leaf_node.n-1;
    while(low <= height) {
        int mid = (low+height) / 2;
        int n = key_cmp(key, leaf_node.record[mid].key);
        if(n == 0) {
            leaf_node.record[mid].value = value;
            write_file(&leaf_node, offset);
            close_file();
            return true;
        }
        else if(n > 0) {
            low = mid+1;
        }
        else {
            height = mid-1;
        }
    }

    // 不存在(key, value)
    insert(key, value);
    return false;
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


off_t BplusTree::create_new_root_node(NODE_T &node) {
    bzero(&node, sizeof(NODE_T));
    off_t offset = allocate(node);
    _meta.root_node = offset;
    _meta.height++;
    node.n = 2;
    node.parent = 0;
    write_file(&_meta, META_OFF);
    return offset;
}

off_t BplusTree::create_node(NODE_T *old_node, NODE_T *new_node) {
    // 创建一个新的非叶子节点
    new_node->parent = old_node->parent;
    new_node->n = 0;
    //bzero(new_node->children, sizeof(new_node->children));
    return allocate(*new_node);
}

void BplusTree::create_leaf_node(LEAF_NODE_T *old_leaf, LEAF_NODE_T *new_leaf, off_t offset) {
    bzero(new_leaf, sizeof(LEAF_NODE_T));
    new_leaf->parent = old_leaf->parent;
    new_leaf->next = offset;
    new_leaf->prev = old_leaf->prev;

    off_t new_offset = allocate(*new_leaf);
    //_meta.leaf_node_n++;

    if(offset == _meta.leaf_node) {
        _meta.leaf_node = new_offset;
        //write_file(&_meta, META_OFF);
    }
    else {
        LEAF_NODE_T node;
        read_file(&node, new_leaf->prev);
        node.next = new_offset;
        write_file(&node, new_leaf->prev);
    }
    old_leaf->prev = new_offset;
    write_file(&_meta, META_OFF);
}

void BplusTree::init_empty_file() {
    // 初始化文件的元信息，写入文件开始处
    //bzero(&_meta, sizeof(META_T));
    
    // 初始化元信息
    _meta.order = BP_ORDER;
    _meta.slot = BLOCK_OFF;
    _meta.node_n = 0;
    _meta.leaf_node_n = 0;

    // init tree root
    NODE_T root_node;
    bzero(&root_node, sizeof(NODE_T));
    root_node.parent = 0;
    _meta.root_node = allocate(root_node);
    _meta.height = 0;

    //init leaf
    LEAF_NODE_T leaf_node;
    bzero(&leaf_node, sizeof(LEAF_NODE_T));
    leaf_node.next = 0;
    leaf_node.prev = 0;
    leaf_node.parent = _meta.root_node;
    _meta.leaf_node = root_node.children[0].child = allocate(leaf_node);

    // 写入文件
    write_file(&_meta, META_OFF);
    write_file(&root_node, _meta.root_node);
    write_file(&leaf_node, root_node.children[0].child);

}

void BplusTree::insert_key_to_node(NODE_T *node, const KEY_T &key, off_t offset) {
    // node非叶子节点， 没满，直接插入
    int insert_point = node->n;
    int n = node->n;

    while(--n >= 0) {
        if(key_cmp(key, node->children[n].key) < 0) {
            //memcpy(&node->children[insert_point].key, &node->children[n].key, sizeof(KEY_T));
            node->children[insert_point].key = node->children[n].key;
            node->children[insert_point].child = node->children[n].child;
             bzero(&node->children[n], sizeof(RECORD_T));
            --insert_point;
        }
        else
        {
            break;
        }
    }

    node->n++;
    //memcpy(&node->children[insert_point].key, &key, sizeof(KEY_T));
    node->children[insert_point].key = key;
    node->children[insert_point].child = offset;
}

void BplusTree::insert_record_to_leaf(LEAF_NODE_T *node, const KEY_T &key, const VALUE_T &value) {
    // 叶子节点的关键字没满

    int new_record = node->n;
    for(int i = 0; i < node->n; ++i) {
        if(key_cmp(node->record[i].key, key) > 0) {
            new_record = i;
            break;
        }
    }

    int op = node->n-1;
    while(op >= new_record) {
        //memcpy(&node->record[op+1].key, &node->record[op].key, sizeof(KEY_T));
        node->record[op+1].key = node->record[op].key;
        bzero(&node->record[op].key, sizeof(KEY_T));
        //memcpy(&node->record[op+1].value, &node->record[op].value, sizeof(VALUE_T));
        node->record[op+1].value = node->record[op].value;
        bzero(&node->record[op].value, sizeof(VALUE_T));
        --op;
    }

    //memcpy(&node->record[new_record].key, &key, sizeof(KEY_T));
    node->record[new_record].key = key;
    //memcpy(&node->record[new_record].value, &value, sizeof(VALUE_T));
    node->record[new_record].value = value;
    node->n++;
}

void BplusTree::insert_first_key_value(const KEY_T &key, const VALUE_T &value) {
    // 插入第一个（key/value)
    NODE_T root;
    read_file(&root, _meta.root_node);

    root.n = 1;
    //memcpy(&root.children[0].key, &key, sizeof(KEY_T));
    root.children[0].key = key;
    
    LEAF_NODE_T leaf;
    read_file(&leaf, root.children[0].child);
    leaf.n = 1;
    //memcpy(&leaf.record[0].key, &key, sizeof(KEY_T));
    leaf.record[0].key = key;
    //memcpy(&leaf.record[0].value, &value, sizeof(VALUE_T));
    leaf.record[0].value = value;

    // save
    write_file(&root, _meta.root_node);
    write_file(&leaf, _meta.leaf_node);
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
    return n;
}

off_t BplusTree::search_none_leaf(const KEY_T &key) {
    // 查找包含key的最后一个非叶子节点所在的位置
    int height = _meta.height;
    off_t res = _meta.root_node;

    while(height > 1) {
        NODE_T node;
        read_file(&node, res);
        off_t search_point = node.children[node.n-1].child;
        for(int i = 0; i < node.n; ++i) {
            if(key_cmp(key, node.children[i].key) <= 0) {
                search_point = node.children[i].child;
                break;
            }
        }
        res = search_point;
        --height;
    }
    return res;
}

off_t BplusTree::search_leaf(const KEY_T &key, off_t offset) {
    NODE_T node;
    read_file(&node, offset);
    off_t res = node.children[node.n-1].child;

    for(int i = 0; i < node.n; ++i) {
        if(key_cmp(key, node.children[i].key) <= 0) {
            res = node.children[i].child;
            break;
        }
    }
    return res;
}

off_t BplusTree::search_leaf(const KEY_T &key) {
    off_t res = search_leaf(key, search_none_leaf(key));
    return res;
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
        // 新节点有(point+1)个关键字
        int point = parent_node.n / 2;
        bool place_right = (key_cmp(key, parent_node.children[point].key)) <= 0;
        if(place_right)
            --point;

        for(int i = 0; i < (point+1); ++i) {
            //memcpy(&new_node.children[i].key, &parent_node.children[i].key, sizeof(KEY_T));
            new_node.children[i].key = parent_node.children[i].key;
            new_node.children[i].child = parent_node.children[i].child;
            bzero(&parent_node.children[i], sizeof(INDEX_T));
            ++new_node.n;
        }
        //std::copy(parent_node.children, parent_node.children+point+1, new_node.children);
        for(int i = point+1; i < parent_node.n; ++i) {
            //memcpy(&parent_node.children[i-point-1].key, &parent_node.children[i].key, sizeof(KEY_T));
            parent_node.children[i-point-1].key = parent_node.children[i].key;
            parent_node.children[i-point-1].child = parent_node.children[i].child;
            bzero(&parent_node.children[i], sizeof(INDEX_T));
        }
        //std::copy(parent_node.children+point+1, parent_node.children+parent_node.n, parent_node.children);
        parent_node.n = parent_node.n - point - 1;
        //new_node.n = point + 1;

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
            //memcpy(&new_root_node.children[0].key, &new_node.children[new_node.n-1].key, sizeof(KEY_T));
            new_root_node.children[0].key = new_node.children[new_node.n-1].key;
            new_root_node.children[0].child = new_off;
            //memcpy(&new_root_node.children[1].key, &parent_node.children[parent_node.n-1].key, sizeof(KEY_T));
            new_root_node.children[1].key = parent_node.children[parent_node.n-1].key;
            new_root_node.children[1].child = parent_offset;
            write_file(&new_node, new_off);
            write_file(&parent_node, parent_offset);
            write_file(&new_root_node, offset);
            return;
        }
        else
            // 递归更新上一个父节点
            update_parent(new_node.children[new_node.n-1].key, new_node.parent, new_off);
    }
}

void BplusTree::update_parent_no_split(const KEY_T &key, off_t parent, KEY_T &parent_key) {
    // 当插入的record中的key在插入节点中最大时
    // 更新相应父节点的key
    NODE_T node;
    read_file(&node, parent);
    int cmp_point;
    KEY_T old_key;
    for(int i = 0; i < node.n; ++i) {
        if(key_cmp(parent_key, node.children[i].key) == 0) {
            cmp_point = i;
            //memcpy(&old_key, &node.children[i].key, sizeof(KEY_T));
            old_key = node.children[i].key;
            //memcpy(&node.children[i].key, &key, sizeof(KEY_T));
            node.children[i].key = key;
            break;
        }
    }
    write_file(&node, parent);
    if(parent == _meta.root_node)
        return;
    if(cmp_point == node.n-1) 
        // 递归更新
        update_parent_no_split(key, node.parent, old_key);
}
