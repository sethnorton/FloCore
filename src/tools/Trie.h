#pragma once
#include <string>
#include <functional>
#include <unordered_map>

class Trie
{
public:
    Trie() : m_pCur(&root) {}
    ~Trie() {}

    /*------------            Primary Interface             ------------*/
    void addEndpoint(std::string              word,
                     std::function<void(int)> endPoint,
                     int                      arg);
    void process(const char c);

protected:
    /*------------            Internal Data Definitions            ------------*/
    struct TrieNode {
        std::function<void(int arg)> func;
        int                                      arg;
        std::unordered_map<char, TrieNode*>      map;

        TrieNode() : func(nullptr), arg(0) { map.clear(); }
        TrieNode(std::function<void(int)> f, int a) : func(f), arg(a) {}
        ~TrieNode()
        {
            for (auto tNode : map)
            {
                if (tNode.second != nullptr) /// Should never be null but just in case
                    delete tNode.second;     /// Could potentially stack overflow
            }
        }
        TrieNode(const TrieNode& t)            = delete; // Disallow copy constructor
        TrieNode& operator=(const TrieNode& t) = delete; // Disallow copy assignment
    };

    /*------------            Traversing State             ------------*/
    TrieNode  root;
    TrieNode* m_pCur;
};
