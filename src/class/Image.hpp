#ifndef _IMAGE_HPP_
#define _IMAGE_HPP_

#include <cstdint>
#include <vector>
#include <string>

#define INPUT "assets/input/"
#define OUTPUT "assets/output/"

using namespace std;

/**
 * @brief RGB pixels
 *
 */
struct Pixel
{
    uint8_t R, G, B;
};

class Image
{
protected:
    /**
     * @brief Width of image
     *
     */
    int width = 0;

    /**
     * @brief Height of image
     *
     */
    int height = 0;

    /**
     * @brief Contains RGB pixels
     *
     */
    vector<Pixel> pixels;

public:
    /**
     * @brief Destroy the Image object
     *
     */
    virtual ~Image() = default;

    /**
     * @brief Load image
     *
     * @param path path to image
     * @return true
     * @return false
     */
    virtual bool load(const string &fileName) = 0;

    /**
     * @brief Save image
     *
     * @param path path to save image
     * @return true
     * @return false
     */
    virtual bool save(const string &fileName) = 0;

    /**
     * @brief Get the Width object
     *
     * @return int
     */
    int getWidth() const;

    /**
     * @brief Get the Height object
     *
     * @return int
     */
    int getHeight() const;

    /**
     * @brief Get the Pixels object, cannot apply any modifiers
     *
     * @return const vector<Pixel>&
     */
    const vector<Pixel> &getPixels() const;

    /**
     * @brief Get the Pixels object, can apply modifiers
     *
     * @return vector<Pixel>&
     */
    vector<Pixel> &getPixels();
};
#endif