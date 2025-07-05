#include "mca_generator.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace block {
    const std::string ns = "minecraft";
#define DEFINE_BLOCK(name, id) std::shared_ptr<const Block> name = std::make_shared<const Block>(ns, id);
    DEFINE_BLOCK(stone, "stone")
    DEFINE_BLOCK(dirt, "dirt")
    DEFINE_BLOCK(grass_block, "grass_block")
    DEFINE_BLOCK(water, "water")
    DEFINE_BLOCK(lava, "lava")
    DEFINE_BLOCK(sand, "sand")
    DEFINE_BLOCK(gravel, "gravel")
    DEFINE_BLOCK(oak_planks, "oak_planks")
    DEFINE_BLOCK(oak_log, "oak_log")
    DEFINE_BLOCK(oak_leaves, "oak_leaves")
    DEFINE_BLOCK(bedrock, "bedrock")
    DEFINE_BLOCK(coal_ore, "coal_ore")
    DEFINE_BLOCK(iron_ore, "iron_ore")
    DEFINE_BLOCK(gold_ore, "gold_ore")
    DEFINE_BLOCK(diamond_ore, "diamond_ore")
    DEFINE_BLOCK(emerald_ore, "emerald_ore")
    DEFINE_BLOCK(redstone_ore, "redstone_ore")
    DEFINE_BLOCK(lapis_ore, "lapis_ore")
    DEFINE_BLOCK(obsidian, "obsidian")
    DEFINE_BLOCK(cobblestone, "cobblestone")
    DEFINE_BLOCK(mossy_cobblestone, "mossy_cobblestone")
    DEFINE_BLOCK(brick_block, "bricks")
    DEFINE_BLOCK(netherrack, "netherrack")
    DEFINE_BLOCK(soul_sand, "soul_sand")
    DEFINE_BLOCK(glowstone, "glowstone")
    DEFINE_BLOCK(end_stone, "end_stone")
    DEFINE_BLOCK(tnt, "tnt")
    DEFINE_BLOCK(glass, "glass")
    DEFINE_BLOCK(ice, "ice")
    DEFINE_BLOCK(snow_block, "snow_block")
    DEFINE_BLOCK(clay, "clay")
    DEFINE_BLOCK(pumpkin, "pumpkin")
    DEFINE_BLOCK(melon, "melon")
    DEFINE_BLOCK(mycelium, "mycelium")
    DEFINE_BLOCK(nether_quartz_ore, "nether_quartz_ore")
    DEFINE_BLOCK(hay_block, "hay_block")
    DEFINE_BLOCK(emerald_block, "emerald_block")
    DEFINE_BLOCK(redstone_block, "redstone_block")
    DEFINE_BLOCK(sea_lantern, "sea_lantern")
    DEFINE_BLOCK(prismarine, "prismarine")
    DEFINE_BLOCK(dark_prismarine, "dark_prismarine")
    DEFINE_BLOCK(slime_block, "slime_block")
    DEFINE_BLOCK(chorus_plant, "chorus_plant")
    DEFINE_BLOCK(purpur_block, "purpur_block")
    DEFINE_BLOCK(end_rod, "end_rod")
    DEFINE_BLOCK(magma_block, "magma_block")
    DEFINE_BLOCK(nether_wart_block, "nether_wart_block")
    DEFINE_BLOCK(bone_block, "bone_block")
    DEFINE_BLOCK(honey_block, "honey_block")
    DEFINE_BLOCK(crying_obsidian, "crying_obsidian")
    DEFINE_BLOCK(blackstone, "blackstone")
    DEFINE_BLOCK(basalt, "basalt")
    DEFINE_BLOCK(nether_gold_ore, "nether_gold_ore")
    DEFINE_BLOCK(ancient_debris, "ancient_debris")
    DEFINE_BLOCK(gilded_blackstone, "gilded_blackstone")
    DEFINE_BLOCK(amethyst_block, "amethyst_block")
    DEFINE_BLOCK(copper_ore, "copper_ore")
    DEFINE_BLOCK(deepslate, "deepslate")
    DEFINE_BLOCK(tuff, "tuff")
    DEFINE_BLOCK(calcite, "calcite")
    DEFINE_BLOCK(dripstone_block, "dripstone_block")
    DEFINE_BLOCK(pointed_dripstone, "pointed_dripstone")
    DEFINE_BLOCK(rooted_dirt, "rooted_dirt")
    DEFINE_BLOCK(mud, "mud")
    DEFINE_BLOCK(muddy_mangrove_roots, "muddy_mangrove_roots")
    DEFINE_BLOCK(packed_mud, "packed_mud")
    DEFINE_BLOCK(mud_bricks, "mud_bricks")
    DEFINE_BLOCK(deepslate_coal_ore, "deepslate_coal_ore")
    DEFINE_BLOCK(deepslate_iron_ore, "deepslate_iron_ore")
    DEFINE_BLOCK(deepslate_gold_ore, "deepslate_gold_ore")
    DEFINE_BLOCK(deepslate_diamond_ore, "deepslate_diamond_ore")
    DEFINE_BLOCK(deepslate_emerald_ore, "deepslate_emerald_ore")
    DEFINE_BLOCK(deepslate_redstone_ore, "deepslate_redstone_ore")
    DEFINE_BLOCK(deepslate_lapis_ore, "deepslate_lapis_ore")
    DEFINE_BLOCK(deepslate_copper_ore, "deepslate_copper_ore")
    DEFINE_BLOCK(raw_iron_block, "raw_iron_block")
    DEFINE_BLOCK(raw_gold_block, "raw_gold_block")
    DEFINE_BLOCK(raw_copper_block, "raw_copper_block")
