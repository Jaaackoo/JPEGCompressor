#include "PPMImage.hpp"

bool PPMImage::load(const string &filename)
{
    string path = string(INPUT) + filename;
    ifstream file(path, ios::binary);

    // Ensure we can open image
    if (!file.is_open())
    {
        cerr << "Error: cannot open " << path << endl;
        return false;
    }

    // Check fileType
    file >> this->fileType;

    if (this->fileType == "P3")
    {
        return loadP3(file);
    }
    else if (this->fileType == "P6")
    {
        return loadP6(file);
    }
    else
    {
        cerr << "Error: Format isn't handle yet : " << this->fileType << endl;
        return false;
    }
}

bool PPMImage::loadP3(ifstream &file)
{

    // Sauter les commentaires
    while (file >> ws && file.peek() == '#')
    {
        string comment;
        getline(file, comment);
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

bool PPMImage::loadP6(ifstream &file)
{

    while (file >> ws && file.peek() == '#')
    {
        string comment;
        getline(file, comment);
    }

    file >> this->width >> this->height;
    file >> this->maxVal;

    pixels.resize(width * height);
    for (auto &p : pixels)
    {
        char rgb[3];
        file.read(rgb, 3);
        p = {static_cast<uint8_t>(rgb[0]), static_cast<uint8_t>(rgb[1]), static_cast<uint8_t>(rgb[2])};
    }
    return true;
}

bool PPMImage::saveP3(const string &fileName)
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
    file << this->fileType << endl
         << this->width << " " << this->height << endl
         << this->maxVal << endl;
         
    for (const auto &p : pixels)
    {
        file << static_cast<int>(p.R) << " "
             << static_cast<int>(p.G) << " "
             << static_cast<int>(p.B) << "\n";
    }

    return true;
}

bool PPMImage::saveP6(const string &filename)
{
    string path = string(OUTPUT) + filename;
    ofstream file(path, ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: cannot write " << path << endl;
        return false;
    }

    // Writes headers
    file << this->fileType << endl
         << width << " " << height << endl
         << maxVal << endl;

    // Write the file
    for (const auto &p : pixels)
    {
        // Tells compilator that it's a char 
        file.write(reinterpret_cast<const char *>(&p.R), 1);
        file.write(reinterpret_cast<const char *>(&p.G), 1);
        file.write(reinterpret_cast<const char *>(&p.B), 1);
    }

    return true;
}

bool PPMImage::save(const string &filename)
{
    if (this->fileType == "P3")
    {
        return saveP3(filename);
    }
    else if (this->fileType == "P6")
    {
        return saveP6(filename);
    }

    return false;
}
