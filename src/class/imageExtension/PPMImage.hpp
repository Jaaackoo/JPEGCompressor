#ifndef _PPM_HPP_
#define _PPM_HPP_

#include "../Image.hpp"

class PPMImage : public Image {
    public:
        bool load(const string& path) override;
        bool save(const string& path) override;
    };

#endif