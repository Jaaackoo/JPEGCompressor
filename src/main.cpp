#include "class/imageExtension/PPMImage.cpp"
#include "class/JPEGCompressor.hpp"

#include "tools/utils.hpp"
using namespace std;

#include <iostream>

int main()
{

    string fileName = "Clown.ppm";
    string outputFileName = "outputClown.ppm";

    PPMImage img;
    if (!img.load(fileName))
    {
        return 1;
    }

    JPEGCompressor compressor(img);

    compressor.compress();

    std::vector<int> zz = compressor.zigzagScan(compressor.getQuantizedYBlock(0));
    std::cout << "Zig-Zag scan of Y block 0:\n";
    for (int val : zz)
    {
        std::cout << val << " ";
    }
    std::cout << "\n";

    auto rle = compressor.runLengthEncode(zz);

    std::cout << "RLE output:\n";
    for (const auto &[zeros, value] : rle)
    {
        std::cout << "(" << zeros << "," << value << ") ";
    }
    std::cout << "\n";

    // compressor.printQuantizedBlockY(0);

    // PPMImage reconstructed = compressor.reconstructRGBImage();
    // reconstructed.save("resampledImage.ppm");

    // test_YcrCb(&img);

    return 0;
}