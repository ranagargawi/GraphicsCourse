#include <stb/stb_image.h>

#include <stb/stb_image_write.h>
#include <cmath>
#include <iostream>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Debugger.h>
#include <VertexBuffer.h>
#include <VertexBufferLayout.h>
#include <IndexBuffer.h>
#include <VertexArray.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#include <iostream>
#define RED 0.299
#define GREEN 0.587
#define BLUE 0.114
#define BLACK 255
#define WHITE 0
#define NON_RELEVANT 0
#define T_WEAK 1
#define T_STRONG 2
#define CANNY 0.25
#define M_PI 3.14159265358979323846
#define COMPRESSED 16
// compressed grayscale
#define ALPHA 7 / 16.0
#define BETA 3 / 16.0
#define GAMMA 5 / 16.0
#define DELTA 1 / 16.0
unsigned char *fsErrorDiffDithering(unsigned char *buffer, int width, int height, float a, float b, float c, float d);
unsigned char trunc(unsigned char p);
unsigned char *canny(unsigned char *buffer, int width, int height, float scale, float lower, float upper);
int doubleThreshhlding(unsigned char pixel, int low, int high);

float *convolution(float *buffer, float *newBuffer, int width, int height, float *kernel, int kwidth, int kheight, float norm);
void applyKernel(float *buffer, float *padded, int width, int x, int y, float *kernel, int kernelWidth, int kernelHeight, float norm);
float pixelClip(float pixel);
unsigned char *greyScale(unsigned char *input, int length, float red, float green, float blue);
unsigned char *halftone(unsigned char *buffer, int width, int height);
int main(int argc, char *argv[])
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char *buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char *greyBuffer = greyScale(buffer, width * height, RED, GREEN, BLUE);
    int result = stbi_write_png("res/textures/Grayscale.png", width, height, 1, greyBuffer, width);
    unsigned char *cannyBuffer = canny(greyBuffer, width, height, CANNY, 0.1, 0.15);
    result = result + stbi_write_png("res/textures/Canny.png", width, height, 1, cannyBuffer, width);
    unsigned char *fsBuffer = fsErrorDiffDithering(greyBuffer, width, height, ALPHA, BETA, GAMMA, DELTA);
    result += stbi_write_png("res/textures/FloyedSteinberg.png", width, height, 1, fsBuffer, width);
    unsigned char *halftoneBuff = halftone(greyBuffer, width, height);
    result += stbi_write_png("res/textures/Halftone.png", width * 2, height * 2, 1, halftoneBuff, width * 2);

    std::cout << result << std::endl;
    delete[] buffer;
    delete[] greyBuffer;
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

    int kerelheight = 3;
    int kernelwidth = 3;
    int h = (kerelheight - 1) / 2;
    int w = (kernelwidth - 1) / 2;
    int *strenght = new int[width * height];
    float *angles = new float[width * height];
    float *blurredImage = new float[width * height];
    float *xConv = new float[width * height];
    float *yConv = new float[width * height];
    unsigned char *gradients = new unsigned char[width * height];
    unsigned char *outlines = new unsigned char[width * height];
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
    convolution(fBuffer, blurredImage, width, height, gaussian, kernelwidth, kerelheight, 1.0 / 16);

    // finding gradient and angles..
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (i < h || i > height - 1 - h || j < w || j > width - 1 - w)
            {
                gradients[j + i * width] = BLACK;
                angles[j + i * width] = BLACK;
                continue;
            }
            applyKernel(blurredImage, xConv, width, j, i, xSobel, kernelwidth, kerelheight, scale);
            applyKernel(blurredImage, yConv, width, j, i, ySobel, kernelwidth, kerelheight, scale);
            gradients[i * width + j] = pixelClip(std::sqrt(xConv[i * width + j] * xConv[i * width + j] + yConv[i * width + j] * yConv[i * width + j]));
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
                outlines[j + i * width] = BLACK;
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

            currPixel = gradients[j + i * width];
            posPixel = gradients[j + vecXSign + (i + vecYSign) * width];
            negPixel = gradients[j - vecXSign + (i - vecYSign) * width];

            if (posPixel < currPixel && negPixel < currPixel)
            {
                outlines[j + (i)*width] = currPixel;
            }
            else
            {
                outlines[j + i * width] = BLACK;
            }

            // Double threashholding
            strenght[j + i * width] = doubleThreshhlding(outlines[j + i * width], lower * WHITE, upper * WHITE);
        }
    } // Hysteresis
    for (int i = h; i < height - h; i++)
    {
        for (int j = w; j < width - w; j++)
        {
            if (strenght[j + i * width] == NON_RELEVANT)
                outlines[j + i * width] = BLACK;
            if (strenght[j + i * width] == T_STRONG)
                outlines[j + i * width] = WHITE;
            if (strenght[j + i * width] == T_WEAK)
            {
                if (strenght[j - 1 + (i - 1) * width] == T_STRONG ||
                    strenght[j + (i - 1) * width] == T_STRONG ||
                    strenght[j + 1 + (i - 1) * width] == T_STRONG ||
                    strenght[j - 1 + (i)*width] == T_STRONG ||
                    strenght[j + 1 + (i)*width] == T_STRONG ||
                    strenght[j - 1 + (i + 1) * width] == T_STRONG ||
                    strenght[j + (i + 1) * width] == T_STRONG ||
                    strenght[j + 1 + (i + 1) * width] == T_STRONG)
                {
                    outlines[j + i * width] = WHITE;
                    strenght[j + i * width] = T_STRONG;
                }
                else
                    outlines[j + i * width] = BLACK;
            }
        }
    }

    delete[] xConv;
    delete[] yConv;
    delete[] gradients;
    delete[] strenght;
    delete[] angles;
    delete[] blurredImage;
    return outlines;
}
int doubleThreshhlding(unsigned char p, int lower, int upper)
{
    if (p < lower)
        return NON_RELEVANT;
    else if (p >= lower && p <= upper)
        return T_WEAK;
    else
        return T_STRONG;
}

