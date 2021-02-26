#include "Trie.h"



void Trie::addEndpoint(std::string word, std::function<void(int)> endPoint, int arg)
{
    TrieNode* pCur = &root;

    // Create a path through the trie that follows the given string.
    // At the end, the given function will be called with the provided argument.
    for (size_t i = 0; i < word.length(); i++)
    {
        char c = word.at(i);

        if (pCur->map.find(c) == pCur->map.end())
        {
            pCur->map.emplace(c, new TrieNode(nullptr, 0));
        }

        pCur = pCur->map.at(c);

        // We just placed the end node, add the function and arguments
        if (i == word.length() - 1)
        {
            pCur->arg  = arg;
            pCur->func = endPoint;
        }
    }
}

void Trie::process(const char c)
{
    // Take one step into the trie, and if the letter is not on any
    // known path, restart at the beginning.
    /// Note: Has known issues with overlapping keys and substring
    ///       search but for the known CLI commands, it is sufficient
    if (m_pCur->map.find(c) != m_pCur->map.end())
    {
        m_pCur = m_pCur->map.at(c);
        if (m_pCur->func != nullptr)
            m_pCur->func(m_pCur->arg);
    }
    else
    {
        m_pCur = &root;

        // Immediately search root for another starting chain
        // If two words "$$ASCII" and "ACK" are in the trie,
        // $$ACK would traverse as root->$->$->A->root, but because
        // another key starts with 'A', we want to immediately
        // find root->$->$->A->root->A so we can continue down the next path
        if (m_pCur->map.find(c) != m_pCur->map.end())
        {
            m_pCur = m_pCur->map.at(c);
            if (m_pCur->func != nullptr)
                m_pCur->func(m_pCur->arg);
        }
    }
}
