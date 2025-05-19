#include "JPEGCompressor.hpp"



// Build a map from symbol→(code,length)
void buildHuffmanCodes(const uint8_t bits[16],
                       const uint8_t huffval[],
                       int valCount,
                       HuffmanCode outCodes[256])
{
    // JPEG canonical algorithm:
    // 1) For each bit-length L = 1..16, there are bits[L-1] codes.
    // 2) Starting with code = 0, assign codes in ascending order of symbol
    uint16_t code = 0;
    int idx = 0;
    for (int L = 1; L <= 16; ++L)
    {
        for (int i = 0; i < bits[L - 1]; ++i)
        {
            uint8_t symbol = huffval[idx++];
            outCodes[symbol] = {code, (uint8_t)L};
            ++code;
        }
        code <<= 1;
    }
}

void writeBits(uint16_t bits, uint8_t length, ofstream& file) {
    static uint8_t currentByte = 0;
    static int bitPosition = 0;
    // flush + reset si length==0
    if (length == 0) {
        if (bitPosition > 0) {
            file.put(currentByte);
            // byte‑stuffing obligatoire en JPEG :
            if (currentByte == 0xFF)
                file.put((uint8_t)0x00);
        }
        // réinitialisation complète
        currentByte = 0;
        bitPosition  = 0;
        return;
    }


    for (int i = length - 1; i >= 0; --i) {
        currentByte |= ((bits >> i) & 1) << (7 - bitPosition);
        bitPosition++;
        if (bitPosition == 8) {
            file.put(currentByte);
            if (currentByte == 0xFF)
                file.put((uint8_t)0x00);
            currentByte = 0;
            bitPosition = 0;
        }
    }
}

const uint8_t standardLuminanceQuantTable[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}};

const uint8_t standardChrominanceQuantTable[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}};

static const int zigzagMap[64][2] = {
    {0, 0}, {0, 1}, {1, 0}, {2, 0}, {1, 1}, {0, 2}, {0, 3}, {1, 2}, {2, 1}, {3, 0}, {4, 0}, {3, 1}, {2, 2}, {1, 3}, {0, 4}, {0, 5}, {1, 4}, {2, 3}, {3, 2}, {4, 1}, {5, 0}, {6, 0}, {5, 1}, {4, 2}, {3, 3}, {2, 4}, {1, 5}, {0, 6}, {0, 7}, {1, 6}, {2, 5}, {3, 4}, {4, 3}, {5, 2}, {6, 1}, {7, 0}, {7, 1}, {6, 2}, {5, 3}, {4, 4}, {3, 5}, {2, 6}, {1, 7}, {2, 7}, {3, 6}, {4, 5}, {5, 4}, {6, 3}, {7, 2}, {7, 3}, {6, 4}, {5, 5}, {4, 6}, {3, 7}, {4, 7}, {5, 6}, {6, 5}, {7, 4}, {7, 5}, {6, 6}, {5, 7}, {6, 7}, {7, 6}, {7, 7}};

JPEGCompressor::JPEGCompressor(Image &image)
{
    this->height = image.getHeight();
    this->width = image.getWidth();
    this->pixels = image.getPixels();
}

YCbCrPixel JPEGCompressor::RGBtoYCbCr(const Pixel &pixel)
{
    double r = static_cast<double>(pixel.R);
    double g = static_cast<double>(pixel.G);
    double b = static_cast<double>(pixel.B);

    YCbCrPixel result;
    result.y = (0.299 * r) + (0.587 * g) + (0.114 * b);
    result.cb = (-0.1687 * r) + (-0.3313 * g) + (0.5 * b) + 128;
    result.cr = (0.5 * r) + (-0.4187 * g) + (-0.0813 * b) + 128;

    return result;
}

void JPEGCompressor::convertToYCbCr()
{
    ycbcrPixels.reserve(pixels.size());
    for (const Pixel &pixel : pixels)
    {
        ycbcrPixels.push_back(this->RGBtoYCbCr(pixel));
    }
}

