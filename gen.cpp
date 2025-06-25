#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <random>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <zlib.h>
#include <cassert>
#include <cmath>

// Represents a Minecraft block (namespace and ID).
struct Block {
    std::string ns, id;
    Block(const std::string &ns_, const std::string &id_) : ns(ns_), id(id_) {}
    // Full name "namespace:id"
    std::string name() const { return ns + ":" + id; }
};

// Represents a 16×16×16 section of blocks at height Y.
// Blocks are stored in XZY order: index = y*256 + z*16 + x.
struct Section {
    int y;                       // Section index (0=Y=0..15)
    std::array<Block*, 4096> blocks;
    Block* air;

    Section(int y_) : y(y_) {
        blocks.fill(nullptr);
        air = new Block("minecraft", "air");
    }

    // Set a block pointer at (x,z) in [0,15] and y in [0,15] within this section.
    void setBlock(Block* block, int x, int yy, int z) {
        if (x<0||x>15||yy<0||yy>15||z<0||z>15) {
            std::cerr << "Section setBlock out of bounds\n"; exit(1);
        }
        int idx = yy*256 + z*16 + x;
        blocks[idx] = block;
    }

    // Gather unique blocks (including air if nullptr found).
    std::vector<Block*> palette() const {
        std::vector<Block*> p;
        for (Block* b : blocks) {
            if (b && std::find(p.begin(), p.end(), b) == p.end()) {
                p.push_back(b);
            }
        }
        // If any position is nullptr, include air as a palette entry.
        if (std::find(blocks.begin(), blocks.end(), nullptr) != blocks.end()) {
            p.push_back(air);
        }
        // If only air is present, ensure palette = {air}.
        if (p.empty()) {
            p.push_back(air);
        }
        return p;
    }

    // Compute packed block states as uint64 array (4 bits per block minimum).
    std::vector<uint64_t> blockStates(const std::vector<Block*>& pal) const {
        int palSize = (int)pal.size();
        // Compute bits needed (at least 4)
        int bits = std::max((palSize - 1) > 0 ? 32 - __builtin_clz(palSize - 1) : 0, 4);
        std::vector<uint64_t> states;
        uint64_t current = 0;
        int curLen = 0;
        // Map blocks to indices in palette.
        for (int i = 0; i < 4096; i++) {
            Block* b = blocks[i];
            int idx;
            if (!b) {
                // air index
                idx = std::find(pal.begin(), pal.end(), air) - pal.begin();
            } else {
                auto it = std::find(pal.begin(), pal.end(), b);
                idx = (int)(it - pal.begin());
            }
            // Pack into 64-bit words
            if (curLen + bits > 64) {
                int leftover = 64 - curLen;
                uint64_t lowmask = (1ULL << leftover) - 1;
                uint64_t lowbits = idx & lowmask;
                // Append word
                states.push_back(current | (lowbits << curLen));
                current = (uint64_t)idx >> leftover;
                curLen = bits - leftover;
            } else {
                current |= (uint64_t)idx << curLen;
                curLen += bits;
            }
        }
        states.push_back(current);
        return states;
    }
};

// Represents one chunk at (cx, cz) relative to region, with up to 16 sections.
struct Chunk {
    int cx, cz;
    std::array<Section*, 16> sections;
    int version = 2566;  // DataVersion (matches Python default)

    Chunk(int cx_, int cz_) : cx(cx_), cz(cz_) {
        sections.fill(nullptr);
    }

    // Set block at local chunk coords (0..15) and global y (0..255).
    void setBlock(Block* block, int x, int y, int z) {
        if (x<0||x>15||z<0||z>15||y<0||y>255) {
            std::cerr << "Chunk setBlock out of bounds\n"; exit(1);
        }
        int secY = y / 16;
        if (!sections[secY]) {
            sections[secY] = new Section(secY);
        }
        sections[secY]->setBlock(block, x, y - secY*16, z);
    }

