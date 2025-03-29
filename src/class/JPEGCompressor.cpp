#include "JPEGCompressor.hpp"

JPEGCompressor::JPEGCompressor(int width, int height, const vector<Pixel> &pixels)
{
    this->height = height;
    this->width = width;
    this->pixels = pixels;
}

JPEGCompressor::JPEGCompressor(Image *image)
{
    this->height = image->getHeight();
    this->width = image->getWidth();
    this->pixels = image->getPixels();
}

void JPEGCompressor::convertToYcbCR()
{
    ycbcr.resize(this->pixels.size());

    for (size_t i = 0; i < this->pixels.size(); ++i)
    {
        const Pixel &p = this->pixels[i];

        float red = static_cast<float>(p.R);
        float green = static_cast<float>(p.G);
        float blue = static_cast<float>(p.B);

        YCbCr pixelYcbCr;

        // See Informations/Conversion_RGB_YcbCr

        // y: [0, 255]
        // cb - cr : Center on 128 with extremum near 0 or 255
        pixelYcbCr.y = (0.299f * red) + (0.587f * green) + (0.114f * blue);
        pixelYcbCr.cb = (-0.1687f * red) - (0.3313f * green) + (0.5f * blue) + 128;
        pixelYcbCr.cr = (0.5f * red) - (0.4187f * green) - (0.0813f * blue) + 128;

        ycbcr[i] = pixelYcbCr;
    }
}

void JPEGCompressor::splitYToBlocks()
{

    // Block of height by block of height
    for (int by = 0; by < this->height; by += 8)
    {
        for (int bx = 0; bx < this->width; bx += 8)
        {
            Block subBlock;

            // Iterates on the block of 8
            for (int yPos = 0; yPos < 8; yPos++)
            {
                for (int xPos = 0; xPos < 8; xPos++)
                {
                    int x = bx + xPos;
                    int y = by + yPos;

                    // Ensure that we are not overflowing image (image can be != %8)
                    if (x < width && y < height)
                    {
                        int index = y * width + x;
                        subBlock.block[yPos][xPos] = this->ycbcr[index].y;
                    }
                    else
                    {
                        subBlock.block[yPos][xPos] = 0.0f; // 0 if out of bound (padding)
                    }
                }
            }
            yBlocks.push_back(subBlock);
        }
    }
}

void JPEGCompressor::compress()
{
    cout << "[JPEG] → Conversion RGB → YCbCr\n";
    this->convertToYcbCR();
    cout << "[JPEG] → TODO: Découpage en blocs + DCT + Quantification\n";
}