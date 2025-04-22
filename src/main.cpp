#include "class/imageExtension/PPMImage.cpp"
#include "class/JPEGCompressor.hpp"
#include "class/JPEGWriter.hpp"

#include "tools/utils.hpp"
using namespace std;


#include <iostream>

// === main.cpp ===
int main()
{
    PPMImage img;
    if (!img.load("Clown.ppm"))
        return 1;

    JPEGCompressor comp(&img);

    // Extra debug sanity for Cr
    float r = 255, g = 255, b = 255;
    float Cr = 0.5f * r - 0.4187f * g - 0.0813f * b + 128.0f;
    cout << "\n=== TEST: Cr of white pixel ===\n";
    cout << "Expected Cr: " << Cr << endl;

    comp.convertToYCbCr();

    vector<Block> yBlocks, cbBlocks, crBlocks;
    comp.splitToBlocks(comp.yChan, yBlocks);
    comp.splitToBlocks(comp.cbChan, cbBlocks);
    comp.splitToBlocks(comp.crChan, crBlocks);

    auto printBlock = [](const Block& blk, const string& label) {
        cout << "\n" << label << ":\n";
        for (int i = 0; i < 8; ++i)
        {
            for (int j = 0; j < 8; ++j)
                cout << blk.block[i][j] << " ";
            cout << "\n";
        }
    };

    printBlock(yBlocks[0], "Raw Y Block[0]");
    printBlock(cbBlocks[0], "Raw Cb Block[0]");
    printBlock(crBlocks[0], "Raw Cr Block[0]");

    vector<Block> dctY, dctCb, dctCr;
    comp.performDCT(yBlocks, dctY);
    comp.performDCT(cbBlocks, dctCb);
    comp.performDCT(crBlocks, dctCr);

    printBlock(dctY[0], "DCT Y Block[0]");
    printBlock(dctCb[0], "DCT Cb Block[0]");
    printBlock(dctCr[0], "DCT Cr Block[0]");

    vector<Block> quantY, quantCb, quantCr;
    comp.quantify(dctY, JPEG_LUMINANCE_QUANT_TABLE, quantY);
    comp.quantify(dctCb, JPEG_CHROMINANCE_QUANT_TABLE, quantCb);
    comp.quantify(dctCr, JPEG_CHROMINANCE_QUANT_TABLE, quantCr);

    printBlock(quantY[0], "Quantized Y Block[0]");
    printBlock(quantCb[0], "Quantized Cb Block[0]");
    printBlock(quantCr[0], "Quantized Cr Block[0]");

    vector<vector<pair<int, int>>> rleY, rleCb, rleCr;
    comp.buildRLE(quantY, rleY);
    comp.buildRLE(quantCb, rleCb);
    comp.buildRLE(quantCr, rleCr);

    cout << "\nRLE Y Block[0]:\n";
    for (const auto& pair : rleY[0])
        cout << "(" << pair.first << ", " << pair.second << ") ";

    cout << "\nRLE Cb Block[0]:\n";
    for (const auto& pair : rleCb[0])
        cout << "(" << pair.first << ", " << pair.second << ") ";

    cout << "\nRLE Cr Block[0]:\n";
    for (const auto& pair : rleCr[0])
        cout << "(" << pair.first << ", " << pair.second << ") ";

    cout << "\n";

    if (!comp.compressAndSave("Clown_color.jpg"))
    {
        cerr << "Erreur JPEG\n";
        return 1;
    }

    cout << "Fini\n";
    return 0;
}



// int main()
// {

//     string fileName = "Clown.ppm";
//     // string outputFileName = "outputClown.ppm";

//     PPMImage img;
//     if (!img.load(fileName)) {
//         return 1;
//     }

//     JPEGCompressor compressor(&img);

//     compressor.compress();   // va produire output.huff

//     // PPMImage ppm = PPMImage();
//     // if (ppm.load(fileName))
//     // {
//     //     cout << "Image size: (" << ppm.getWidth() << ", " << ppm.getHeight() << ")" << endl;
//     // }

//     // test_YcrCb(&ppm);
//     // test_splitYToBlocks(&ppm);
//     // test_DCT(&ppm);
//     // test_quantification(&ppm);
//     // test_zigzag(&ppm);

//     // array<int, 64> zz = test_zigzag(&ppm);
//     // auto encoded = JPEGCompressor::runLengthEncode(zz);

//     // cout << "RLE (ZigZag compressé) :\n";
//     // for (auto &[zeros, value] : encoded)
//     // {
//     //     cout << "(" << zeros << ", " << value << ") ";
//     // }

//     return 0;
// }