void JPEGCompressor::compress(void)
{
    this->convertToYCbCr();
    this->subsample420();
    this->splitIntoBlocks();
    this->applyDCTToAllBlocks();
    this->quantizeAllBlocks();
    if (!qBlocksCb.empty())
    {
        std::cout << "Cb DC[0]: " << qBlocksCb[0][0][0]
                  << "   Cr DC[0]: " << qBlocksCr[0][0][0] << "\n";
    }
}

/**
 * @brief 4:2:0 subsampling:
 *   Y (luma): Full resolution (every pixel)
 *   Cb/Cr (chroma): Subsampled by 2 in width and 2 in height
 *   So for every 2×2 Y pixels, you have 1 Cb and 1 Cr
 *
 */
void JPEGCompressor::subsample420()
{
    // Initialize full Y, Cb, Cr planes
    Y.resize(height, std::vector<double>(width));
    Cb.resize(height, std::vector<double>(width));
    Cr.resize(height, std::vector<double>(width));

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const YCbCrPixel &p = ycbcrPixels[y * width + x];
            Y[y][x] = p.y;
            Cb[y][x] = p.cb;
            Cr[y][x] = p.cr;
        }
    }

    // Allocate subsampled Cb and Cr
    int halfHeight = (height + 1) / 2;
    int halfWidth = (width + 1) / 2;

    Cb_420.resize(halfHeight, std::vector<double>(halfWidth));
    Cr_420.resize(halfHeight, std::vector<double>(halfWidth));

    for (int y = 0; y < height; y += 2)
    {
        for (int x = 0; x < width; x += 2)
        {
            double cbSum = 0.0;
            double crSum = 0.0;
            int count = 0;

            // Handle edges safely (even if width or height is odd)
            for (int dy = 0; dy < 2; dy++)
            {
                for (int dx = 0; dx < 2; dx++)
                {
                    int yy = y + dy;
                    int xx = x + dx;
                    if (yy < height && xx < width)
                    {
                        cbSum += Cb[yy][xx];
                        crSum += Cr[yy][xx];
                        count++;
                    }
                }
            }

            Cb_420[y / 2][x / 2] = cbSum / count;
            Cr_420[y / 2][x / 2] = crSum / count;
        }
    }
}

void JPEGCompressor::splitIntoBlocks()
{
    // Helper lambda to extract 8x8 blocks from a 2D matrix
    auto splitChannel = [](const std::vector<std::vector<double>> &channel, int blockHeight, int blockWidth)
    {
        std::vector<std::vector<std::vector<double>>> blocks;

        for (int by = 0; by < blockHeight; by += 8)
        {
            for (int bx = 0; bx < blockWidth; bx += 8)
            {
                std::vector<std::vector<double>> block(8, std::vector<double>(8));

                for (int dy = 0; dy < 8; dy++)
                {
                    for (int dx = 0; dx < 8; dx++)
                    {
                        int yy = std::min(by + dy, blockHeight - 1); // Clamp to edge
                        int xx = std::min(bx + dx, blockWidth - 1);  // Clamp to edge
                        block[dy][dx] = channel[yy][xx];
                    }
                }

                blocks.push_back(block);
            }
        }

        return blocks;
    };

    // Step 1: Split Y channel into 8x8 blocks
    blocksY = splitChannel(Y, height, width);

    // Step 2: Split Cb_420 into 8x8 blocks
    int cbHeight = (height + 1) / 2;
    int cbWidth = (width + 1) / 2;
    blocksCb = splitChannel(Cb_420, cbHeight, cbWidth);

    // Step 3: Split Cr_420 into 8x8 blocks
    blocksCr = splitChannel(Cr_420, cbHeight, cbWidth);
}

/**
 * @brief Apply DCT (Discrete Cosine Transform) on an 8x8 block
 *
 * @param block 8x8 input block
 * @return 8x8 transformed block
 */
