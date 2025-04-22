#ifndef _JPEGCOMPRESSOR_HPP_
#define _JPEGCOMPRESSOR_HPP_

#include "JPEGWriter.hpp"
#include "Huffman.hpp"
#include "Image.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <string>

using namespace std;

extern const uint8_t JPEG_LUMINANCE_QUANT_TABLE[8][8];
extern const uint8_t JPEG_CHROMINANCE_QUANT_TABLE[8][8];

typedef struct { float block[8][8]; } Block;

class JPEGCompressor {
public:
    JPEGCompressor(Image *img);
    bool compressAndSave(const string &outputPath);
    Image *image;
    int width, height;
    vector<Pixel> pixels;
    vector<float> yChan, cbChan, crChan;
    vector<Block> yBlocks, cbBlocks, crBlocks;
    vector<vector<pair<int,int>>> rleY, rleCb, rleCr;
    Huffman hY, hCb, hCr;

    void convertToYCbCr();
    void splitToBlocks(const vector<float> &chan, vector<Block> &blocks);
    void performDCT(const vector<Block> &in, vector<Block> &out);
    void quantify(const vector<Block> &in,
                  const uint8_t quant[8][8],
                  vector<Block> &out);
    void buildRLE(const vector<Block> &blocks,
                  vector<vector<pair<int,int>>> &rles);
    void buildHuffman(const vector<vector<pair<int,int>>> &rles,
                      Huffman &h);
};

#endif // _JPEGCOMPRESSOR_HPP_