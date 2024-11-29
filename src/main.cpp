// if you wish to run with makefile extantion, try removinf the 2 defines below.

// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <cmath>
#include <iostream>

// gray scale weights
#define RED_WEIGHT 0.2989
#define GREEN_WEIGHT 0.5870
#define BLUE_WEIGHT 0.1140

// canny constants
#define WHITE 255
#define BLACK 0
#define NON_RELEVANT 0
#define WEAK 1
#define STRONG 2
#define CANNY 0.25
#define M_PI 3.14159265358979323846

// Floyed-Steinberg dithering
#define COMPRESSED 16 // compressed grayscale
#define ALPHA 7 / 16.0
#define BETA 3 / 16.0
#define GAMMA 5 / 16.0
#define DELTA 1 / 16.0

unsigned char *greyScale(unsigned char *buffer, int length, float gw, float rw, float bw);
unsigned char *canny(unsigned char *buffer, int width, int height, float scale, float lower, float upper);
float *convolution(float *buffer, float *newBuffer, int width, int height, float *kernel, int kwidth, int kheight, float norm);
void applyKernel(float *buffer, float *newBuffer, int width, int x, int y, float *kernel, int kwidth, int kheight, float norm);
float clippedPixel(float p);
int doubleThreshhldingPixel(unsigned char p, int lower, int upper);
unsigned char *halftone(unsigned char *buffer, int width, int height);
unsigned char *floyedSteinberg(unsigned char *buffer, int width, int height, float a, float b, float c, float d);
unsigned char trunc(unsigned char p);
int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char *original = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char *greyscaleBuffer = greyScale(original, width * height, RED_WEIGHT, GREEN_WEIGHT, BLUE_WEIGHT);
    stbi_write_png("res/textures/GrayScale.png", width, height, 1, greyscaleBuffer, width);

    unsigned char *cannyBuffer = canny(greyscaleBuffer, width, height, CANNY, 0.1, 0.15);
    stbi_write_png("res/textures/Canny.png", width, height, 1, cannyBuffer, width);
    unsigned char *halftoneBuff = halftone(greyscaleBuffer, width, height);
    stbi_write_png("res/textures/HalfTone.png", width * 2, height * 2, 1, halftoneBuff, width * 2);

    unsigned char *fsBuffer = floyedSteinberg(greyscaleBuffer, width, height, ALPHA, BETA, GAMMA, DELTA);
    stbi_write_png("res/textures/FloyedSteinberg.png", width, height, 1, fsBuffer, width);
    std::cout << ">>   Where do I find the pictures after the filters?  << " << std::endl;
    std::cout << ">>   GrayScale: bin/res/textures/GrayScale.png   <<" << std::endl;
    std::cout << ">>   Canny: bin/res/textures/Canny.png   <<" << std::endl;
    std::cout << ">>   HalfTone: bin/res/textures/HalfTone.png    <<" << std::endl;
    std::cout << ">>   FloyedSteinberg: bin/res/textures/FloyedSteinberg.pn   <<" << std::endl;

    delete[] original;
    delete[] greyscaleBuffer;
    delete[] cannyBuffer;
    delete[] halftoneBuff;
    delete fsBuffer;
    return 0;
}