std::vector<std::vector<double>> JPEGCompressor::applyDCT(
    const std::vector<std::vector<double>> &inBlock)
{
    // 1) Level-shift into signed range
    std::vector<std::vector<double>> block(8, std::vector<double>(8));
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            block[y][x] = inBlock[y][x] - 128.0;

    // 2) Now do your normal DCT on ‘block’
    const double PI = std::acos(-1);
    std::vector<std::vector<double>> dctBlock(8, std::vector<double>(8));
    for (int u = 0; u < 8; ++u)
    {
        for (int v = 0; v < 8; ++v)
        {
            double sum = 0;
            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                    sum += block[y][x] * std::cos((2 * y + 1) * u * PI / 16) * std::cos((2 * x + 1) * v * PI / 16);
            double cu = (u == 0) ? (1.0 / std::sqrt(2)) : 1.0;
            double cv = (v == 0) ? (1.0 / std::sqrt(2)) : 1.0;
            dctBlock[u][v] = 0.25 * cu * cv * sum;
        }
    }

    return dctBlock;
}

/**
 * @brief Apply DCT to all 8x8 blocks of Y, Cb, Cr
 */
void JPEGCompressor::applyDCTToAllBlocks()
{
    for (auto &block : blocksY)
    {
        block = applyDCT(block);
    }

    for (auto &block : blocksCb)
    {
        block = applyDCT(block);
    }

    for (auto &block : blocksCr)
    {
        block = applyDCT(block);
    }
}

std::vector<std::vector<int>> JPEGCompressor::quantizeBlock(
    const std::vector<std::vector<double>> &block,
    const uint8_t table[8][8])
{
    std::vector<std::vector<int>> quantized(8, std::vector<int>(8));
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            quantized[y][x] = static_cast<int>(std::round(block[y][x] / table[y][x]));
        }
    }
    return quantized;
}

void JPEGCompressor::quantizeAllBlocks()
{
    qBlocksY.clear();
    qBlocksCb.clear();
    qBlocksCr.clear();

    // Quantize all Y blocks
    for (const auto &block : blocksY)
    {
        qBlocksY.push_back(quantizeBlock(block, standardLuminanceQuantTable));
    }

    // Quantize all Cb blocks
    for (const auto &block : blocksCb)
    {
        qBlocksCb.push_back(quantizeBlock(block, standardChrominanceQuantTable));
    }

    // Quantize all Cr blocks
    for (const auto &block : blocksCr)
    {
        qBlocksCr.push_back(quantizeBlock(block, standardChrominanceQuantTable));
    }
}

const std::vector<std::vector<int>> &JPEGCompressor::getQuantizedYBlock(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= qBlocksY.size())
    {
        std::cerr << "Invalid Y block index!\n";
        static std::vector<std::vector<int>> dummy(8, std::vector<int>(8, 0));
        return dummy; // return safe dummy block
    }
    return qBlocksY[index];
}

std::vector<int> JPEGCompressor::zigzagScan(const std::vector<std::vector<int>> &block)
{
    std::vector<int> result;
    result.reserve(64);

    for (int i = 0; i < 64; ++i)
    {
        int y = zigzagMap[i][0];
        int x = zigzagMap[i][1];
        result.push_back(block[y][x]);
    }

    return result;
}

std::vector<std::pair<int, int>> JPEGCompressor::runLengthEncode(const std::vector<int> &zigzaggedBlock)
{
    std::vector<std::pair<int, int>> rle;
    int zeroCount = 0;

    // Start at index 1: DC is treated differently (first value)
    rle.push_back({0, zigzaggedBlock[0]}); // DC coefficient

    for (size_t i = 1; i < zigzaggedBlock.size(); i++)
    {
        if (zigzaggedBlock[i] == 0)
        {
            ++zeroCount;
        }
        else
        {
            rle.push_back({zeroCount, zigzaggedBlock[i]});
            zeroCount = 0;
        }
    }

    if (zeroCount > 0)
    {
        rle.push_back({0, 0}); // EOB (End Of Block)
    }

    return rle;
}

// Simplified DC encoding
void JPEGCompressor::huffmanEncodeDC(int dcDiff, ofstream& file, HuffmanCode dcLumaCodes[12])
{
    int category = 0;
    int temp = std::abs(dcDiff);

    while (temp > 0)
    {
        temp >>= 1;
        category++;
    }

    HuffmanCode huff = dcLumaCodes[category];

    writeBits(huff.code, huff.length, file);

    // Encode value bits
    if (category > 0) {
        uint16_t bits;
        if (dcDiff >= 0) {
            bits = dcDiff;
        } else {
            bits = (1 << category) - 1 + dcDiff; // JPEG negative value encoding
        }
        writeBits(bits, category, file);
    }
}

