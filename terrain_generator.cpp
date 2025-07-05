#include "mca_generator.h"
#include <cmath>
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
    World world;

    const int width = 512*2, depth = 512*2, height_limit = 32;
    const float scale = 0.004f;
    const int sea_level = 53, forrest_line = 90;

    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            float h = fbm(x * scale, z * scale, 5);
            int height = (int)(h * height_limit) + 32;
            int biome_offset = fbm(x * 0.02 + 12423, z * 0.02, 2);

            auto top_block = (height > sea_level + biome_offset * 4) ? block::grass_block : block::sand;
            top_block = (height > forrest_line + biome_offset * 10) ? block::stone : top_block;

            auto below_surface_block = (top_block == block::grass_block) ? block::dirt : top_block;
            for (int y = 0; y <= std::max(height, sea_level); y++) {
                if (y > height)
                    world.setBlock(block::water, x, y, z);
                else if (y == height)
                    world.setBlock(top_block, x, y, z);
                else if (y > height-3)
                    world.setBlock(below_surface_block, x, y, z);
                else
                    world.setBlock(block::stone, x, y, z);
            }
            int biome = 1;
            if (height > sea_level + biome_offset * 4) biome = 0;
            if (height > forrest_line - 5 + biome_offset * 10) biome = 0;
            world.setBiomeColumn(x, z, 0, 255, biome);
        }
    }

    world.save();
    std::cout << "Saved perlin terrain\n";
    return 0;
}