unsigned char *greyScale(unsigned char *input, int length, float red, float green, float blue)
{
    unsigned char *result = new unsigned char[length];
    for (int i = 0; i < length; i++)
        result[i] = input[i * 4] * red + input[i * 4 + 1] * green + input[i * 4 + 2] * blue;

    return result;
}
//
unsigned char *canny(unsigned char *buffer, int width, int height, float scale, float lower, float upper)
{
    float xSobel[] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
    float ySobel[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
    float gaussian[] = {1, 2, 1, 2, 4, 2, 1, 2, 1};

    int kernelheight = 3;
    int kerenlwidth = 3;
    int h = (kernelheight - 1) / 2;
    int w = (kerenlwidth - 1) / 2;
    int *strength = new int[width * height];
    float *angles = new float[width * height];
    float *blurred = new float[width * height];
    float *xConv = new float[width * height];
    float *yConv = new float[width * height];
    unsigned char *gradient = new unsigned char[width * height];
    unsigned char *outlined = new unsigned char[width * height];
    float currAngel = 0;
    int vecXSign = 0;
    int vecYSign = 0;
    unsigned char currPixel = 0;
    unsigned char posPixel = 0;
    unsigned char negPixel = 0;

    float *fBuffer = new float[width * height];
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            fBuffer[j + i * width] = buffer[j + i * width];
        }
    }
    // reducing noise
    convolution(fBuffer, blurred, width, height, gaussian, kerenlwidth, kernelheight, 1.0 / 16);

    // finding gradient and angels
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (i < h || i > height - 1 - h || j < w || j > width - 1 - w)
            {
                gradient[j + i * width] = BLACK;
                angles[j + i * width] = BLACK;
                continue;
            }
            applyKernel(blurred, xConv, width, j, i, xSobel, kerenlwidth, kernelheight, scale);
            applyKernel(blurred, yConv, width, j, i, ySobel, kerenlwidth, kernelheight, scale);
            gradient[i * width + j] = clippedPixel(std::sqrt(xConv[i * width + j] * xConv[i * width + j] + yConv[i * width + j] * yConv[i * width + j]));
            angles[j + i * width] = std::atan2(yConv[j + i * width], xConv[j + i * width]) * (180 / M_PI);
        }
    }

    // Non-max suppresion
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (i < h || i > height - 1 - h || j < w || j > width - 1 - w)
            {
                outlined[j + i * width] = BLACK;
                continue;
            }
            currAngel = angles[j + i * width] < 0 ? angles[j + i * width] + 360 : angles[j + i * width];

            // 0 degrees
            if ((currAngel > 337.5 || currAngel <= 22.5) || (currAngel > 157.5 && currAngel <= 202.5))
            {
                vecXSign = 1;
                vecYSign = 0;
            }
            // 45 degrees
            else if ((currAngel > 22.5 && currAngel <= 67.5) || (currAngel > 202.5 && currAngel <= 247.5))
            {
                vecXSign = 1;
                vecYSign = 1;
            }
            // 90 degrees
            else if ((currAngel > 67.5 && currAngel <= 112.5) || (currAngel > 247.5 && currAngel <= 292.5))
            {
                vecXSign = 0;
                vecYSign = 1;
            }
            // 135 degrees
            else if ((currAngel > 112.5 && currAngel <= 157.5) || (currAngel > 292.5 && currAngel <= 337.5))
            {
                vecXSign = -1;
                vecYSign = 1;
            }

            currPixel = gradient[j + i * width];
            posPixel = gradient[j + vecXSign + (i + vecYSign) * width];
            negPixel = gradient[j - vecXSign + (i - vecYSign) * width];

            if (posPixel < currPixel && negPixel < currPixel)
            {
                outlined[j + (i)*width] = currPixel;
            }
            else
            {
                outlined[j + i * width] = BLACK;
            }

            // Double threashholding
            strength[j + i * width] = doubleThreshhldingPixel(outlined[j + i * width], lower * WHITE, upper * WHITE);
        }
    }

    // Hysteresis
    for (int i = h; i < height - h; i++)
    {
        for (int j = w; j < width - w; j++)
        {
            if (strength[j + i * width] == NON_RELEVANT)
                outlined[j + i * width] = BLACK;
            if (strength[j + i * width] == STRONG)
                outlined[j + i * width] = WHITE;
            if (strength[j + i * width] == WEAK)
            {
                if (strength[j - 1 + (i - 1) * width] == STRONG ||
                    strength[j + (i - 1) * width] == STRONG ||
                    strength[j + 1 + (i - 1) * width] == STRONG ||
                    strength[j - 1 + (i)*width] == STRONG ||
                    strength[j + 1 + (i)*width] == STRONG ||
                    strength[j - 1 + (i + 1) * width] == STRONG ||
                    strength[j + (i + 1) * width] == STRONG ||
                    strength[j + 1 + (i + 1) * width] == STRONG)
                {
                    outlined[j + i * width] = WHITE;
                    strength[j + i * width] = STRONG;
                }
                else
                    outlined[j + i * width] = BLACK;
            }
        }
    }

    delete[] xConv;
    delete[] yConv;
    delete[] gradient;
    delete[] strength;
    delete[] angles;
    delete[] blurred;
    return outlined;
}

int doubleThreshhldingPixel(unsigned char pixel, int lower, int upper)
{
    if (pixel < lower)
        return NON_RELEVANT;
    else if (pixel >= lower && pixel <= upper)
        return WEAK;
    else
        return STRONG;
}

