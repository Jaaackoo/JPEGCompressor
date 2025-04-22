// #include "utils.hpp"
// #include "../class/JPEGCompressor.hpp"
// #include <algorithm>
// #include <string>

// #include "../class/imageExtension/PPMImage.hpp"

// void test_YcrCb(Image *img)
// {
//     vector<Pixel> yPixels, cbPixels, crPixels;
//     const vector<Pixel> pixels = img->getPixels();

//     yPixels.reserve(pixels.size());
//     cbPixels.reserve(pixels.size());
//     crPixels.reserve(pixels.size());

//     cout << "abv" << endl;
//     for (const Pixel &p : pixels)
//     {
//         float r = static_cast<float>(p.R);
//         float g = static_cast<float>(p.G);
//         float b = static_cast<float>(p.B);

//         float y = 0.299f * r + 0.587f * g + 0.114f * b;
//         float cb = -0.1687f * r - 0.3313f * g + 0.5f * b + 128.0f;
//         float cr = 0.5f * r - 0.4187f * g - 0.0813f * b + 128.0f;

//         // Pour l'affichage en niveaux de gris
//         uint8_t Y = clamp(static_cast<int>(y), 0, 255);
//         uint8_t Cb = clamp(static_cast<int>(cb), 0, 255);
//         uint8_t Cr = clamp(static_cast<int>(cr), 0, 255);

//         yPixels.push_back({Y, Y, Y});     // On doit voir le même constraste de lumiere
//         cbPixels.push_back({Cb, Cb, Cb}); // On voit que les zones bleus
//         crPixels.push_back({Cr, Cr, Cr}); // On voit que les zones rouges
//     }

//     PPMImage yImg, cbImg, crImg;

//     yImg.setFileType("P6");
//     cbImg.setFileType("P6");
//     crImg.setFileType("P6");

//     yImg.setMaxVal(255);
//     cbImg.setMaxVal(255);
//     crImg.setMaxVal(255);

//     yImg.setSize(img->getHeight(), img->getWidth());
//     cbImg.setSize(img->getHeight(), img->getWidth());
//     crImg.setSize(img->getHeight(), img->getWidth());

//     yImg.setPixels(yPixels);
//     cbImg.setPixels(cbPixels);
//     crImg.setPixels(crPixels);

//     yImg.save("Clown_Y.ppm");
//     cbImg.save("Clown_Cb.ppm");
//     crImg.save("Clown_Cr.ppm");
// }

// void test_splitYToBlocks(Image *img)
// {

//     JPEGCompressor compressor = JPEGCompressor(img);

//     compressor.convertToYcbCR();
//     compressor.splitYToBlocks();

//     const vector<Block> blocks = compressor.getYBlocks();

//     cout << "Total blocs Y : " << blocks.size() << "\n";

//     // Afficher les 3 premiers blocks
//     for (size_t i = 0; i < 3; i++)
//     {
//         cout << "Block #" << i << " :" << endl;
//         for (int j = 0; j < 8; j++)
//         {
//             for (int k = 0; k < 8; k++)
//             {
//                 cout << blocks[i].block[j][k] << " ";
//             }
//             cout << endl;
//         }
//         cout << endl;
//     }
// }

// void test_DCT(Image *img)
// {
//     JPEGCompressor compressor(img);
//     compressor.convertToYcbCR();
//     compressor.splitYToBlocks();

//     const vector<Block> &yBlocks = compressor.getYBlocks();

//     cout << "🔍 DCT sur les 3 premiers blocs :\n";

//     // Afficher les 3 premiers blocks
//     for (size_t i = 0; i < 3; i++)
//     {

//         Block out;
//         compressor.DCT(yBlocks[i], out);
//         cout << "Block #" << i << " :" << endl;
//         for (int j = 0; j < 8; j++)
//         {
//             for (int k = 0; k < 8; k++)
//             {
//                 cout << out.block[j][k] << " ";
//             }
//             cout << endl;
//         }
//         cout << endl;
//     }
// }

// void test_quantification(Image *img)
// {
//     JPEGCompressor compressor(img);
//     compressor.convertToYcbCR();
//     compressor.splitYToBlocks();

//     const vector<Block> &yBlocks = compressor.getYBlocks();

//     // Générer la matrice de quantification pour qualité 50
//     uint8_t quantMatrix[8][8];

//     for (int y = 0; y < 8; y++)
//     {
//         for (int x = 0; x < 8; x++)
//         {
//             quantMatrix[y][x] = JPEG_LUMINANCE_QUANT_TABLE[y][x];
//         }
//     }

//     for (int i = 0; i < min<size_t>(3, yBlocks.size()); ++i)
//     {
//         Block dctBlock;
//         Block quantBlock;

//         compressor.DCT(yBlocks[i], dctBlock);

//         // Appliquer la quantification
//         for (int y = 0; y < 8; ++y)
//         {
//             for (int x = 0; x < 8; ++x)
//             {
//                 quantBlock.block[y][x] = round(dctBlock.block[y][x] / quantMatrix[y][x]);
//             }
//         }

//         cout << "Bloc #" << i << " avant quantification (DCT brut) :\n";

//         for (int j = 0; j < 8; j++)
//         {
//             for (int k = 0; k < 8; k++)
//             {
//                 cout << dctBlock.block[j][k] << " ";
//             }
//             cout << endl;
//         }

//         cout << "Bloc #" << i << " après quantification :\n";
//         for (int j = 0; j < 8; j++)
//         {
//             for (int k = 0; k < 8; k++)
//             {
//                 cout << quantBlock.block[j][k] << " ";
//             }
//             cout << endl;
//         }

//         cout << "---------------------------------------------\n";
//     }
// }

// array<int, 64UL> test_zigzag(Image *img)
// {
//     JPEGCompressor compressor(img);
//     compressor.convertToYcbCR();
//     compressor.splitYToBlocks();

//     const vector<Block> &yBlocks = compressor.getYBlocks();
//     Block dct;
//     compressor.DCT(yBlocks[0], dct);

//     uint8_t quantMatrix[8][8];

//     for (int y = 0; y < 8; y++)
//     {
//         for (int x = 0; x < 8; x++)
//         {
//             quantMatrix[y][x] = JPEG_LUMINANCE_QUANT_TABLE[y][x];
//         }
//     }
//     Block quant;
//     for (int y = 0; y < 8; ++y)
//         for (int x = 0; x < 8; ++x)
//             quant.block[y][x] = round(dct.block[y][x] / quantMatrix[y][x]);

//     array<int, 64> zz;
//     compressor.zigZag(quant, zz);

//     cout << "🌀 ZigZag sur bloc quantifié :\n";
//     for (int i = 0; i < 64; ++i)
//     {
//         cout << zz[i] << " ";
//         if ((i + 1) % 8 == 0)
//             cout << "\n";
//     }

//     return zz;
// }