#undef DEFINE_BLOCK
}// Section implementation
Section::Section(int y_) : y(y_) {
    blocks.fill(nullptr);
    air = new Block("minecraft", "air");
}

void Section::setBlock(Block* block, int x, int yy, int z) {
    if (x<0||x>15||yy<0||yy>15||z<0||z>15) {
        std::cerr << "Section setBlock out of bounds\n"; exit(1);
    }
    int idx = yy*256 + z*16 + x;
    blocks[idx] = block; // Note: This still takes Block*, but we'll adjust downstream
}

std::vector<Block*> Section::palette() const {
    std::vector<Block*> p;
    for (Block* b : blocks) {
        if (b && std::find(p.begin(), p.end(), b) == p.end()) {
            p.push_back(b);
        }
    }
    if (std::find(blocks.begin(), blocks.end(), nullptr) != blocks.end()) {
        p.push_back(air);
    }
    if (p.empty()) {
        p.push_back(air);
    }
    return p;
}

std::vector<uint64_t> Section::blockStates(const std::vector<Block*>& pal) const {
    int palSize = (int)pal.size();
    int bits = std::max((palSize - 1) > 0 ? 32 - __builtin_clz(palSize - 1) : 0, 4);
    std::vector<uint64_t> states;
    uint64_t current = 0;
    int curLen = 0;
    for (int i = 0; i < 4096; i++) {
        Block* b = blocks[i];
        int idx = !b ? std::find(pal.begin(), pal.end(), air) - pal.begin() :
                      std::find(pal.begin(), pal.end(), b) - pal.begin();
        if (curLen + bits > 64) {
            int leftover = 64 - curLen;
            uint64_t lowmask = (1ULL << leftover) - 1;
            uint64_t lowbits = idx & lowmask;
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

// Chunk implementation
Chunk::Chunk(int cx_, int cz_) : cx(cx_), cz(cz_) {
    sections.fill(nullptr);
    biomes.resize(1024, 1); // Initialize with plains (ID 1)
}

void Chunk::setBlock(Block* block, int x, int y, int z) {
    if (x<0||x>15||z<0||z>15||y<0||y>255) {
        std::cerr << "Chunk setBlock out of bounds\n"; exit(1);
    }
    int secY = y / 16;
    if (!sections[secY]) {
        sections[secY] = new Section(secY);
    }
    sections[secY]->setBlock(block, x, y - secY*16, z);
}

std::vector<uint8_t> Chunk::toNBT() const {
    std::vector<uint8_t> data;
    auto put_u8 = [&](uint8_t v){ data.push_back(v); };
    auto put_u16 = [&](uint16_t v){ data.push_back((v>>8)&0xFF); data.push_back(v&0xFF); };
    auto put_u32 = [&](uint32_t v){ for(int i=24;i>=0;i-=8) data.push_back((v>>i)&0xFF); };
    auto put_u64 = [&](uint64_t v){ for(int i=56;i>=0;i-=8) data.push_back((v>>i)&0xFF); };
    auto put_str = [&](const std::string &s){ put_u16(s.size()); data.insert(data.end(), s.begin(), s.end()); };

    put_u8(10); put_u16(0); // TAG_Compound, empty name
    put_u8(3); put_str("DataVersion"); put_u32(version);
    put_u8(10); put_str("Level"); // TAG_Compound "Level"
    put_u8(9); put_str("Entities"); put_u8(10); put_u32(0); // Empty Entities
    put_u8(9); put_str("TileEntities"); put_u8(10); put_u32(0); // Empty TileEntities
    put_u8(9); put_str("LiquidTicks"); put_u8(10); put_u32(0); // Empty LiquidTicks
    put_u8(3); put_str("xPos"); put_u32(cx);
    put_u8(3); put_str("zPos"); put_u32(cz);
    put_u8(4); put_str("LastUpdate"); put_u64(0);
    put_u8(4); put_str("InhabitedTime"); put_u64(0);
    put_u8(1); put_str("isLightOn"); put_u8(1);
    put_u8(8); put_str("Status"); put_str("full");

    std::vector<Section*> present;
    for (Section* s : sections) if (s && !(s->palette().size()==1 && s->palette()[0]->name()=="minecraft:air")) present.push_back(s);
    put_u8(9); put_str("Sections"); put_u8(10); put_u32(present.size());
    for (Section* s : present) {
        put_u8(1); put_str("Y"); put_u8(s->y);
        put_u8(9); put_str("Palette"); put_u8(10); auto pal = s->palette(); put_u32(pal.size());
        for (Block* b : pal) { put_u8(8); put_str("Name"); put_str(b->name()); put_u8(0); }
        put_u8(12); put_str("BlockStates"); auto states = s->blockStates(pal); put_u32(states.size());
        for (uint64_t v : states) put_u64(v);
        put_u8(0);
    }

    put_u8(11); put_str("Biomes"); put_u32(1024);
    for (int i = 0; i < 1024; i++) put_u32(biomes[i]);
    put_u8(0); put_u8(0); // End Level, End root
    return data;
}

// Region implementation
Region::Region() {
    chunks.fill(nullptr);
    biomeGrid.resize(128, std::vector<int>(128, 1)); // Initialize with plains (ID 1)
}

int Region::index(int cx, int cz) const {
    return (cz % 32) * 32 + (cx % 32);
}

void Region::setBlock(const Block* block, int x, int y, int z) {
    int cx = x / 16;
    int cz = z / 16;
    int idx = index(cx, cz);
    if (!chunks[idx]) chunks[idx] = new Chunk(cx, cz);
    chunks[idx]->setBlock(const_cast<Block*>(block), x % 16, y, z % 16); // Cast to non-const
}

void Region::save(const std::string &fname) {
    struct Loc { int offset, count; };
    std::vector<Loc> locs(1024, {-1,0});
    std::vector<uint8_t> chunks_bytes;
    for (int i = 0; i < 1024; i++) {
        if (!chunks[i]) { locs[i] = {-1, 0}; continue; }
        auto nbt = chunks[i]->toNBT();
        uLongf compSize = compressBound(nbt.size());
        std::vector<uint8_t> comp(compSize);
        if (compress2(comp.data(), &compSize, nbt.data(), nbt.size(), Z_BEST_COMPRESSION) != Z_OK) {
            std::cerr << "ZLIB compress failed\n"; exit(1);
        }
        comp.resize(compSize);
        uint32_t len = comp.size() + 1;
        std::vector<uint8_t> blob(4 + 1 + comp.size());
        blob[0] = (len>>24)&0xFF; blob[1] = (len>>16)&0xFF; blob[2] = (len>>8)&0xFF; blob[3] = len&0xFF;
        blob[4] = 2; std::memcpy(blob.data()+5, comp.data(), comp.size());
        int sector = chunks_bytes.size() / 4096;
        int sectors = (blob.size() + 4095) / 4096;
        locs[i] = {sector + 2, sectors};
        int pad = sectors*4096 - blob.size();
        blob.insert(blob.end(), pad, 0);
        chunks_bytes.insert(chunks_bytes.end(), blob.begin(), blob.end());
    }
    std::vector<uint8_t> locations(4096, 0);
    for (int i = 0; i < 1024; i++) if (locs[i].offset >= 0) {
        locations[4*i + 0] = (locs[i].offset >> 16) & 0xFF;
        locations[4*i + 1] = (locs[i].offset >> 8) & 0xFF;
        locations[4*i + 2] = locs[i].offset & 0xFF;
        locations[4*i + 3] = locs[i].count & 0xFF;
    }
    std::vector<uint8_t> timestamps(4096, 0);
    std::ofstream fout(fname, std::ios::binary);
    fout.write(reinterpret_cast<char*>(locations.data()), locations.size());
    fout.write(reinterpret_cast<char*>(timestamps.data()), timestamps.size());
    if (!chunks_bytes.empty()) fout.write(reinterpret_cast<char*>(chunks_bytes.data()), chunks_bytes.size());
    std::streamoff fileSize = fout.tellp();
    int pad = 4096 - (fileSize % 4096);
    if (pad < 4096) { std::vector<uint8_t> pp(pad, 0); fout.write(reinterpret_cast<char*>(pp.data()), pad); }
    fout.close();
}

// World implementation
void World::setBlock(const std::shared_ptr<const Block>& block, int x, int y, int z) {
    int rx = x / 512; if (x < 0 && x % 512 != 0) rx--;
    int rz = z / 512; if (z < 0 && z % 512 != 0) rz--;
    auto key = std::make_pair(rx, rz);
    if (regions.find(key) == regions.end()) regions[key] = std::make_shared<Region>();
    regions[key]->setBlock(block.get(), x, y, z);
}

void World::setBiomeColumn(int x, int z, int minY, int maxY, int biomeId) {
    int rx = x / 512; if (x < 0 && x % 512 != 0) rx--;
    int rz = z / 512; if (z < 0 && z % 512 != 0) rz--;
    auto key = std::make_pair(rx, rz);
    if (regions.find(key) == regions.end()) regions[key] = std::make_shared<Region>();

    int gridX = (x % 512) / 4;
    int gridZ = (z % 512) / 4;
    minY = std::max(0, minY);
    maxY = std::min(255, maxY);
    if (minY > maxY) std::swap(minY, maxY);

    regions[key]->biomeGrid[gridX][gridZ] = biomeId;

    for (int cx = (x / 16) % 32; cx <= (x / 16) % 32; cx++) {
        for (int cz = (z / 16) % 32; cz <= (z / 16) % 32; cz++) {
            int idx = regions[key]->index(cx, cz);
            if (!regions[key]->chunks[idx]) regions[key]->chunks[idx] = new Chunk(cx + rx * 32, cz + rz * 32);
            Chunk* chunk = regions[key]->chunks[idx];
            int localX = x % 16;
            int localZ = z % 16;
            int minLevel = minY / 4;
            int maxLevel = maxY / 4;
            for (int yLevel = minLevel; yLevel <= maxLevel; yLevel++) {
                int biomeIndex = (localZ / 4) * 64 + (localX / 4) * 16 + yLevel;
                if (biomeIndex >= 0 && biomeIndex < 1024) chunk->biomes[biomeIndex] = biomeId;
            }
        }
    }
}

void World::save() {
    for (const auto& [key, region] : regions) {
        int rx = key.first;
        int rz = key.second;
        std::string fname = "files/r." + std::to_string(rx) + "." + std::to_string(rz) + ".mca";
        region->save(fname);
        std::cout << "Saved region to " << fname << "\n";
    }
}
