#ifndef _JPEGWRITER_HPP_
#define _JPEGWRITER_HPP_

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iostream>
using namespace std;

class JPEGWriter
{
public:
    /**
     * @brief Enregistre un JPEG baseline 4:4:4 couleur (Y, Cb, Cr)
     * @param filename   Chemin du fichier .jpg
     * @param width      Largeur de l’image
     * @param height     Hauteur de l’image
     * @param quantY     Table de quantification luminance (8×8)
     * @param quantC     Table de quantification chrominance (8×8)
     * @param bitstream  Flux compressé par MCU (avec stuffing fait en amont)
     * @return true en cas de succès
     */
    static bool saveColor(const string &filename,
                          int width, int height,
                          const uint8_t quantY[8][8],
                          const uint8_t quantC[8][8],
                          const vector<uint8_t> &bitstream);

private:
    static void writeMarker(ofstream &out, uint8_t m1, uint8_t m2)
    {
        out.put(m1);
        out.put(m2);
    }
    static void write16(ofstream &out, uint16_t v)
    {
        out.put((v >> 8) & 0xFF);
        out.put(v & 0xFF);
    }
};

#endif // _JPEGWRITER_HPP_
