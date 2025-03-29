#include "class/imageExtension/PPMImage.cpp"
#include "class/JPEGCompressor.hpp"

using namespace std;

int main()
{

    string fileName = "Clown.ppm";
    string outputFileName = "output.ppm";

    PPMImage ppm = PPMImage();
    if (ppm.load(fileName))
    {
        cout << "Image size: (" << ppm.getWidth() << ", " << ppm.getHeight() << ")" << endl;
    }


        return 0;
}