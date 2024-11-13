#pragma once
#include <unordered_map>

enum Status {
    MISSED, HIT
};

template <typename _NodeDT>
class ListNode {
public:
    ListNode() : next(nullptr), prev(nullptr) {}
    ListNode(_NodeDT _name) : name(_name), next(nullptr), prev(nullptr) {}

public:
    _NodeDT name;
    ListNode<_NodeDT>* next;
    ListNode<_NodeDT>* prev;
};

template <typename _NodeDT>
class LinkedList {
public:
    LinkedList() {
        head = new ListNode<_NodeDT>();
        tail = new ListNode<_NodeDT>();
        head->next = tail;
        tail->prev = head;
    }

    void addToHead(ListNode<_NodeDT>* node) {
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
        node->prev = head;
    }

    void deleteNode(ListNode<_NodeDT>* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
    }

    ListNode<_NodeDT>* lastNode() {
        return tail->prev;
    }

private:
    ListNode<_NodeDT>* head;
    ListNode<_NodeDT>* tail;
};

template <typename _Key, typename _Value>
class LRUCache {
public:
    LRUCache(int _maxSize) : maxSize(_maxSize), curSize(0) {}

    std::pair<_Value, Status> get(_Key key) {
        if (cache.find(key) == cache.end()) {
            return std::make_pair(defaultValue, MISSED);
        } else {
            // Move the accessed node to the head (most recently used)
            _Value value = cache[key];
            deleteKey(key);
            add(key, value);
            return std::make_pair(value, HIT);
        }
    }

    void add(_Key key, _Value value) {
        if (key_Node_Mapper.find(key) != key_Node_Mapper.end()) {
            // If key already exists, remove the old node
            deleteKey(key);
        } else if (curSize == maxSize) {
            // Cache is full, delete least recently used (LRU) node
            deleteLeastRecentlyUsedNode();
        }

        ListNode<_Key>* node = new ListNode<_Key>(key);
        linkedList.addToHead(node);
        key_Node_Mapper[key] = node;
        cache[key] = value;
        curSize++;
    }

private:
    void deleteKey(_Key key) {
        linkedList.deleteNode(key_Node_Mapper[key]);
        key_Node_Mapper.erase(key);
        cache.erase(key);
        curSize--;
    }

    void deleteLeastRecentlyUsedNode() {
        ListNode<_Key>* leastRecentlyUsedNode = linkedList.lastNode();
        deleteKey(leastRecentlyUsedNode->name);
    }

private:
    _Value defaultValue;
    int maxSize;
    int curSize;
    LinkedList<_Key> linkedList;
    std::unordered_map<_Key, ListNode<_Key>*> key_Node_Mapper;
    std::unordered_map<_Key, _Value> cache;
};
