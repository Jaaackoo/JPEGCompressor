#include "src/class/imageExtension/PPMImage.cpp"

using namespace std;

int main()
{

    string fileName = "Clown.ppm";
    string outputFileName = "output.ppm";

    PPMImage ppm = PPMImage();
    if (ppm.load(fileName))
    {
        cout << "Image size: (" << ppm.getWidth() << ", " << ppm.getHeight() << ")" << endl;
    } else {
        cerr << "Error in loading of image" << endl;
    }

    ppm.save(outputFileName);

    return 0;
}