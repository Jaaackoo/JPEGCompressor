#include "Image.hpp"

/**
 * @brief Get the Width object
 *
 * @return int
 */
int Image::getWidth() const
{
    return this->width;
}

/**
 * @brief Get the Height object
 *
 * @return int
 */
int Image::getHeight() const
{
    return this->height;
}

/**
 * @brief Get the Pixels object, cannot apply any modifiers
 *
 * @return const vector<Pixel>&
 */
const vector<Pixel> &Image::getPixels() const
{
    return this->pixels;
}

/**
 * @brief Get the Pixels object, can apply modifiers
 *
 * @return vector<Pixel>&
 */
vector<Pixel> &Image::getPixels()
{
    return this->pixels;
}