    // Generate the NBT byte blob for this chunk (uncompressed).
    std::vector<uint8_t> toNBT() const {
        std::vector<uint8_t> data;
        // Helper lambdas for writing big-endian primitives
        auto put_u8 = [&](uint8_t v){ data.push_back(v); };
        auto put_u16 = [&](uint16_t v){
            data.push_back((v>>8)&0xFF);
            data.push_back(v&0xFF);
        };
        auto put_u32 = [&](uint32_t v){
            data.push_back((v>>24)&0xFF);
            data.push_back((v>>16)&0xFF);
            data.push_back((v>>8)&0xFF);
            data.push_back(v&0xFF);
        };
        auto put_u64 = [&](uint64_t v){
            for(int i=7;i>=0;i--) data.push_back((v>>(8*i))&0xFF);
        };
        auto put_str = [&](const std::string &s){
            put_u16((uint16_t)s.size());
            data.insert(data.end(), s.begin(), s.end());
        };

        // Start root compound
        put_u8(10);     // TAG_Compound
        put_u16(0);     // name length = 0 (empty)
        // TAG_Int "DataVersion"
        put_u8(3);
        put_str("DataVersion");
        put_u32((uint32_t)version);
        // TAG_Compound "Level"
        put_u8(10);
        put_str("Level");
        // Inside Level:
        // Empty list "Entities"
        put_u8(9);
        put_str("Entities");
        put_u8(10);     // element type = Compound
        put_u32(0);     // length 0
        // Empty list "TileEntities"
        put_u8(9);
        put_str("TileEntities");
        put_u8(10);
        put_u32(0);
        // Empty list "LiquidTicks"
        put_u8(9);
        put_str("LiquidTicks");
        put_u8(10);
        put_u32(0);
        // xPos
        put_u8(3);
        put_str("xPos");
        put_u32((uint32_t)cx);
        // zPos
        put_u8(3);
        put_str("zPos");
        put_u32((uint32_t)cz);
        // LastUpdate (long)
        put_u8(4);
        put_str("LastUpdate");
        put_u64(0);
        // InhabitedTime (long)
        put_u8(4);
        put_str("InhabitedTime");
        put_u64(0);
        // isLightOn (byte)
        put_u8(1);
        put_str("isLightOn");
        put_u8(1);
        // Status (string "full")
        put_u8(8);
        put_str("Status");
        put_str("full");
        // Sections list
        // Count how many non-air sections
        std::vector<Section*> present;
        for (Section* s : sections) {
            if (s) {
                // Determine if section is all air
                auto pal = s->palette();
                if (!(pal.size()==1 && pal[0]->name()=="minecraft:air")) {
                    present.push_back(s);
                }
            }
        }
        put_u8(9);
        put_str("Sections");
        put_u8(10);    // element type = Compound
        put_u32((uint32_t)present.size());
        // For each present section, write its compound (no name in list)
        for (Section* s : present) {
            // TAG_Byte Y
            put_u8(1);
            put_str("Y");
            put_u8((uint8_t)s->y);
            // TAG_List Palette (Compound)
            put_u8(9);
            put_str("Palette");
            put_u8(10);
            auto pal = s->palette();
            put_u32((uint32_t)pal.size());
            for (Block* b : pal) {
                // Each palette element is a TAG_Compound (element type known, so no ID here)
                // Inside it: TAG_String "Name"
                put_u8(8);
                put_str("Name");
                put_str(b->name());
                // No properties for stone/dirt
                // End of this compound
                put_u8(0);
            }
            // TAG_Long_Array BlockStates
            put_u8(12);
            put_str("BlockStates");
            auto states = s->blockStates(pal);
            put_u32((uint32_t)states.size());
            for (uint64_t v : states) {
                put_u64(v);
            }
            // End of section compound
            put_u8(0);
        }
        // End Level compound
        put_u8(0);
        // End root compound
        put_u8(0);
        return data;
    }
};

// Represents a 32×32 chunk region at region coordinates (rx,rz).
struct Region {
    int rx, rz;
    std::array<Chunk*, 1024> chunks;  // row-major: index = cz*32 + cx
    Region(int rx_, int rz_) : rx(rx_), rz(rz_) {
        chunks.fill(nullptr);
    }
    // Helper: compute 1D index from chunk coords (cx,cz in [0,31]).
    int index(int cx, int cz) const {
        return (cz % 32) * 32 + (cx % 32);
    }
    // Place a block in global coords (x,z up to 511). We assume region(0,0).
    void setBlock(Block* block, int x, int y, int z) {
        if (x<0||z<0||x>=512||z>=512) {
            std::cerr<<"Region setBlock out of region\n"; exit(1);
        }
        int cx = x / 16;
        int cz = z / 16;
        int idx = index(cx, cz);
        if (!chunks[idx]) {
            chunks[idx] = new Chunk(cx, cz);
        }
        chunks[idx]->setBlock(block, x % 16, y, z % 16);
    }

