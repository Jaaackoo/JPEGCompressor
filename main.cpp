#include "src/class/imageExtension/PPMImage.cpp"

using namespace std;

int main()
{

    PPMImage ppm = PPMImage();
    if (ppm.load("Gris.ppm"))
    {
        cout << "Image size: (" << ppm.getWidth() << ", " << ppm.getHeight() << ")" << endl;
    } else {
        cerr << "Error in loading of image" << endl;
    }

    for(Pixel pixel : ppm.getPixels()) {
        cout << "RGB(" << pixel.R << ", " << pixel.G << ", " << pixel.B << ")" << endl;
    }

    ppm.save("Grisoutput.ppm");

    return 0;
}