#ifndef _PPMIMAGE_HPP_
#define _PPMIMAGE_HPP_

#include "../Image.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

class PPMImage : public Image
{
protected:
    int maxVal;
    string fileType;

private:
    bool loadP3(ifstream &fileName);
    bool loadP6(ifstream &fileName);
    bool saveP3(const string &filename);
    bool saveP6(const string &filename);

public:
    bool load(const string &fileName) override;
    bool save(const string &fileName) override;
    void setMaxVal(int maxVal);
    void setFileType(string fileType);
};

#endif