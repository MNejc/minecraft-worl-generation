#ifndef MCA_GENERATOR_H
#define MCA_GENERATOR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <zlib.h>
#include <cassert>

struct Block {
    std::string ns, id;
    Block(const std::string &ns_, const std::string &id_) : ns(ns_), id(id_) {}
    std::string name() const { return ns + ":" + id; }
};

namespace block {
#define DECLARE_BLOCK(name) extern std::shared_ptr<const Block> name;
    DECLARE_BLOCK(stone)
    DECLARE_BLOCK(dirt)
    DECLARE_BLOCK(grass_block)
    DECLARE_BLOCK(water)
    DECLARE_BLOCK(lava)
    DECLARE_BLOCK(sand)
    DECLARE_BLOCK(gravel)
    DECLARE_BLOCK(oak_planks)
    DECLARE_BLOCK(oak_log)
    DECLARE_BLOCK(oak_leaves)
    DECLARE_BLOCK(bedrock)
    DECLARE_BLOCK(coal_ore)
    DECLARE_BLOCK(iron_ore)
    DECLARE_BLOCK(gold_ore)
    DECLARE_BLOCK(diamond_ore)
    DECLARE_BLOCK(emerald_ore)
    DECLARE_BLOCK(redstone_ore)
    DECLARE_BLOCK(lapis_ore)
    DECLARE_BLOCK(obsidian)
    DECLARE_BLOCK(cobblestone)
    DECLARE_BLOCK(mossy_cobblestone)
    DECLARE_BLOCK(brick_block)
    DECLARE_BLOCK(netherrack)
    DECLARE_BLOCK(soul_sand)
    DECLARE_BLOCK(glowstone)
    DECLARE_BLOCK(end_stone)
    DECLARE_BLOCK(tnt)
    DECLARE_BLOCK(glass)
    DECLARE_BLOCK(ice)
    DECLARE_BLOCK(snow_block)
    DECLARE_BLOCK(clay)
    DECLARE_BLOCK(pumpkin)
    DECLARE_BLOCK(melon)
    DECLARE_BLOCK(mycelium)
    DECLARE_BLOCK(nether_quartz_ore)
    DECLARE_BLOCK(hay_block)
    DECLARE_BLOCK(emerald_block)
    DECLARE_BLOCK(redstone_block)
    DECLARE_BLOCK(sea_lantern)
    DECLARE_BLOCK(prismarine)
    DECLARE_BLOCK(dark_prismarine)
    DECLARE_BLOCK(slime_block)
    DECLARE_BLOCK(chorus_plant)
    DECLARE_BLOCK(purpur_block)
    DECLARE_BLOCK(end_rod)
    DECLARE_BLOCK(magma_block)
    DECLARE_BLOCK(nether_wart_block)
    DECLARE_BLOCK(bone_block)
    DECLARE_BLOCK(honey_block)
    DECLARE_BLOCK(crying_obsidian)
    DECLARE_BLOCK(blackstone)
    DECLARE_BLOCK(basalt)
    DECLARE_BLOCK(nether_gold_ore)
    DECLARE_BLOCK(ancient_debris)
    DECLARE_BLOCK(gilded_blackstone)
    DECLARE_BLOCK(amethyst_block)
    DECLARE_BLOCK(copper_ore)
    DECLARE_BLOCK(deepslate)
    DECLARE_BLOCK(tuff)
    DECLARE_BLOCK(calcite)
    DECLARE_BLOCK(dripstone_block)
    DECLARE_BLOCK(pointed_dripstone)
    DECLARE_BLOCK(rooted_dirt)
    DECLARE_BLOCK(mud)
    DECLARE_BLOCK(muddy_mangrove_roots)
    DECLARE_BLOCK(packed_mud)
    DECLARE_BLOCK(mud_bricks)
    DECLARE_BLOCK(deepslate_coal_ore)
    DECLARE_BLOCK(deepslate_iron_ore)
    DECLARE_BLOCK(deepslate_gold_ore)
    DECLARE_BLOCK(deepslate_diamond_ore)
    DECLARE_BLOCK(deepslate_emerald_ore)
    DECLARE_BLOCK(deepslate_redstone_ore)
    DECLARE_BLOCK(deepslate_lapis_ore)
    DECLARE_BLOCK(deepslate_copper_ore)
    DECLARE_BLOCK(raw_iron_block)
    DECLARE_BLOCK(raw_gold_block)
    DECLARE_BLOCK(raw_copper_block)
#undef DECLARE_BLOCK
}

// Represents a 16×16×16 section of blocks at height Y.
// Blocks are stored in XZY order: index = y*256 + z*16 + x.
struct Section {
    int y;                       // Section index (0=Y=0..15)
    std::array<Block*, 4096> blocks;
    Block* air;

    Section(int y_);
    void setBlock(Block* block, int x, int yy, int z);
    std::vector<Block*> palette() const;
    std::vector<uint64_t> blockStates(const std::vector<Block*>& pal) const;
};

// Represents one chunk at (cx, cz) relative to region, with up to 16 sections.
struct Chunk {
    int cx, cz;
    std::array<Section*, 16> sections;
    std::vector<int> biomes; // Store 1024 biome IDs
    int version = 2566;  // DataVersion

    Chunk(int cx_, int cz_);
    void setBlock(Block* block, int x, int y, int z);
    std::vector<uint8_t> toNBT() const;
};

// Represents a 32×32 chunk region at region coordinates (rx,rz).
struct Region {
    std::array<Chunk*, 1024> chunks;  // row-major: index = cz*32 + cx
    std::vector<std::vector<int>> biomeGrid; // 128x128 grid for 4x4 resolution

    Region();
    int index(int cx, int cz) const;
    void setBlock(const Block* block, int x, int y, int z); // Updated to const
    void save(const std::string &fname);
};

struct World {
    std::map<std::pair<int, int>, std::shared_ptr<Region>> regions;

    void setBlock(const std::shared_ptr<const Block>& block, int x, int y, int z); // Updated to const
    void setBiomeColumn(int x, int z, int minY, int maxY, int biomeId);
    void save();
};

#endif
