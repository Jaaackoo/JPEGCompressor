#ifndef _HUFFMAN_HPP_
#define _HUFFMAN_HPP_

#include <vector>
#include <map>
#include <cstdint>
#include <algorithm>
using namespace std;

class Huffman
{
public:
    using Symbol = int;
    Huffman() = default;
    void build(const vector<Symbol> &symbols);
    void encode(const vector<Symbol> &symbols);
    const vector<uint8_t> &getBitstream() const { return bitstream; }
    const map<Symbol, vector<bool>> &getCodes() const { return codes; }
    ~Huffman();

private:
    struct Node
    {
        Symbol symbol;
        int freq;
        Node *left, *right;
        Node(Symbol s, int f, Node *l = nullptr, Node *r = nullptr)
            : symbol(s), freq(f), left(l), right(r) {}
    };
    struct NodeCmp
    {
        bool operator()(Node *a, Node *b) const { return a->freq > b->freq; }
    };
    Node *root = nullptr;
    map<Symbol, vector<bool>> codes;
    vector<uint8_t> bitstream;
    uint8_t bitBuf = 0;
    int bitCount = 0;

    void generateCodes(Node *node, vector<bool> &prefix);
    void freeTree(Node *node);
    void writeBit(bool b);
    void writeBits(const vector<bool> &bits);
    void flushBits();
};

#endif // _HUFFMAN_HPP_