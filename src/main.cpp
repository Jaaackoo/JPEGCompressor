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
    compressor.writeJPEGFile("ALED.jpg");

    return 0;
    // compressor.compress();

    // 3. Convert RGB → YCbCr
    // compressor.convertToYCbCr();

    // // 4. Subsample (4:2:0)
    // compressor.subsample420();
    // PPMImage reconstructed = compressor.reconstructRGBImage();
    // reconstructed.save("resampledImage.ppm");

    // // 5. Split into 8x8 blocks
    // compressor.splitIntoBlocks();

    // // 6. Apply DCT to all blocks
    // compressor.applyDCTToAllBlocks();

    // // 7. Quantize all blocks
    // compressor.quantizeAllBlocks();

    // // 8. Print quantized first Y block (optional debug)
    // compressor.printQuantizedBlockY(0);

    // // 9. Zigzag scan first quantized Y block
    // std::vector<int> zz = compressor.zigzagScan(compressor.getQuantizedYBlock(0));

    // std::cout << "\nZigzag scan of first Y block:\n";
    // for (int val : zz)
    // {
    //     std::cout << val << " ";
    // }
    // std::cout << "\n";

    // // 10. Run-Length Encode the zigzag block
    // auto rle = compressor.runLengthEncode(zz);

    // std::cout << "\nRun-Length Encoding output:\n";
    // for (const auto &[zeros, value] : rle)
    // {
    //     std::cout << "(" << zeros << "," << value << ") ";
    // }
    // std::cout << "\n";

    // // 11. Huffman encode DC
    // std::cout << "\nHuffman DC encoding:\n";
    // std::cout << compressor.huffmanEncodeDC(rle[0].second) << "\n";

    // // 12. Huffman encode AC
    // std::cout << "\nHuffman AC encoding:\n";
    // std::cout << compressor.huffmanEncodeAC(rle) << "\n";

    // return 0;
}

// compressor.printQuantizedBlockY(0);

// test_YcrCb(&img);

// return 0;
// }