    // Save to .mca: write location table + timestamps + chunk data.
    void save(const std::string &fname) {
        // First, build each chunk's compressed data
        struct Loc { int offset, count; };
        std::vector<std::vector<uint8_t>> chunkData(1024);
        std::vector<Loc> locs(1024, {-1,0});
        // Gather chunk blobs
        std::vector<uint8_t> chunks_bytes;  // concatenated chunk data after headers
        for (int i = 0; i < 1024; i++) {
            if (!chunks[i]) {
                // Empty chunk slot
                locs[i] = {-1, 0};
                continue;
            }
            // Get NBT for chunk and compress
            auto nbt = chunks[i]->toNBT();
            uLongf compSize = compressBound(nbt.size());
            std::vector<uint8_t> comp(compSize);
            if (compress2(comp.data(), &compSize, nbt.data(), nbt.size(), Z_BEST_COMPRESSION) != Z_OK) {
                std::cerr << "ZLIB compress failed\n";
                exit(1);
            }
            comp.resize(compSize);
            // Build chunk blob: [length (4 bytes)] [compression byte=2] [zlib data]
            uint32_t len = (uint32_t)comp.size() + 1;
            std::vector<uint8_t> blob;
            blob.resize(4 + 1 + comp.size());
            // big-endian length
            blob[0] = (len>>24)&0xFF;
            blob[1] = (len>>16)&0xFF;
            blob[2] = (len>>8)&0xFF;
            blob[3] = len&0xFF;
            blob[4] = 2; // compression type: zlib
            std::memcpy(blob.data()+5, comp.data(), comp.size());
            // Compute offset/count
            int sector = chunks_bytes.size() / 4096;
            int sectors = (blob.size() + 4095) / 4096;
            locs[i] = {sector + 2, sectors};
            // Pad blob to 4KB
            int pad = sectors*4096 - (int)blob.size();
            blob.insert(blob.end(), pad, 0);
            // Append to chunks_bytes
            chunks_bytes.insert(chunks_bytes.end(), blob.begin(), blob.end());
        }
        // Build header: 4096 bytes for locations (4 bytes each)
        std::vector<uint8_t> locations(4096, 0);
        for (int i = 0; i < 1024; i++) {
            if (locs[i].offset < 0) continue;
            int off = locs[i].offset;
            int cnt = locs[i].count;
            // 3-byte offset, 1-byte count
            locations[4*i + 0] = (off >> 16) & 0xFF;
            locations[4*i + 1] = (off >> 8) & 0xFF;
            locations[4*i + 2] = off & 0xFF;
            locations[4*i + 3] = cnt & 0xFF;
        }
        // Timestamps: 4096 bytes of zero
        std::vector<uint8_t> timestamps(4096, 0);
        // Combine into final file
        std::ofstream fout(fname, std::ios::binary);
        fout.write(reinterpret_cast<char*>(locations.data()), locations.size());
        fout.write(reinterpret_cast<char*>(timestamps.data()), timestamps.size());
        if (!chunks_bytes.empty()) {
            fout.write(reinterpret_cast<char*>(chunks_bytes.data()), chunks_bytes.size());
        }
        // Final pad to multiple of 4096
        std::streamoff fileSize = fout.tellp();
        int pad = 4096 - (fileSize % 4096);
        if (pad < 4096) {
            std::vector<uint8_t> pp(pad, 0);
            fout.write(reinterpret_cast<char*>(pp.data()), pad);
        }
        fout.close();
    }
};



// Additional code
float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

float lerp(float a, float b, float t) { return a + t * (b - a); }

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float perlin(float x, float y, int seed = 0) {
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
    return total;
}

int main() {
    Region region(0, 0);
    Block stone("minecraft", "stone");
    Block dirt("minecraft", "dirt");
    Block grass("minecraft", "grass_block");

    const int width = 512, depth = 512, height_limit = 128;
    const float scale = 0.01f;

    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            // Get height using fbm-based Perlin
            float h = fbm(x * scale, z * scale, 5);
            int height = (int)(h * 64) + 64; // center at sea level

            for (int y = 0; y <= height; y++) {
                if (y == height)
                    region.setBlock(&grass, x, y, z);
                else if (y >= height - 3)
                    region.setBlock(&dirt, x, y, z);
                else
                    region.setBlock(&stone, x, y, z);
            }
        }
    }

    region.save("r.0.0.mca");
    std::cout << "Saved perlin terrain to r.0.0.mca\n";
    return 0;
}