// Simplified AC encoding
void JPEGCompressor::huffmanEncodeAC(const std::vector<std::pair<int, int>> &rle,
                                     const HuffmanCode huffAC[256],
                                     ofstream& file)
{
    for (size_t i = 1; i < rle.size(); ++i)
    {
        int zeros = rle[i].first;
        int val = rle[i].second;

        if (val == 0 && zeros == 0)
        {
            // End-of-block (EOB)
            HuffmanCode eob = huffAC[0x00];
            writeBits(eob.code, eob.length, file);
            break;
        }

        while (zeros > 15)
        {
            // Write ZRL (16 zeros → code 0xF0)
            HuffmanCode zrl = huffAC[0xF0];
            writeBits(zrl.code, zrl.length, file);
            zeros -= 16;
        }

        int category = 0;
        int magnitude = std::abs(val);
        while (magnitude != 0) {
            magnitude >>= 1;
            category++;
        }

        int symbol = (zeros << 4) | category;
        HuffmanCode huff = huffAC[symbol];
        writeBits(huff.code, huff.length, file);

        if (category > 0)
        {
            uint16_t bits = (val >= 0) ? val : (1 << category) - 1 + val;
            writeBits(bits, category, file);
        }
    }
}


PPMImage JPEGCompressor::reconstructRGBImage() const
{
    std::vector<Pixel> reconstructedPixels;
    reconstructedPixels.reserve(width * height);

    auto clamp = [](double val)
    {
        return static_cast<uint8_t>(std::round(std::min(255.0, std::max(0.0, val))));
    };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {

            double yVal = Y[y][x];
            double cbVal = Cb_420[y / 2][x / 2];
            double crVal = Cr_420[y / 2][x / 2];

            // Inverse conversion: YCbCr to RGB
            double r = yVal + 1.402 * (crVal - 128);
            double g = yVal - 0.34414 * (cbVal - 128) - 0.71414 * (crVal - 128);
            double b = yVal + 1.772 * (cbVal - 128);

            reconstructedPixels.push_back({clamp(r),
                                           clamp(g),
                                           clamp(b)});
        }
    }

    // Create and return a PPMImage
    PPMImage out;
    out.setSize(width, height);
    out.setPixels(reconstructedPixels);
    out.setFileType("P6");
    out.setMaxVal(255);

    return out;
}

void JPEGCompressor::printQuantizedBlockY(int blockIndex) const
{
    if (blockIndex >= qBlocksY.size())
    {
        std::cerr << "Invalid block index!\n";
        return;
    }

    const auto &block = qBlocksY[blockIndex];
    std::cout << "Quantized DCT coefficients for Y block " << blockIndex << ":\n";

    for (const auto &row : block)
    {
        for (int val : row)
        {
            std::cout << val << "\t";
        }
        std::cout << "\n";
    }
}

int JPEGCompressor::getHeight() const
{
    return this->height;
}

int JPEGCompressor::getWidth() const
{
    return this->width;
}

#include <fstream>

void writeQuantizationTable(std::ofstream &file, const uint8_t table[8][8], uint8_t tableID)
{
    // DQT marker
    file.put(0xFF);
    file.put(0xDB);
    file.put(0x00);
    file.put(0x43);    // Length = 67 bytes
    file.put(tableID); // Pq = 0 (8-bit), Tq = tableID

    // Zigzag reorder
    const int zigzagOrder[64] = {
        0, 1, 5, 6, 14, 15, 27, 28,
        2, 4, 7, 13, 16, 26, 29, 42,
        3, 8, 12, 17, 25, 30, 41, 43,
        9, 11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54,
        20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61,
        35, 36, 48, 49, 57, 58, 62, 63};

    for (int i = 0; i < 64; ++i)
    {
        int row = zigzagOrder[i] / 8;
        int col = zigzagOrder[i] % 8;
        file.put(table[row][col]);
    }
}

