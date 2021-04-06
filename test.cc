#include <iostream>
#include "./db.h"
#include "./bplusTree.h"

int main()
{
    BplusTree bt("./yyj.txt");
    KEY_T key("tyf");
    VALUE_T value = 27;
    //VALUE_T value;
    bt.insert(key, value);
    //bt.search(key, &value);
    std::cout << value << "\n" ;
    //std::cout << sizeof(META_T) << "\n" << sizeof(NODE_T) << "\n" << sizeof(LEAF_NODE_T) << "\n";
    //bt.print(); 
}




