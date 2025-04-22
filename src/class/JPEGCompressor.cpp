#include "JPEGCompressor.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <unordered_map>

// Tables de quantification
const uint8_t JPEG_LUMINANCE_QUANT_TABLE[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}};
const uint8_t JPEG_CHROMINANCE_QUANT_TABLE[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}};

static const uint8_t dcLumBits[17] = {
    0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

static const uint8_t dcLumVals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const uint8_t acLumBits[17] = {
    0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};

static const uint8_t acLumVals[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x09, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA};

static const uint8_t dcChBits[17] = {
    0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
static const uint8_t dcChVals[12] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static const uint8_t acChBits[17] = {
    0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
static const uint8_t acChVals[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA};

static unordered_map<uint16_t, vector<bool>> buildStaticCodes(const uint8_t bits[17], const uint8_t vals[], int nvals)
{
    unordered_map<uint16_t, vector<bool>> codes;
    uint32_t code = 0;
    int ptr = 0;
    for (int len = 1; len <= 16; ++len)
    {
        for (int cnt = 0; cnt < bits[len]; ++cnt)
        {
            uint16_t sym = vals[ptr++];
            vector<bool> vec(len);
            for (int b = 0; b < len; ++b)
                vec[len - 1 - b] = (code >> b) & 1;
            codes[sym] = vec;
            ++code;
        }
        code <<= 1;
    }
    return codes;
}

JPEGCompressor::JPEGCompressor(Image *img)
    : image(img), width(img->getWidth()), height(img->getHeight()), pixels(img->getPixels()) {}

bool JPEGCompressor::compressAndSave(const string &outputPath)
{
    // 1. RGB -> YCbCr
    convertToYCbCr();

    // 2. Split en blocs 8×8
    splitToBlocks(yChan, yBlocks);
    splitToBlocks(cbChan, cbBlocks);
    splitToBlocks(crChan, crBlocks);

    // 3. DCT
    vector<Block> dY, dCb, dCr;
    performDCT(yBlocks, dY);
    performDCT(cbBlocks, dCb);
    performDCT(crBlocks, dCr);

    // 4. Quantification
    vector<Block> qY, qCb, qCr;
    quantify(dY, JPEG_LUMINANCE_QUANT_TABLE, qY);
    quantify(dCb, JPEG_CHROMINANCE_QUANT_TABLE, qCb);
    quantify(dCr, JPEG_CHROMINANCE_QUANT_TABLE, qCr);

    // 5. RLE
    vector<vector<pair<int, int>>> rleY, rleCb, rleCr;
    buildRLE(qY, rleY);
    buildRLE(qCb, rleCb);
    buildRLE(qCr, rleCr);

    // 6. Codes statiques
    auto codesYDC = buildStaticCodes(dcLumBits, dcLumVals, 12);
    auto codesYAC = buildStaticCodes(acLumBits, acLumVals, 162);
    auto codesCDC = buildStaticCodes(dcChBits, dcChVals, 12);
    auto codesCAC = buildStaticCodes(acChBits, acChVals, 162);

    // 7. Encodage bitstream (avec amplitude bits)
    vector<uint8_t> bitstream;
    uint8_t buf = 0;
    int bitCount = 0;
    auto writeBit = [&](bool b)
    {
        buf = (buf << 1) | (b ? 1 : 0);
        if (++bitCount == 8)
        {
            bitstream.push_back(buf);
            buf = 0;
            bitCount = 0;
        }
    };
    auto writeBits = [&](const vector<bool> &bits)
    {
        for (bool b : bits)
            writeBit(b);
    };
    // écrit les bits d'amplitude pour un coefficient non nul
    auto writeValueBits = [&](int v)
    {
        if (v == 0)
            return;
        int cat = 32 - __builtin_clz(abs(v)) - 1;
        int amp = (v < 0) ? (v + (1 << (cat + 1)) - 1) : v;
        for (int i = cat; i >= 0; --i)
            writeBit((amp >> i) & 1);
    };

    size_t n = rleY.size();
    for (size_t i = 0; i < n; ++i)
    {
        // — DC Y —
        int diffY = rleY[i][0].second;
        int catY = (diffY == 0 ? 0 : 32 - __builtin_clz(abs(diffY)) - 1);
        writeBits(codesYDC[catY]);
        if (catY > 0)
            writeValueBits(diffY);

        // — AC Y —
        for (size_t j = 1; j < rleY[i].size(); ++j)
        {
            int run = rleY[i][j].first, v = rleY[i][j].second;
            if (run == 0 && v == 0)
            {
                writeBits(codesYAC[0]);
                break;
            } // EOB
            if (run == 15 && v == 0)
            {
                writeBits(codesYAC[0xF0]);
                continue;
            } // ZRL
            int catAC = 32 - __builtin_clz(abs(v)) - 1;
            int sym = (run << 4) | catAC;
            writeBits(codesYAC[sym]);
            writeValueBits(v);
        }

        // — DC Cb —
        int diffCb = rleCb[i][0].second;
        int catCb = (diffCb == 0 ? 0 : 32 - __builtin_clz(abs(diffCb)) - 1);
        writeBits(codesCDC[catCb]);
        if (catCb > 0)
            writeValueBits(diffCb);

        // — AC Cb —
        for (size_t j = 1; j < rleCb[i].size(); ++j)
        {
            int run = rleCb[i][j].first, v = rleCb[i][j].second;
            if (run == 0 && v == 0)
            {
                writeBits(codesCAC[0]);
                break;
            }
            if (run == 15 && v == 0)
            {
                writeBits(codesCAC[0xF0]);
                continue;
            }
            int catAC = 32 - __builtin_clz(abs(v)) - 1;
            int sym = (run << 4) | catAC;
            writeBits(codesCAC[sym]);
            writeValueBits(v);
        }

        // — AC Cr (mêmes codes que Cb) —
        for (size_t j = 1; j < rleCr[i].size(); ++j)
        {
            int run = rleCr[i][j].first, v = rleCr[i][j].second;
            if (run == 0 && v == 0)
            {
                writeBits(codesCAC[0]);
                break;
            }
            if (run == 15 && v == 0)
            {
                writeBits(codesCAC[0xF0]);
                continue;
            }
            int catAC = 32 - __builtin_clz(abs(v)) - 1;
            int sym = (run << 4) | catAC;
            writeBits(codesCAC[sym]);
            writeValueBits(v);
        }
    }
    if (bitCount > 0)
    {
        buf <<= (8 - bitCount);
        bitstream.push_back(buf);
    }

    // 8. Sauvegarde
    return JPEGWriter::saveColor(outputPath,
                                 width, height,
                                 JPEG_LUMINANCE_QUANT_TABLE,
                                 JPEG_CHROMINANCE_QUANT_TABLE,
                                 bitstream);
}

void JPEGCompressor::convertToYCbCr()
{

    int N = width * height;
    yChan.resize(N);
    cbChan.resize(N);
    crChan.resize(N);

    for (int i = 0; i < N; ++i)
    {
        float r = pixels[i].R, g = pixels[i].G, b = pixels[i].B;
        float Y = 0.299f * r + 0.587f * g + 0.114f * b;
        float Cb = -0.1687f * r - 0.3313f * g + 0.5f * b + 128.0f;
        float Cr = 0.5f * r - 0.4187f * g - 0.0813f * b + 128.0f;
        if (i == 0)
        {
            cout << "DEBUG Cr[" << i << "] = " << Cr << endl;
        }

        yChan[i] = Y - 128.0f;
        cbChan[i] = Cb;
        crChan[i] = Cr;
    }
}

void JPEGCompressor::splitToBlocks(const vector<float> &chan,
                                   vector<Block> &blocks)
{
    blocks.clear();
    for (int by = 0; by < height; by += 8)
        for (int bx = 0; bx < width; bx += 8)
        {
            Block blk{};
            for (int y = 0; y < 8; ++y)
                for (int x = 0; x < 8; ++x)
                {
                    int yy = by + y, xx = bx + x;
                    float v = (xx < width && yy < height)
                                  ? chan[yy * width + xx]
                                  : 0;
                    blk.block[y][x] = v;
                }
            blocks.push_back(blk);
        }
}

void JPEGCompressor::performDCT(const vector<Block> &in, vector<Block> &out)
{
    out.clear();
    out.reserve(in.size());
    auto alpha = [&](int u)
    { return u == 0 ? sqrt(1.f / 8) : sqrt(2.f / 8); };
    for (auto &blk : in)
    {
        Block dct{};
        for (int u = 0; u < 8; ++u)
            for (int v = 0; v < 8; ++v)
            {
                float s = 0;
                for (int y = 0; y < 8; ++y)
                    for (int x = 0; x < 8; ++x)
                        s += blk.block[y][x] *
                             cos((2 * x + 1) * u * M_PI / 16) *
                             cos((2 * y + 1) * v * M_PI / 16);
                dct.block[u][v] = alpha(u) * alpha(v) * s;
            }
        out.push_back(dct);
    }
}

void JPEGCompressor::quantify(const vector<Block> &in,
                              const uint8_t quant[8][8],
                              vector<Block> &out)
{
    out.clear();
    out.reserve(in.size());
    for (auto &blk : in)
    {
        Block q{};
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                q.block[y][x] = round(blk.block[y][x] / quant[y][x]);
        out.push_back(q);
    }
}

void JPEGCompressor::buildRLE(const vector<Block> &blocks,
                              vector<vector<pair<int, int>>> &rles)
{
    static const int zig[64] = {/* zigzag order */
                                0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42,
                                3, 8, 12, 17, 25, 30, 41, 43, 9, 11, 18, 24, 31, 40, 44, 53,
                                10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
                                21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};
    rles.clear();
    int prevDC = 0;
    for (auto &blk : blocks)
    {
        vector<pair<int, int>> rle;
        int dc = round(blk.block[0][0]);
        int diff = dc - prevDC;
        rle.emplace_back(0, diff);
        prevDC = dc;
        int run = 0;
        for (int i = 1; i < 64; ++i)
        {
            int y = zig[i] / 8, x = zig[i] % 8;
            int v = round(blk.block[y][x]);
            if (v == 0)
            {
                ++run;
            }
            else
            {
                while (run >= 16)
                {
                    rle.emplace_back(15, 0);
                    run -= 16;
                }
                rle.emplace_back(run, v);
                run = 0;
            }
        }
        while (run >= 16)
        {
            rle.emplace_back(15, 0);
            run -= 16;
        }
        rle.emplace_back(0, 0);
        rles.push_back(move(rle));
    }
}

void JPEGCompressor::buildHuffman(const vector<vector<pair<int, int>>> &rles,
                                  Huffman &h)
{
    vector<Huffman::Symbol> syms;
    for (auto &rle : rles)
        for (auto &p : rle)
            syms.push_back((p.first << 8) | (p.second & 0xFF));
    h.build(syms);
    h.encode(syms);
}