void writeHuffmanTables(std::ofstream &file)
{
    // DC Luminance
    uint8_t bits_dc_luminance[16] = {
        0x00, // 1-bit codes:   0
        0x01, // 2-bit codes:   1
        0x05, // 3-bit codes:   5
        0x01, // 4-bit codes:   1
        0x01, // 5-bit codes:   1
        0x01, // 6-bit codes:   1
        0x01, // 7-bit codes:   1
        0x01, // 8-bit codes:   1
        0x01, // 9-bit codes:   1
        0x00, // 10-bit codes:  0
        0x00, // 11-bit codes:  0
        0x00, // 12-bit codes:  0
        0x00, // 13-bit codes:  0
        0x00, // 14-bit codes:  0
        0x00, // 15-bit codes:  0
        0x00  // 16-bit codes:  0
    };
    uint8_t val_dc_luminance[12] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    file.put(0xFF);
    file.put(0xC4);
    file.put(0x00);
    file.put(0x1F);
    file.put(0x00); // DC, Table 0
    file.write(reinterpret_cast<char *>(bits_dc_luminance), 16);
    file.write(reinterpret_cast<char *>(val_dc_luminance), 12);

    // AC Luminance
    uint8_t bits_ac_luminance[16] = {
        0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
        0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D};
    uint8_t val_ac_luminance[162] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
        0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
        0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
        0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
        0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA};

    file.put(0xFF);
    file.put(0xC4);
    file.put(0x00);
    file.put(0xB5); // Length = 418 (2 + 1 + 16 + 162)
    file.put(0x10); // AC, Table 0huffman
    file.write(reinterpret_cast<char *>(bits_ac_luminance), 16);
    file.write(reinterpret_cast<char *>(val_ac_luminance), 162);

    // DC Chrominance
    uint8_t bits_dc_chrominance[16] = {
        0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t val_dc_chrominance[12] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    file.put(0xFF);
    file.put(0xC4);
    file.put(0x00);
    file.put(0x1F);
    file.put(0x01); // DC, Table 1
    file.write(reinterpret_cast<char *>(bits_dc_chrominance), 16);
    file.write(reinterpret_cast<char *>(val_dc_chrominance), 12);

    // AC Chrominance
    uint8_t bits_ac_chrominance[16] = {
        0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
        0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77};
    uint8_t val_ac_chrominance[162] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
        0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
        0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
        0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
        0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
        0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
        0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
        0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
        0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
        0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA};

    file.put(0xFF);
    file.put(0xC4);
    file.put(0x00);
    file.put(0xB5);
    file.put(0x11); // AC, Table 1
    file.write(reinterpret_cast<char *>(bits_ac_chrominance), 16);
    file.write(reinterpret_cast<char *>(val_ac_chrominance), 162);
}

