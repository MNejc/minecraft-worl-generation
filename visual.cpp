#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>

float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

float lerp(float a, float b, float t) { return a + t * (b - a); }

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float perlin(float x, float y, int seed = 5) {
    int xi = (int)std::floor(x) & 255;
    int yi = (int)std::floor(y) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);

    float u = fade(xf);
    float v = fade(yf);

    static uint8_t p[512];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 256; i++) p[i] = i;
        std::shuffle(p, p + 256, std::mt19937(seed));
        for (int i = 0; i < 256; i++) p[256 + i] = p[i];
        initialized = true;
    }

    int aa = p[p[xi] + yi];
    int ab = p[p[xi] + yi + 1];
    int ba = p[p[xi + 1] + yi];
    int bb = p[p[xi + 1] + yi + 1];

    float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
    float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);

    return (lerp(x1, x2, v) + 1.0f) * 0.5f; // normalize 0..1
}

float fbm(float x, float y, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) {
    float total = 0.0f, amplitude = 1.0f, frequency = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        total += perlin(x * frequency, y * frequency) * amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    return total / ((1.0f - std::pow(gain, octaves)) / (1.0f - gain));
}


int main() {
    const int width = 512 * 2, height = 512 * 2;
    const std::string filename = "heightmap.png";
    std::vector<std::vector<unsigned char>> pixels(height, std::vector<unsigned char>(width));
    const float scale = 0.002f;
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float h = fbm(x * scale, z * scale, 5);
            pixels[z][x] = h*255; // Grayscale value
        }
    }

    std::vector<unsigned char> image_data(width * height * 3); // 3 channels (RGB)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 3; // Index for RGB
            image_data[index] = 0;  // Red channel = 0
            image_data[index + 1] = (pixels[y][x] > 150) ? pixels[y][x] : 0; // Green channel = original value
            image_data[index + 2] = (pixels[y][x] > 150) ? 0 : pixels[y][x];;       // Blue channel = 0
        }
    }

    // Save as PNG
    if (stbi_write_png(filename.c_str(), width, height, 3, image_data.data(), width * 3) == 0) {
        std::cerr << "Failed to save " << filename << std::endl;
        return 1;
    }

    std::cout << "Saved heightmap to " << filename << std::endl;

    return 0;
}
