#include "PPMImage.hpp"

bool PPMImage::load(const string &fileName)
{
    string path = string(INPUT) + fileName;

    // Ensure it can access image
    ifstream file(path);
    if (!file.is_open())
    {
        cerr << "Error: cannot open " << path << endl;
        return false;
    }

    // Check file format
    string fileType;
    file >> fileType;

    if (fileType != "P3")
    {
        cerr << "Error: PPM format not handle (only P3 (for now))" << endl;
        return false;
    }

    // Get width and height
    file >> this->width >> this->height;

    // Get the maxValue (0 - 255) of the ppm image
    file >> this->maxVal;


    // Resize memory for every pixels
    pixels.resize(this->width * this->height);
    for (int i = 0; i < width * height; ++i)
    {
        int r, g, b;
        file >> r >> g >> b;

        // Casts value to ensure type is uint8_t
        pixels[i] = {static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
    }

    return true;
}

bool PPMImage::save(const string &fileName)
{

    string path = string(OUTPUT) + fileName;

    // Ensure we can write image
    ofstream file(path);
    if (!file.is_open())
    {
        cerr << "Error: cannot write " << path << endl;
        return false;
    }

    // Write headers
    file << "P3" << endl << this->width << " " << this->height << endl << this->maxVal << endl;
    for (const auto &p : pixels)
    {
        file << static_cast<int>(p.R) << " "    
             << static_cast<int>(p.G) << " "
             << static_cast<int>(p.B) << "\n";
    }

    return true;
}