float *convolution(float *buffer, float *newBuffer, int width, int height, float *kernel, int kwidth, int kernelheight, float norm)
{
    for (int i = 0; i < kernelheight; i++)
    {
        for (int j = 0; j < width; j++)
        {
            newBuffer[j + i * width] = buffer[j + i * width];
            newBuffer[j + (height - 1 - i) * width] = buffer[j + (height - 1 - i) * width];
        }
    }
    for (int i = kernelheight; i < height; i++)
    {
        for (int j = 0; j < kwidth; j++)
        {
            newBuffer[j + i * width] = buffer[j + i * width];
            newBuffer[width - 1 - j + i * width] = buffer[width - 1 - j + i * width];
        }
    }

    for (int i = (kernelheight - 1) / 2; i < height - (kernelheight - 1) / 2; i++)
    {
        for (int j = (kwidth - 1) / 2; j < width - (kernelheight - 1) / 2; j++)
        {
            applyKernel(buffer, newBuffer, width, j, i, kernel, kwidth, kernelheight, norm);
        }
    }
    return newBuffer;
}

void applyKernel(float *buffer, float *newBuffer, int width, int x, int y, float *kernel, int kernelwidth, int kernelheight, float norm)
{
    float avg = 0;
    for (int i = 0; i < kernelheight; i++)
    {
        for (int j = 0; j < kernelwidth; j++)
        {
            avg += buffer[x - (kernelwidth - 1) / 2 + j + (y - (kernelheight - 1) / 2 + i) * width] * kernel[j + (i)*kernelwidth];
        }
    }
    newBuffer[x + y * width] = avg * norm;
}

float clippedPixel(float pixel)
{
    if (pixel > WHITE)
        return WHITE;
    if (pixel < BLACK)
        return BLACK;
    return pixel;
}

unsigned char *floyedSteinberg(unsigned char *buffer, int width, int height, float a, float b, float c, float d)
{
    unsigned char *diffused = new unsigned char[width * height]; // copy of image with diffused error
    unsigned char *result = new unsigned char[width * height];

    diffused[0] = buffer[0];
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            result[i * width + j] = trunc(diffused[i * width + j]);
            char error = buffer[i * width + j] - result[i * width + j];
            diffused[i * width + j + 1] = buffer[i * width + j + 1] + error * a;
            if (i < height - 1)
            {
                diffused[(i + 1) * width + j - 1] = buffer[(i + 1) * width + j - 1] + error * b;
                diffused[(i + 1) * width + j] = buffer[(i + 1) * width + j] + error * c;
                diffused[(i + 1) * width + j + 1] = buffer[(i + 1) * width + j + 1] + error * d;
            }
        }
    }
    delete[] diffused;
    return result;
}

unsigned char trunc(unsigned char pixel)
{
    // maps 0-255 into one of COMPRESSED_GS values spaced
    return (unsigned char)(pixel / 255.0 * COMPRESSED) / (float)COMPRESSED * 255.0;
}

unsigned char *halftone(unsigned char *buffer, int width, int height)
{
    const int length = width * height;
    unsigned char *result = new unsigned char[length * 4];
    const int resultWidth = width * 2;

    for (int i = 0; i < length; ++i)
    {
        int row = i / width;
        int col = i % width;
        int resultRowBase = row * 2 * resultWidth;
        int resultCol = col * 2;

        unsigned char pixelValue = buffer[i];
        if (pixelValue < WHITE / 5)
        {
            result[resultRowBase + resultCol] = BLACK;
            result[resultRowBase + resultCol + 1] = BLACK;
            result[resultRowBase + resultWidth + resultCol] = BLACK;
            result[resultRowBase + resultWidth + resultCol + 1] = BLACK;
        }
        else if (pixelValue < WHITE / 5 * 2)
        {
            result[resultRowBase + resultCol] = BLACK;
            result[resultRowBase + resultCol + 1] = BLACK;
            result[resultRowBase + resultWidth + resultCol] = WHITE;
            result[resultRowBase + resultWidth + resultCol + 1] = BLACK;
        }
        else if (pixelValue < WHITE / 5 * 3)
        {
            result[resultRowBase + resultCol] = WHITE;
            result[resultRowBase + resultCol + 1] = BLACK;
            result[resultRowBase + resultWidth + resultCol] = WHITE;
            result[resultRowBase + resultWidth + resultCol + 1] = BLACK;
        }
        else if (pixelValue < WHITE / 5 * 4)
        {
            result[resultRowBase + resultCol] = BLACK;
            result[resultRowBase + resultCol + 1] = WHITE;
            result[resultRowBase + resultWidth + resultCol] = WHITE;
            result[resultRowBase + resultWidth + resultCol + 1] = WHITE;
        }
        else
        {
            result[resultRowBase + resultCol] = WHITE;
            result[resultRowBase + resultCol + 1] = WHITE;
            result[resultRowBase + resultWidth + resultCol] = WHITE;
            result[resultRowBase + resultWidth + resultCol + 1] = WHITE;
        }
    }
    return result;
}