// working but only give convolution
float *convolution(float *buffer, float *newBuffer, int width, int height, float *kernel, int kernelwidth, int kernelheight, float norm)
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
        for (int j = 0; j < kernelwidth; j++)
        {
            newBuffer[j + i * width] = buffer[j + i * width];
            newBuffer[width - 1 - j + i * width] = buffer[width - 1 - j + i * width];
        }
    }

    for (int i = (kernelheight - 1) / 2; i < height - (kernelheight - 1) / 2; i++)
    {
        for (int j = (kernelwidth - 1) / 2; j < width - (kernelheight - 1) / 2; j++)
        {
            applyKernel(buffer, newBuffer, width, j, i, kernel, kernelwidth, kernelheight, norm);
        }
    }
    return newBuffer;
}

void applyKernel(float *buffer, float *newBuffer, int width, int x, int y, float *kernel, int kwidth, int kheight, float norm)
{
    float sum = 0;
    for (int i = 0; i < kheight; i++)
    {
        for (int j = 0; j < kwidth; j++)
        {
            sum += buffer[x - (kwidth - 1) / 2 + j + (y - (kheight - 1) / 2 + i) * width] * kernel[j + (i)*kwidth];
        }
    }
    newBuffer[x + y * width] = sum * norm;
}

float pixelClip(float p)
{
    if (p > WHITE)
        return WHITE;
    if (p < BLACK)
        return BLACK;
    return p;
}

unsigned char *fsErrorDiffDithering(unsigned char *buffer, int width, int height, float a, float b, float c, float d)
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
    int length = width * height;
    unsigned char *result = new unsigned char[length * 4];
    for (int i = 0; i < length; i++)
    {
        int row = i / width;
        int col = i % width;
        if (buffer[i] < WHITE / 5)
        {
            result[row * 2 * 2 * width + col * 2] = BLACK;
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        }
        else if (buffer[i] >= WHITE / 5 && buffer[i] < WHITE / 5 * 2)
        {
            result[row * 2 * 2 * width + col * 2] = BLACK;
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        }
        else if (buffer[i] >= WHITE / 5 * 2 && buffer[i] < WHITE / 5 * 3)
        {
            result[row * 2 * 2 * width + col * 2] = WHITE;
            result[row * 2 * 2 * width + col * 2 + 1] = BLACK;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = BLACK;
        }
        else if (buffer[i] >= WHITE / 5 * 3 && buffer[i] < WHITE / 5 * 4)
        {
            result[row * 2 * 2 * width + col * 2] = BLACK;
            result[row * 2 * 2 * width + col * 2 + 1] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = WHITE;
        }
        else if (buffer[i] >= WHITE / 5 * 4)
        {
            result[row * 2 * 2 * width + col * 2] = WHITE;
            result[row * 2 * 2 * width + col * 2 + 1] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2] = WHITE;
            result[(row * 2 + 1) * 2 * width + col * 2 + 1] = WHITE;
        }
    }
    return result;
}
