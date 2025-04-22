#ifndef _UTILS_HPP
#define _UTILS_HPP
#include "../class/Image.hpp"

using namespace std;

void test_YcrCb(Image *img);
void test_splitYToBlocks(Image *img);
void test_DCT(Image *img);
void test_quantification(Image *img);
array<int, 64UL> test_zigzag(Image *img);

#endif