#ifndef _JPEGCOMPRESSOR_HPP_
#define _JPEGCOMPRESSOR_HPP_

#include "Image.hpp"
#include <iostream>

struct Block
{
    float block[8][8];
};

struct YCbCr
{
    float y, cb, cr;
};

class JPEGCompressor
{
protected:
    int height;
    int width;

    vector<Pixel> pixels;
    vector<YCbCr> ycbcr;

    // For compress
    vector<Block> yBlocks;
    vector<Block> crBlocks;
    vector<Block> cBlocks;

private:
    void convertToYcbCR();
    void splitYToBlocks();

public:
    JPEGCompressor(int width, int height, const vector<Pixel> &pixels);
    JPEGCompressor(Image *image);

    void compress();
};

#endif