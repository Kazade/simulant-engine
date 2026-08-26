#include "png_writer.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <simulant/logging.h>

namespace sprite_gen {

namespace {

uint32_t crc32_of(const uint8_t* data, std::size_t len) {
    static uint32_t table[256];
    static bool table_ready = false;
    if(!table_ready) {
        for(uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for(int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[n] = c;
        }
        table_ready = true;
    }

    uint32_t crc = 0xFFFFFFFFu;
    for(std::size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

uint32_t adler32_of(const uint8_t* data, std::size_t len) {
    uint32_t a = 1, b = 0;
    const uint32_t MOD = 65521;
    for(std::size_t i = 0; i < len; ++i) {
        a = (a + data[i]) % MOD;
        b = (b + a) % MOD;
    }
    return (b << 16) | a;
}

void put_u32be(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(uint8_t((v >> 24) & 0xFF));
    out.push_back(uint8_t((v >> 16) & 0xFF));
    out.push_back(uint8_t((v >> 8) & 0xFF));
    out.push_back(uint8_t(v & 0xFF));
}

void write_chunk(std::vector<uint8_t>& out, const char* type,
                 const std::vector<uint8_t>& data) {
    put_u32be(out, uint32_t(data.size()));
    std::size_t type_off = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    out.resize(out.size() + 4);
    uint32_t crc = crc32_of(out.data() + type_off, 4 + data.size());
    out[out.size() - 4] = uint8_t((crc >> 24) & 0xFF);
    out[out.size() - 3] = uint8_t((crc >> 16) & 0xFF);
    out[out.size() - 2] = uint8_t((crc >> 8) & 0xFF);
    out[out.size() - 1] = uint8_t(crc & 0xFF);
}

/* Wraps `raw` in a zlib stream made of uncompressed ("stored") DEFLATE
 * blocks - the simplest valid DEFLATE encoding, needing no compression
 * library. The result is larger than a compressed PNG, but this is only ever
 * a short-lived temp file read straight back by texconv. */
std::vector<uint8_t> zlib_store(const std::vector<uint8_t>& raw) {
    std::vector<uint8_t> out;
    out.push_back(0x78);
    out.push_back(0x01);

    const std::size_t max_block = 65535;
    std::size_t offset = 0;
    do {
        std::size_t remaining = raw.size() - offset;
        std::size_t block_len = std::min(remaining, max_block);
        bool is_final = (offset + block_len) >= raw.size();

        out.push_back(is_final ? 0x01 : 0x00);
        uint16_t len = uint16_t(block_len);
        uint16_t nlen = uint16_t(~len);
        out.push_back(uint8_t(len & 0xFF));
        out.push_back(uint8_t((len >> 8) & 0xFF));
        out.push_back(uint8_t(nlen & 0xFF));
        out.push_back(uint8_t((nlen >> 8) & 0xFF));
        out.insert(out.end(), raw.begin() + offset, raw.begin() + offset + block_len);

        offset += block_len;
    } while(offset < raw.size());

    put_u32be(out, adler32_of(raw.data(), raw.size()));
    return out;
}

} // namespace

bool write_png(const std::string& path, uint16_t width, uint16_t height,
               const uint8_t* rgba) {
    FILE* fp = fopen(path.c_str(), "wb");
    if(!fp) {
        S_ERROR("Could not open '{0}' for writing", path);
        return false;
    }

    static const uint8_t SIGNATURE[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    std::vector<uint8_t> file(SIGNATURE, SIGNATURE + 8);

    std::vector<uint8_t> ihdr;
    put_u32be(ihdr, width);
    put_u32be(ihdr, height);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(6); // color type: truecolor + alpha
    ihdr.push_back(0); // compression method
    ihdr.push_back(0); // filter method
    ihdr.push_back(0); // interlace method
    write_chunk(file, "IHDR", ihdr);

    std::vector<uint8_t> raw;
    raw.reserve(std::size_t(height) * (1 + std::size_t(width) * 4));
    for(uint16_t y = 0; y < height; ++y) {
        raw.push_back(0); // scanline filter: none
        const uint8_t* row = rgba + std::size_t(y) * width * 4;
        raw.insert(raw.end(), row, row + std::size_t(width) * 4);
    }

    write_chunk(file, "IDAT", zlib_store(raw));
    write_chunk(file, "IEND", {});

    bool ok = fwrite(file.data(), file.size(), 1, fp) == 1;
    if(!ok) {
        S_ERROR("Failed to write PNG data to '{0}'", path);
    }

    fclose(fp);
    return ok;
}

} // namespace sprite_gen
