#include "JPEGCompressor.hpp"
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
    result.y = 0.299 * r + 0.587 * g + 0.114 * b;
    result.cb = -0.1687 * r - 0.3313 * g + 0.5 * b + 128;
    result.cr = 0.5 * r - 0.4187 * g - 0.0813 * b + 128;

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
std::vector<std::vector<double>> JPEGCompressor::applyDCT(const std::vector<std::vector<double>> &block)
{
    const double PI = std::acos(-1);
    std::vector<std::vector<double>> dctBlock(8, std::vector<double>(8, 0.0));

    for (int u = 0; u < 8; u++)
    {
        for (int v = 0; v < 8; v++)
        {
            double sum = 0.0;
            for (int x = 0; x < 8; x++)
            {
                for (int y = 0; y < 8; y++)
                {
                    sum += block[x][y] *
                           std::cos((2 * x + 1) * u * PI / 16.0) *
                           std::cos((2 * y + 1) * v * PI / 16.0);
                }
            }

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

    const std::vector<std::vector<double>> &block = blocksCb[0];
    std::cout << "DCT coefficients for first Y block:\n";
    for (const std::vector<double> &row : block)
    {
        for (double val : row)
        {
            std::cout << std::fixed << std::setprecision(1) << val << "\t";
        }
        std::cout << "\n";
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
        qBlocksY.push_back(quantizeBlock(block, standardLuminanceQuantTable));

    // Quantize all Cb blocks
    for (const auto &block : blocksCb)
        qBlocksCb.push_back(quantizeBlock(block, standardChrominanceQuantTable));

    // Quantize all Cr blocks
    for (const auto &block : blocksCr)
        qBlocksCr.push_back(quantizeBlock(block, standardChrominanceQuantTable));
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
