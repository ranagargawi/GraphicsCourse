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

/* Window size */
const unsigned int width = 800;
const unsigned int height = 800;
// const float FOVdegree = 45.0f;  // Field Of View Angle
const float near = 0.1f;
const float far = 100.0f;

/* Shape vertices coordinates with positions, colors, and corrected texCoords */
float vertices[] = {
    // positions            // colors            // texCoords
    -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Bottom-left
    0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // Bottom-right
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // Top-right
    -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // Top-left
};

/* Indices for vertices order */
unsigned int indices[] = {
    0, 1, 2,
    2, 3, 0};
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