void JPEGCompressor::writeJPEGFile(const std::string &filename)
{
    // DC Luminance
    uint8_t bits_dc_luminance[16] = {
        0x00, // 1-bit codes:   0
        0x01, // 2-bit codes:   1
        0x05, // 3-bit codes:   5
        0x01, // 4-bit codes:   1
        0x01, // 5-bit codes:   1
        0x01, // 6-bit codes:   1
        0x01, // 7-bit codes:   1
        0x01, // 8-bit codes:   1
        0x01, // 9-bit codes:   1
        0x00, // 10-bit codes:  0
        0x00, // 11-bit codes:  0
        0x00, // 12-bit codes:  0
        0x00, // 13-bit codes:  0
        0x00, // 14-bit codes:  0
        0x00, // 15-bit codes:  0
        0x00  // 16-bit codes:  0
    };
    uint8_t val_dc_luminance[12] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};


    // DC Chrominance
    uint8_t bits_dc_chrominance[16] = {
        0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t val_dc_chrominance[12] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    // AC chrominance
    uint8_t bits_ac_luminance[16] = {
        0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
        0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D};
    uint8_t val_ac_luminance[162] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
        0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
        0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
        0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
        0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
        0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
        0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
        0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
        0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA};

    // AC Chrominance
    uint8_t bits_ac_chrominance[16] = {
        0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
        0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77};
    uint8_t val_ac_chrominance[162] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
        0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
        0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
        0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
        0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
        0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
        0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
        0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
        0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
        0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
        0xF9, 0xFA};

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        return;
    }

    // 1. SOI marker
    file.put(0xFF);
    file.put(0xD8);

    // 2. DQT (Define Quantization Table)
    writeQuantizationTable(file, standardLuminanceQuantTable, 0x00);
    writeQuantizationTable(file, standardChrominanceQuantTable, 0x01);

    // 3. SOF0 (Start of Frame - Baseline DCT)
    file.put(0xFF);
    file.put(0xC0);

    file.put(0x00);
    file.put(0x11); // 17 bytes total for 3 components

    file.put(0x08);
    file.put((height >> 8) & 0xFF);
    file.put(height & 0xFF);
    file.put((width >> 8) & 0xFF);
    file.put(width & 0xFF);
    file.put(0x03); // Nf = 3

    // Y component
    file.put(0x01); // ID = 1
    file.put(0x22); // H=2, V=2 (for 4:2:0) or 0x11 for 1×1
    file.put(0x00); // QTable = 0

    // Cb component
    file.put(0x02); // ID = 2
    file.put(0x11); // H=1, V=1
    file.put(0x01); // QTable = 1

    // Cr component
    file.put(0x03); // ID = 3
    file.put(0x11); // H=1, V=1
    file.put(0x01); // QTable = 1

    // 4. DHT (Define Huffman Table)
    writeHuffmanTables(file);
    HuffmanCode dcLumaCodes[12], acLumaCodes[256];
    HuffmanCode dcChromaCodes[12], acChromaCodes[256];

    // e.g. at init:
    buildHuffmanCodes(bits_dc_luminance, val_dc_luminance, 12, dcLumaCodes);
    buildHuffmanCodes(bits_ac_luminance, val_ac_luminance, 162, acLumaCodes);
    buildHuffmanCodes(bits_dc_chrominance, val_dc_chrominance, 12, dcChromaCodes);
    buildHuffmanCodes(bits_ac_chrominance, val_ac_chrominance, 162, acChromaCodes);

    // 6. SOS (Start of Scan)
    file.put(0xFF);
    file.put(0xDA);

    // We have 3 components, so length = 6 + 2*3 = 12
    file.put(0x00);
    file.put(0x0C);

    file.put(0x03); // 3 components in scan

    // Y  : component ID = 1, DC-table=0, AC-table=0 → selector = 0x00
    file.put(0x01);
    file.put(0x00);

    // Cb : component ID = 2, DC-table=1, AC-table=1 → selector = 0x11
    file.put(0x02);
    file.put(0x11);

    // Cr : component ID = 3, DC-table=1, AC-table=1 → selector = 0x11
    file.put(0x03);
    file.put(0x11);

    // spectral selection (baseline JPEG)
    file.put(0x00); // Ss
    file.put(0x3F); // Se
    file.put(0x00); // Ah/Al

    // 7. Compressed Entropy Data

    int prevY = 0, prevCb = 0, prevCr = 0;
    size_t nMCUs = qBlocksY.size() / 4;
    for (size_t m = 0; m < nMCUs; ++m)
    {
        // — Y 4:2:0 => 4 Y blocks per MCU
        for (int i = 0; i < 4; ++i)
        {
            auto &B = qBlocksY[m * 4 + i];
            int diff = B[0][0] - prevY;
            huffmanEncodeDC(diff, file, dcLumaCodes);
            prevY = B[0][0];
            huffmanEncodeAC(runLengthEncode(zigzagScan(B)), acLumaCodes, file);
        }
        // — Cb
        {
            auto &B = qBlocksCb[m];
            int diff = B[0][0] - prevCb;
            huffmanEncodeDC(diff, file, dcChromaCodes);
            prevCb = B[0][0];
            huffmanEncodeAC(runLengthEncode(zigzagScan(B)), acChromaCodes, file);
        }
        // — Cr
        {
            auto &B = qBlocksCr[m];
            int diff = B[0][0] - prevCr;
            huffmanEncodeDC(diff, file, dcChromaCodes);
            prevCr = B[0][0];
            huffmanEncodeAC(runLengthEncode(zigzagScan(B)), acChromaCodes, file);
        }
    }

    //Flush the buffer if needed
    writeBits(0,0,file);

    // 8. EOI
    file.put(0xFF);
    file.put(0xD9);
    file.close();

    std::cout << "JPEG successfully written to: " << filename << std::endl;
}