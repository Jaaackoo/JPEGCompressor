#include "Huffman.hpp"
#include <queue>
#include <iostream>

void Huffman::build(const vector<Symbol> &symbols)
{
    map<Symbol, int> freq;
    for (auto s : symbols)
        freq[s]++;

    priority_queue<Node *, vector<Node *>, NodeCmp> pq;
    for (auto &p : freq)
    {
        pq.push(new Node(p.first, p.second));
    }

    while (pq.size() > 1)
    {
        Node *a = pq.top();
        pq.pop();
        Node *b = pq.top();
        pq.pop();
        pq.push(new Node(-1, a->freq + b->freq, a, b));
    }
    if (!pq.empty())
        root = pq.top();

    vector<bool> prefix;
    generateCodes(root, prefix);
}

void Huffman::generateCodes(Node *node, vector<bool> &prefix)
{
    if (!node)
        return;
    if (node->symbol >= 0)
    {
        codes[node->symbol] = prefix;
    }
    else
    {
        prefix.push_back(false);
        generateCodes(node->left, prefix);
        prefix.back() = true;
        generateCodes(node->right, prefix);
        prefix.pop_back();
    }
}

void Huffman::encode(const vector<Symbol> &symbols)
{
    bitstream.clear();
    bitBuf = 0;
    bitCount = 0;
    for (auto s : symbols)
    {
        writeBits(codes.at(s));
    }
    flushBits();
}

void Huffman::writeBit(bool b)
{
    bitBuf = (bitBuf << 1) | (b ? 1 : 0);
    if (++bitCount == 8)
    {
        bitstream.push_back(bitBuf);
        bitBuf = 0;
        bitCount = 0;
    }
}

void Huffman::writeBits(const vector<bool> &bits)
{
    for (bool b : bits)
        writeBit(b);
}

void Huffman::flushBits()
{
    if (bitCount > 0)
    {
        bitBuf <<= (8 - bitCount);
        bitstream.push_back(bitBuf);
        bitBuf = 0;
        bitCount = 0;
    }
}

void Huffman::freeTree(Node *node)
{
    if (!node)
    {
        return;
    }
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

Huffman::~Huffman()
{
    freeTree(root);
}