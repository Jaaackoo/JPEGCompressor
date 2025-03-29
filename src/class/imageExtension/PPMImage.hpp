#ifndef _PPM_HPP_
#define _PPM_HPP_

#include "../Image.hpp"
#include <fstream>
#include <sstream>
#include <iostream>


class PPMImage : public Image {
    protected:
        int maxVal;
    public:
        bool load(const string& fileName) override;
        bool save(const string& fileName) override;
    };

#endif