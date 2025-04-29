#ifndef _JPEGCOMPRESSOR_HPP_
#define _JPEGCOMPRESSOR_HPP_

#include "Image.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <iostream>
#include "imageExtension/PPMImage.hpp"
#include <cmath>
#include <iomanip>

using namespace std;

struct YCbCrPixel
{
    double y;
    double cb;
    double cr;
};

class JPEGCompressor
{
public:
    JPEGCompressor(Image &image);
    void compress(void);
    const std::vector<std::vector<int>> &getQuantizedYBlock(int index) const;

    void convertToYCbCr();
    void subsample420();
    void splitIntoBlocks();

    void applyDCTToAllBlocks();
    std::vector<std::vector<int>> quantizeBlock(const std::vector<std::vector<double>> &block, const uint8_t table[8][8]);
    void quantizeAllBlocks();

    std::vector<int> zigzagScan(const std::vector<std::vector<int>> &block);

    // test
    PPMImage reconstructRGBImage() const;
    void printQuantizedBlockY(int blockIndex) const;

private:
    int width;
    int height;
    vector<Pixel> pixels;
    vector<YCbCrPixel> ycbcrPixels; // Converted YCbCr pixels

    // subsampling
    vector<vector<double>> Y;
    vector<vector<double>> Cb;
    vector<vector<double>> Cr;
    vector<vector<double>> Cb_420;
    vector<vector<double>> Cr_420;

    // DCT
    std::vector<std::vector<std::vector<double>>> blocksY;
    std::vector<std::vector<std::vector<double>>> blocksCb;
    std::vector<std::vector<std::vector<double>>> blocksCr;

    std::vector<std::vector<double>> applyDCT(const std::vector<std::vector<double>> &block);

    // Quantization
    std::vector<std::vector<std::vector<int>>> qBlocksY;
    std::vector<std::vector<std::vector<int>>> qBlocksCb;
    std::vector<std::vector<std::vector<int>>> qBlocksCr;

    YCbCrPixel RGBtoYCbCr(const Pixel &pixel);
};

#endif // JPEGCOMPRESSOR_HPP
