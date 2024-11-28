#include <stb/stb_image.h>

#include <stb/stb_image_write.h>
#include <cmath>
#include <iostream>

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
unsigned char *canny(unsigned char *buffer, int width, int height, float scale, float lower, float upper);
float *convolution(float *buffer, float *newBuffer, int width, int height, float *kernel, int kwidth, int kheight, float norm);

unsigned char *greyScale(unsigned char *input, int length, float red, float green, float blue);
int main(int argc, char *argv[])
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char *buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char *greyBuffer = greyScale(buffer, width * height, RED, GREEN, BLUE);
    int result = stbi_write_png("res/textures/Grayscale.png", width, height, 1, greyBuffer, width);

    delete[] buffer;
    delete[] greyBuffer;
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
    float gaussian[] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
    int kernelHeight = 3;
    int kernelWidth = 3;
    int h = (kernelHeight - 1) / 2;
    int w = (kernelWidth - 1) / 2;
    float *blurred = new float[width * height];

    float *padded = new float[width * height];
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            padded[j + i * width] = buffer[j + i * width];
        }
    }

    convolution(padded, blurred, width, height, gaussian, kernelWidth, kernelHeight, 1.0 / 16);
}
float *convolution(float *buffer, float *padded, int width, int height, float *kernel, int kernelWidth, int kernelHeight, float norm)
{
    for (int i = 0; i < kernelHeight; i++)
    {
        for (int j = 0; j < width; j++)
        {
            padded[j + i * width] = buffer[j + i * width];
            padded[j + (height - 1 - i) * width] = buffer[j + (height - 1 - i) * width];
        }
    }
    for (int i = kernelHeight; i < height; i++)
    {
        for (int j = 0; j < kernelWidth; j++)
        {
            padded[j + i * width] = buffer[j + i * width];
            padded[width - 1 - j + i * width] = buffer[width - 1 - j + i * width];
        }
    }

    for (int i = (kernelHeight - 1) / 2; i < height - (kernelHeight - 1) / 2; i++)
    {
        for (int j = (kernelWidth - 1) / 2; j < width - (kernelHeight - 1) / 2; j++)
        {
            applyKernel(buffer, padded, width, j, i, kernel, kernelWidth, kernelHeight, norm);
        }
    }

    return padded;
}

void applyKernel(float *buffer, float *padded, int width, int x, int y, float *kernel, int kernelWidth, int kernelHeight, float norm)
{
    float avg = 0;
    for (int i = 0; i < kernelHeight; i++)
    {
        for (int j = 0; j < kernelWidth; j++)
        {
            avg += buffer[x - (kernelWidth - 1) / 2 + j + (y - (kernelHeight - 1) / 2 + i) * width] * kernel[j + (i)*kernelWidth];
        }
    }
    padded[x + y * width] = avg * norm;
}
