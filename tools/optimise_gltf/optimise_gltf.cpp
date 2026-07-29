/*
 * optimise_gltf
 * -------------
 * A host-side command line tool that takes a .gltf or .glb file and:
 *
 *   1. Converts all triangle-list meshes to triangle strips (degenerate
 *      triangles are used to stitch disjoint strips together).
 *   2. Creates .dtex versions of every texture using the `texconv` tool,
 *      registers the "SMLT_dtex_texture" extension in extensionsUsed and adds
 *      {"SMLT_dtex_texture": {"uri": "..."}} to every image. External images
 *      get a .dtex written alongside the source; embedded images get the .dtex
 *      embedded as a data URI.
 *   3. Writes the result to a new, self-contained .glb file.
 *
 * This is intentionally a single translation unit with no third party
 * dependencies beyond the C++20 standard library and the external `texconv`
 * binary.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Base64
// ---------------------------------------------------------------------------
namespace b64 {

static const char* ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string encode(const std::vector<uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    for(; i + 2 < data.size(); i += 3) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(ALPHABET[(n >> 18) & 63]);
        out.push_back(ALPHABET[(n >> 12) & 63]);
        out.push_back(ALPHABET[(n >> 6) & 63]);
        out.push_back(ALPHABET[n & 63]);
    }
    size_t rem = data.size() - i;
    if(rem == 1) {
        uint32_t n = data[i] << 16;
        out.push_back(ALPHABET[(n >> 18) & 63]);
        out.push_back(ALPHABET[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if(rem == 2) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
        out.push_back(ALPHABET[(n >> 18) & 63]);
        out.push_back(ALPHABET[(n >> 12) & 63]);
        out.push_back(ALPHABET[(n >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

static int val(char c) {
    if(c >= 'A' && c <= 'Z') return c - 'A';
    if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;
    return -1;
}

static std::vector<uint8_t> decode(const std::string& in) {
    std::vector<uint8_t> out;
    int buf = 0, bits = 0;
    for(char c : in) {
        if(c == '=' ) break;
        int v = val(c);
        if(v < 0) continue; // skip whitespace / newlines
        buf = (buf << 6) | v;
        bits += 6;
        if(bits >= 8) {
            bits -= 8;
            out.push_back((uint8_t)((buf >> bits) & 0xFF));
        }
    }
    return out;
}

} // namespace b64

// ---------------------------------------------------------------------------
// Minimal JSON value with insertion-ordered objects and (de)serialization.
// ---------------------------------------------------------------------------
struct Json {
    enum Type { Null, Bool, Number, String, Array, Object };
    Type type = Null;
    bool b = false;
    double num = 0.0;
    std::string str;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;

    Json() = default;
    Json(Type t): type(t) {}

    static Json object() { return Json(Object); }
    static Json array() { return Json(Array); }
    static Json number(double v) { Json j(Number); j.num = v; return j; }
    static Json string(const std::string& v) { Json j(String); j.str = v; return j; }

    bool is_object() const { return type == Object; }
    bool is_array() const { return type == Array; }

    bool has(const std::string& k) const {
        if(type != Object) return false;
        for(auto& p : obj) if(p.first == k) return true;
        return false;
    }

    Json* find(const std::string& k) {
        if(type != Object) return nullptr;
        for(auto& p : obj) if(p.first == k) return &p.second;
        return nullptr;
    }
    const Json* find(const std::string& k) const {
        if(type != Object) return nullptr;
        for(auto& p : obj) if(p.first == k) return &p.second;
        return nullptr;
    }

    // Get-or-create object member.
    Json& operator[](const std::string& k) {
        if(type != Object) { type = Object; obj.clear(); }
        for(auto& p : obj) if(p.first == k) return p.second;
        obj.emplace_back(k, Json());
        return obj.back().second;
    }

    long long as_int(long long def = 0) const {
        return type == Number ? (long long)num : def;
    }
    double as_double(double def = 0.0) const {
        return type == Number ? num : def;
    }
    std::string as_string(const std::string& def = "") const {
        return type == String ? str : def;
    }

    long long get_int(const std::string& k, long long def = 0) const {
        auto* p = find(k);
        return p ? p->as_int(def) : def;
    }
    std::string get_string(const std::string& k, const std::string& def = "") const {
        auto* p = find(k);
        return p ? p->as_string(def) : def;
    }
};

// ---- JSON parser ----------------------------------------------------------
namespace jsonp {

struct Parser {
    const char* p;
    const char* end;

    void skip_ws() {
        while(p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    }

    [[noreturn]] void fail(const char* msg) {
        throw std::runtime_error(std::string("JSON parse error: ") + msg);
    }

    Json parse() {
        skip_ws();
        Json v = parse_value();
        return v;
    }

    Json parse_value() {
        skip_ws();
        if(p >= end) fail("unexpected end");
        char c = *p;
        switch(c) {
            case '{': return parse_object();
            case '[': return parse_array();
            case '"': { Json j(Json::String); j.str = parse_string(); return j; }
            case 't': case 'f': return parse_bool();
            case 'n': return parse_null();
            default: return parse_number();
        }
    }

    std::string parse_string() {
        if(*p != '"') fail("expected string");
        ++p;
        std::string out;
        while(p < end) {
            char c = *p++;
            if(c == '"') return out;
            if(c == '\\') {
                if(p >= end) fail("bad escape");
                char e = *p++;
                switch(e) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        if(end - p < 4) fail("bad \\u");
                        int cp = 0;
                        for(int i = 0; i < 4; ++i) {
                            char h = *p++;
                            cp <<= 4;
                            if(h >= '0' && h <= '9') cp |= h - '0';
                            else if(h >= 'a' && h <= 'f') cp |= h - 'a' + 10;
                            else if(h >= 'A' && h <= 'F') cp |= h - 'A' + 10;
                            else fail("bad hex");
                        }
                        // Encode UTF-8 (surrogate pairs left as-is; rare in gltf).
                        if(cp < 0x80) out.push_back((char)cp);
                        else if(cp < 0x800) {
                            out.push_back((char)(0xC0 | (cp >> 6)));
                            out.push_back((char)(0x80 | (cp & 0x3F)));
                        } else {
                            out.push_back((char)(0xE0 | (cp >> 12)));
                            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                            out.push_back((char)(0x80 | (cp & 0x3F)));
                        }
                        break;
                    }
                    default: fail("bad escape char");
                }
            } else {
                out.push_back(c);
            }
        }
        fail("unterminated string");
    }

    Json parse_object() {
        Json j(Json::Object);
        ++p; // {
        skip_ws();
        if(p < end && *p == '}') { ++p; return j; }
        while(true) {
            skip_ws();
            std::string key = parse_string();
            skip_ws();
            if(p >= end || *p != ':') fail("expected ':'");
            ++p;
            Json v = parse_value();
            j.obj.emplace_back(std::move(key), std::move(v));
            skip_ws();
            if(p >= end) fail("unterminated object");
            if(*p == ',') { ++p; continue; }
            if(*p == '}') { ++p; break; }
            fail("expected ',' or '}'");
        }
        return j;
    }

    Json parse_array() {
        Json j(Json::Array);
        ++p; // [
        skip_ws();
        if(p < end && *p == ']') { ++p; return j; }
        while(true) {
            Json v = parse_value();
            j.arr.push_back(std::move(v));
            skip_ws();
            if(p >= end) fail("unterminated array");
            if(*p == ',') { ++p; continue; }
            if(*p == ']') { ++p; break; }
            fail("expected ',' or ']'");
        }
        return j;
    }

    Json parse_bool() {
        if(end - p >= 4 && std::strncmp(p, "true", 4) == 0) {
            p += 4; Json j(Json::Bool); j.b = true; return j;
        }
        if(end - p >= 5 && std::strncmp(p, "false", 5) == 0) {
            p += 5; Json j(Json::Bool); j.b = false; return j;
        }
        fail("bad literal");
    }

    Json parse_null() {
        if(end - p >= 4 && std::strncmp(p, "null", 4) == 0) { p += 4; return Json(); }
        fail("bad literal");
    }

    Json parse_number() {
        const char* start = p;
        if(p < end && (*p == '-' || *p == '+')) ++p;
        while(p < end && ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' ||
                          *p == 'E' || *p == '+' || *p == '-')) ++p;
        std::string tok(start, p);
        Json j(Json::Number);
        j.num = std::strtod(tok.c_str(), nullptr);
        return j;
    }
};

static Json parse(const std::string& text) {
    Parser parser{text.data(), text.data() + text.size()};
    return parser.parse();
}

} // namespace jsonp

// ---- JSON serializer ------------------------------------------------------
static void json_escape(const std::string& s, std::string& out) {
    out.push_back('"');
    for(unsigned char c : s) {
        switch(c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if(c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back((char)c);
                }
        }
    }
    out.push_back('"');
}

static void json_write(const Json& j, std::string& out) {
    switch(j.type) {
        case Json::Null: out += "null"; break;
        case Json::Bool: out += j.b ? "true" : "false"; break;
        case Json::Number: {
            double v = j.num;
            char buf[32];
            if(v == (double)(long long)v && std::abs(v) < 1e15) {
                std::snprintf(buf, sizeof(buf), "%lld", (long long)v);
            } else {
                std::snprintf(buf, sizeof(buf), "%.9g", v);
            }
            out += buf;
            break;
        }
        case Json::String: json_escape(j.str, out); break;
        case Json::Array: {
            out.push_back('[');
            for(size_t i = 0; i < j.arr.size(); ++i) {
                if(i) out.push_back(',');
                json_write(j.arr[i], out);
            }
            out.push_back(']');
            break;
        }
        case Json::Object: {
            out.push_back('{');
            for(size_t i = 0; i < j.obj.size(); ++i) {
                if(i) out.push_back(',');
                json_escape(j.obj[i].first, out);
                out.push_back(':');
                json_write(j.obj[i].second, out);
            }
            out.push_back('}');
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------
static std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("Could not open " + path.string());
    f.seekg(0, std::ios::end);
    std::streamoff len = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data((size_t)len);
    f.read((char*)data.data(), len);
    return data;
}

static void write_file(const fs::path& path, const uint8_t* data, size_t len) {
    std::ofstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("Could not write " + path.string());
    f.write((const char*)data, (std::streamsize)len);
}

// ---------------------------------------------------------------------------
// GLB / glTF container
// ---------------------------------------------------------------------------
static const uint32_t GLB_MAGIC = 0x46546C67; // 'glTF'
static const uint32_t GLB_CHUNK_JSON = 0x4E4F534A; // 'JSON'
static const uint32_t GLB_CHUNK_BIN = 0x004E4942; // 'BIN\0'

static uint32_t rd_u32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}
static void wr_u32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
    v.push_back((x >> 16) & 0xFF);
    v.push_back((x >> 24) & 0xFF);
}

struct GltfDoc {
    Json root;
    // Loaded contents of each buffer in root["buffers"].
    std::vector<std::vector<uint8_t>> buffers;
    fs::path source_dir;
    bool input_was_glb = false;
};

// Decode a "data:...;base64,..." URI into bytes. Returns false if not a data URI.
static bool decode_data_uri(const std::string& uri, std::vector<uint8_t>& out,
                            std::string* mime = nullptr) {
    if(uri.rfind("data:", 0) != 0) return false;
    size_t comma = uri.find(',');
    if(comma == std::string::npos) return false;
    std::string header = uri.substr(5, comma - 5); // e.g. "image/png;base64"
    if(mime) {
        size_t semi = header.find(';');
        *mime = header.substr(0, semi);
    }
    out = b64::decode(uri.substr(comma + 1));
    return true;
}

static GltfDoc load_gltf(const fs::path& path) {
    GltfDoc doc;
    doc.source_dir = path.parent_path();
    std::vector<uint8_t> file = read_file(path);

    std::vector<uint8_t> glb_bin; // BIN chunk (glb only)

    if(file.size() >= 12 && rd_u32(file.data()) == GLB_MAGIC) {
        doc.input_was_glb = true;
        uint32_t total = rd_u32(file.data() + 8);
        if(total > file.size()) total = (uint32_t)file.size();
        size_t off = 12;
        std::string json_text;
        while(off + 8 <= total) {
            uint32_t clen = rd_u32(file.data() + off);
            uint32_t ctype = rd_u32(file.data() + off + 4);
            off += 8;
            if(off + clen > file.size()) throw std::runtime_error("Corrupt GLB chunk");
            if(ctype == GLB_CHUNK_JSON) {
                json_text.assign((const char*)file.data() + off, clen);
            } else if(ctype == GLB_CHUNK_BIN) {
                glb_bin.assign(file.begin() + off, file.begin() + off + clen);
            }
            off += clen;
        }
        if(json_text.empty()) throw std::runtime_error("GLB has no JSON chunk");
        doc.root = jsonp::parse(json_text);
    } else {
        std::string text((const char*)file.data(), file.size());
        doc.root = jsonp::parse(text);
    }

    // Load every buffer.
    if(auto* buffers = doc.root.find("buffers")) {
        for(auto& buf : buffers->arr) {
            std::vector<uint8_t> data;
            const std::string uri = buf.get_string("uri");
            if(uri.empty()) {
                // glb BIN chunk (buffer 0).
                data = glb_bin;
            } else if(!decode_data_uri(uri, data)) {
                data = read_file(doc.source_dir / fs::path(uri));
            }
            doc.buffers.push_back(std::move(data));
        }
    }
    return doc;
}

// ---------------------------------------------------------------------------
// Accessor / index helpers
// ---------------------------------------------------------------------------
static int component_size(long long componentType) {
    switch(componentType) {
        case 5120: case 5121: return 1; // (u)byte
        case 5122: case 5123: return 2; // (u)short
        case 5125: case 5126: return 4; // uint / float
        default: return 0;
    }
}

// Read the SCALAR index accessor referenced by `accessorIndex` as uint32s.
static std::vector<uint32_t> read_indices(const GltfDoc& doc, long long accessorIndex) {
    const Json& accessors = *doc.root.find("accessors");
    const Json& acc = accessors.arr[(size_t)accessorIndex];
    long long componentType = acc.get_int("componentType");
    long long count = acc.get_int("count");
    long long accOffset = acc.get_int("byteOffset", 0);
    long long bvIndex = acc.get_int("bufferView", -1);

    std::vector<uint32_t> out;
    out.reserve((size_t)count);
    if(bvIndex < 0) { out.assign((size_t)count, 0); return out; }

    const Json& bv = doc.root.find("bufferViews")->arr[(size_t)bvIndex];
    long long bufIndex = bv.get_int("buffer", 0);
    long long bvOffset = bv.get_int("byteOffset", 0);
    const std::vector<uint8_t>& data = doc.buffers[(size_t)bufIndex];

    int cs = component_size(componentType);
    long long stride = bv.get_int("byteStride", 0);
    if(stride == 0) stride = cs;

    const uint8_t* base = data.data() + bvOffset + accOffset;
    for(long long i = 0; i < count; ++i) {
        const uint8_t* p = base + i * stride;
        uint32_t v = 0;
        if(cs == 1) v = p[0];
        else if(cs == 2) v = (uint32_t)p[0] | ((uint32_t)p[1] << 8);
        else v = rd_u32(p);
        out.push_back(v);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Mesh simplification (quadric error metric, endpoint edge collapse)
// ---------------------------------------------------------------------------
struct Vec3 {
    double x = 0, y = 0, z = 0;
};
static Vec3 operator-(const Vec3& a, const Vec3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Symmetric 4x4 error quadric stored as its 10 unique coefficients.
struct Quadric {
    double q[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    void add_plane(double a, double b, double c, double d) {
        q[0] += a * a; q[1] += a * b; q[2] += a * c; q[3] += a * d;
        q[4] += b * b; q[5] += b * c; q[6] += b * d;
        q[7] += c * c; q[8] += c * d;
        q[9] += d * d;
    }
    Quadric operator+(const Quadric& o) const {
        Quadric r;
        for(int i = 0; i < 10; ++i) r.q[i] = q[i] + o.q[i];
        return r;
    }
    double eval(const Vec3& p) const {
        return q[0] * p.x * p.x + 2 * q[1] * p.x * p.y + 2 * q[2] * p.x * p.z +
               2 * q[3] * p.x + q[4] * p.y * p.y + 2 * q[5] * p.y * p.z +
               2 * q[6] * p.y + q[7] * p.z * p.z + 2 * q[8] * p.z + q[9];
    }
};

// Read a VEC3 float accessor (e.g. POSITION) into Vec3s. Returns empty if the
// accessor is missing or not float VEC3 (simplification is then skipped).
static std::vector<Vec3> read_positions(const GltfDoc& doc, long long accessorIndex) {
    std::vector<Vec3> out;
    if(accessorIndex < 0) return out;
    const Json& acc = doc.root.find("accessors")->arr[(size_t)accessorIndex];
    if(acc.get_int("componentType") != 5126) return out; // FLOAT only
    if(acc.get_string("type") != "VEC3") return out;

    long long count = acc.get_int("count");
    long long accOffset = acc.get_int("byteOffset", 0);
    long long bvIndex = acc.get_int("bufferView", -1);
    if(bvIndex < 0) return out;

    const Json& bv = doc.root.find("bufferViews")->arr[(size_t)bvIndex];
    long long bufIndex = bv.get_int("buffer", 0);
    long long bvOffset = bv.get_int("byteOffset", 0);
    const std::vector<uint8_t>& data = doc.buffers[(size_t)bufIndex];
    long long stride = bv.get_int("byteStride", 0);
    if(stride == 0) stride = 12; // tightly packed VEC3<float>

    out.resize((size_t)count);
    const uint8_t* base = data.data() + bvOffset + accOffset;
    for(long long i = 0; i < count; ++i) {
        const uint8_t* p = base + i * stride;
        float f[3];
        std::memcpy(f, p, 12);
        out[(size_t)i] = {f[0], f[1], f[2]};
    }
    return out;
}

// Read a VEC2 float accessor (e.g. TEXCOORD_0). Returns empty if the
// accessor is missing or not float VEC2 (UV-aware substitution is then
// skipped, falling back to an arbitrary but position-correct choice).
static std::vector<std::array<float, 2>> read_uvs(const GltfDoc& doc,
                                                   long long accessorIndex) {
    std::vector<std::array<float, 2>> out;
    if(accessorIndex < 0) return out;
    const Json& acc = doc.root.find("accessors")->arr[(size_t)accessorIndex];
    if(acc.get_int("componentType") != 5126) return out; // FLOAT only
    if(acc.get_string("type") != "VEC2") return out;

    long long count = acc.get_int("count");
    long long accOffset = acc.get_int("byteOffset", 0);
    long long bvIndex = acc.get_int("bufferView", -1);
    if(bvIndex < 0) return out;

    const Json& bv = doc.root.find("bufferViews")->arr[(size_t)bvIndex];
    long long bufIndex = bv.get_int("buffer", 0);
    long long bvOffset = bv.get_int("byteOffset", 0);
    const std::vector<uint8_t>& data = doc.buffers[(size_t)bufIndex];
    long long stride = bv.get_int("byteStride", 0);
    if(stride == 0) stride = 8; // tightly packed VEC2<float>

    out.resize((size_t)count);
    const uint8_t* base = data.data() + bvOffset + accOffset;
    for(long long i = 0; i < count; ++i) {
        const uint8_t* p = base + i * stride;
        float f[2];
        std::memcpy(f, p, 8);
        out[(size_t)i] = {f[0], f[1]};
    }
    return out;
}

// A position quantized to a grid of size `eps`, used to weld/compare vertex
// positions within a small tolerance regardless of which primitive (or which
// exact float bits) they came from.
struct PosKey {
    int64_t x, y, z;
    bool operator==(const PosKey& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};
struct PosKeyHash {
    size_t operator()(const PosKey& k) const {
        size_t h = std::hash<int64_t>()(k.x);
        h ^= std::hash<int64_t>()(k.y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= std::hash<int64_t>()(k.z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

static PosKey quantize_pos(const Vec3& p, double eps) {
    auto q = [eps](double v) -> int64_t { return (int64_t)std::llround(v / eps); };
    return {q(p.x), q(p.y), q(p.z)};
}

// A weld tolerance appropriate for a bounding box: small enough not to merge
// genuinely distinct features, large enough to absorb float noise between
// vertices that are "the same point" but not bit-identical.
static double weld_epsilon(const Vec3& lo, const Vec3& hi) {
    double diag = std::sqrt(dot(hi - lo, hi - lo));
    return std::max(diag * 1e-5, 1e-6);
}

// glTF meshes commonly duplicate vertices along UV/normal seams (adjacent
// faces reference distinct vertex indices even though they sit at the same
// position). If edge-collapse adjacency were built from raw vertex indices,
// every seam would look like a mesh boundary, and each duplicated island
// would collapse independently of its neighbours - tearing the seams apart
// and leaving visible holes. To avoid that, positions within a small
// tolerance of each other are welded into a single canonical id purely for
// adjacency/quadric purposes; canonical ids are themselves raw vertex indices
// (the first raw vertex seen at that position), so no extra index space is
// needed.
//
// If `clusters` is non-null, it is filled in so that clusters[c] lists every
// raw vertex sharing canonical id c - i.e. every UV/normal "wedge" at that
// position - which callers can use to pick an attribute-correct substitute
// rather than an arbitrary one.
static std::vector<uint32_t> weld_positions(const std::vector<Vec3>& pos, double eps,
                                            std::vector<std::vector<uint32_t>>* clusters = nullptr) {
    size_t n = pos.size();
    std::vector<uint32_t> canon(n);
    if(clusters) clusters->assign(n, {});
    if(n == 0) return canon;

    std::unordered_map<PosKey, uint32_t, PosKeyHash> firstSeen;
    firstSeen.reserve(n);
    for(size_t i = 0; i < n; ++i) {
        PosKey k = quantize_pos(pos[i], eps);
        auto it = firstSeen.find(k);
        uint32_t c;
        if(it == firstSeen.end()) {
            firstSeen.emplace(k, (uint32_t)i);
            c = (uint32_t)i;
        } else {
            c = it->second;
        }
        canon[i] = c;
        if(clusters) (*clusters)[c].push_back((uint32_t)i);
    }
    return canon;
}

// Simplify a triangle list down to `ratio` of its triangles using quadric-error
// edge collapses. Adjacency and quadrics are computed on position-welded
// (canonical) vertices so that seams collapse in lock-step rather than
// tearing apart (see weld_positions() above). The collapse itself still only
// ever repoints a corner at an *existing* vertex - a corner is only swapped
// away from its original raw vertex when that vertex's canonical cluster was
// actually collapsed, in which case it's replaced with the cluster's
// canonical (surviving) vertex. Positions (and every other per-vertex
// attribute) are therefore never modified: the result is simply a reduced
// index list that references a subset of the original vertices. Collapses
// that would flip a triangle's normal are rejected to avoid gross artefacts.
//
// `eps` is the weld tolerance (see weld_epsilon()) and should be computed
// across the whole mesh (all sibling primitives), not just this primitive,
// so it is consistent with `foreignPos`. `foreignPos` holds vertex positions
// belonging to *other* primitives in the same mesh: separately-modelled
// pieces (e.g. a railing or window frame) frequently rest against this one
// without sharing any vertices, so any local vertex touching one of those
// positions is locked exactly like an internal boundary vertex - otherwise
// it could be collapsed away invisibly to this primitive's own topology,
// separating it from the neighbouring piece.
//
// `uv`, if non-empty, is used to pick the best-matching substitute whenever a
// corner's vertex has to be swapped: several raw vertices can share one
// position but sit in different UV islands (e.g. either side of a texture
// seam), so blindly picking any vertex at the surviving position can sample
// a completely unrelated part of the texture. Picking the closest one in UV
// space keeps that substitution visually correct.
static std::vector<uint32_t> simplify(const std::vector<Vec3>& pos,
                                      const std::vector<uint32_t>& tris,
                                      float ratio, double eps,
                                      const std::vector<Vec3>& foreignPos,
                                      const std::vector<std::array<float, 2>>& uv) {
    size_t triCount = tris.size() / 3;
    size_t target = (size_t)std::llround((double)triCount * (double)ratio);
    if(ratio >= 1.0f || triCount < 2) return tris;
    if(target < 1) target = 1;

    size_t nv = pos.size();
    bool haveUV = uv.size() == nv;
    std::vector<std::vector<uint32_t>> clusters;
    std::vector<uint32_t> canon = weld_positions(pos, eps, &clusters);
    std::vector<Quadric> Q(nv);
    std::vector<std::array<uint32_t, 3>> tri(triCount);
    std::vector<char> triRemoved(triCount, 0);
    std::vector<std::vector<uint32_t>> vtri(nv);

    size_t live = 0;
    for(size_t t = 0; t < triCount; ++t) {
        uint32_t a = canon[tris[t * 3]], b = canon[tris[t * 3 + 1]],
                 c = canon[tris[t * 3 + 2]];
        tri[t] = {a, b, c};
        if(a == b || b == c || a == c || a >= nv || b >= nv || c >= nv) {
            triRemoved[t] = 1;
            continue;
        }
        ++live;
        Vec3 n = cross(pos[b] - pos[a], pos[c] - pos[a]);
        double len = std::sqrt(dot(n, n));
        if(len > 0) {
            n = {n.x / len, n.y / len, n.z / len};
            double d = -dot(n, pos[a]);
            Q[a].add_plane(n.x, n.y, n.z, d);
            Q[b].add_plane(n.x, n.y, n.z, d);
            Q[c].add_plane(n.x, n.y, n.z, d);
        }
        vtri[a].push_back((uint32_t)t);
        vtri[b].push_back((uint32_t)t);
        vtri[c].push_back((uint32_t)t);
    }
    if(target >= live) return tris;

    // Classify vertices as "boundary" from the *original* topology (touching
    // an edge used by exactly one triangle). Meshes are frequently assembled
    // from separate, merely-touching pieces (e.g. a wall resting against a
    // floor without sharing vertices) rather than one watertight surface;
    // such touching seams show up as boundary edges here even though nothing
    // is actually open, and may even meet at a T-junction (a vertex of one
    // piece sitting on the *interior* of an edge in another) which is
    // invisible to vertex-based topology entirely. Removing - or even
    // sliding - a boundary vertex risks silently breaking such a contact and
    // opening a gap, so boundary vertices are always left untouched.
    auto ekey64 = [](uint32_t a, uint32_t b) -> uint64_t {
        if(a > b) std::swap(a, b);
        return ((uint64_t)a << 32) | b;
    };
    std::unordered_map<uint64_t, int> edgeUseCount;
    for(size_t t = 0; t < triCount; ++t) {
        if(triRemoved[t]) continue;
        for(int k = 0; k < 3; ++k) {
            uint32_t a = tri[t][k], b = tri[t][(k + 1) % 3];
            ++edgeUseCount[ekey64(a, b)];
        }
    }
    std::vector<char> isBoundaryVertex(nv, 0);
    for(auto& kv : edgeUseCount) {
        if(kv.second != 1) continue;
        isBoundaryVertex[(uint32_t)(kv.first >> 32)] = 1;
        isBoundaryVertex[(uint32_t)(kv.first & 0xFFFFFFFFu)] = 1;
    }

    // Also lock any vertex that coincides with a vertex from a sibling
    // primitive (see the function comment above).
    if(!foreignPos.empty()) {
        std::unordered_set<PosKey, PosKeyHash> foreignKeys;
        foreignKeys.reserve(foreignPos.size());
        for(auto& p : foreignPos) foreignKeys.insert(quantize_pos(p, eps));

        for(size_t i = 0; i < nv; ++i) {
            if(foreignKeys.count(quantize_pos(pos[i], eps))) {
                isBoundaryVertex[canon[i]] = 1;
            }
        }
    }

    std::vector<uint32_t> ver(nv, 0);
    std::vector<char> vremoved(nv, 0);

    struct E {
        double cost;
        uint32_t u, v, vu, vv;
    };
    struct Cmp {
        bool operator()(const E& a, const E& b) const { return a.cost > b.cost; }
    };
    std::priority_queue<E, std::vector<E>, Cmp> pq;

    auto push_edge = [&](uint32_t a, uint32_t b) {
        if(a == b || a >= nv || b >= nv) return;
        Quadric qs = Q[a] + Q[b];
        double cost = std::min(qs.eval(pos[a]), qs.eval(pos[b]));
        pq.push({cost, a, b, ver[a], ver[b]});
    };

    std::set<std::pair<uint32_t, uint32_t>> seen;
    for(size_t t = 0; t < triCount; ++t) {
        if(triRemoved[t]) continue;
        for(int k = 0; k < 3; ++k) {
            uint32_t a = tri[t][k], b = tri[t][(k + 1) % 3];
            auto key = std::minmax(a, b);
            if(seen.insert({key.first, key.second}).second) push_edge(a, b);
        }
    }

    while(live > target && !pq.empty()) {
        E e = pq.top();
        pq.pop();
        uint32_t u = e.u, v = e.v;
        if(vremoved[u] || vremoved[v]) continue;
        if(e.vu != ver[u] || e.vv != ver[v]) continue; // stale entry

        // Never remove a boundary vertex, even by sliding it along its own
        // border to a neighbouring border vertex. Meshes frequently have
        // separately-modelled pieces touching at a T-junction (a vertex of
        // one piece sitting on the *interior* of an edge in another, e.g. a
        // wall resting against a floor), which is invisible to vertex-based
        // topology. Sliding a border vertex away, while still leaving a
        // perfectly continuous border on its own piece, silently breaks that
        // contact and opens a gap. Keeping every boundary vertex fixed avoids
        // this regardless of whether such contacts can be detected.
        if(isBoundaryVertex[u] || isBoundaryVertex[v]) {
            continue;
        }

        Quadric qs = Q[u] + Q[v];
        uint32_t s = (qs.eval(pos[u]) <= qs.eval(pos[v])) ? u : v; // survivor
        uint32_t r = (s == u) ? v : u;                             // removed

        // Reject the collapse if any surviving incident triangle would flip,
        // or if it would blow up UV distortion. The collapse decision itself
        // is purely geometric (position-based QEM), so without this check it
        // can happily remove a vertex that existed specifically to carry a
        // smooth UV transition across a texture-atlas seam, directly joining
        // two atlas-distant corners into one triangle - substituting the
        // *closest* UV wedge (done at output time) can't fix that, since the
        // underlying edge shouldn't have been collapsed in the first place.
        bool flip = false;
        for(uint32_t t : vtri[r]) {
            if(triRemoved[t]) continue;
            auto& T = tri[t];
            if(T[0] == s || T[1] == s || T[2] == s) continue; // becomes degenerate
            Vec3 p[3] = {pos[T[0]], pos[T[1]], pos[T[2]]};
            Vec3 nOld = cross(p[1] - p[0], p[2] - p[0]);
            for(int k = 0; k < 3; ++k) if(T[k] == r) p[k] = pos[s];
            Vec3 nNew = cross(p[1] - p[0], p[2] - p[0]);
            if(dot(nOld, nNew) <= 0) { flip = true; break; }

            if(haveUV) {
                auto span = [&](std::array<float, 2> a, std::array<float, 2> b,
                                std::array<float, 2> c) {
                    float lo0 = std::min({a[0], b[0], c[0]});
                    float hi0 = std::max({a[0], b[0], c[0]});
                    float lo1 = std::min({a[1], b[1], c[1]});
                    float hi1 = std::max({a[1], b[1], c[1]});
                    return std::max(hi0 - lo0, hi1 - lo1);
                };
                float before = span(uv[T[0]], uv[T[1]], uv[T[2]]);
                std::array<float, 2> nu[3] = {uv[T[0]], uv[T[1]], uv[T[2]]};
                for(int k = 0; k < 3; ++k) if(T[k] == r) nu[k] = uv[s];
                float after = span(nu[0], nu[1], nu[2]);
                if(after > std::max(before * 2.5f, 0.05f)) {
                    flip = true;
                    break;
                }
            }
        }
        if(flip) continue;

        // Perform the collapse r -> s.
        Q[s] = qs;
        for(uint32_t t : vtri[r]) {
            if(triRemoved[t]) continue;
            auto& T = tri[t];
            bool hadS = (T[0] == s || T[1] == s || T[2] == s);
            for(int k = 0; k < 3; ++k) if(T[k] == r) T[k] = s;
            if(hadS || T[0] == T[1] || T[1] == T[2] || T[0] == T[2]) {
                triRemoved[t] = 1;
                --live;
            } else {
                vtri[s].push_back(t);
            }
        }
        vremoved[r] = 1;
        ++ver[s];

        for(uint32_t t : vtri[s]) {
            if(triRemoved[t]) continue;
            auto& T = tri[t];
            for(int k = 0; k < 3; ++k)
                if(T[k] != s && !vremoved[T[k]]) push_edge(s, T[k]);
        }
    }

    std::vector<uint32_t> out;
    out.reserve(live * 3);
    for(size_t t = 0; t < triCount; ++t) {
        if(triRemoved[t]) continue;
        for(int k = 0; k < 3; ++k) {
            uint32_t raw = tris[t * 3 + k];
            uint32_t finalCanon = tri[t][k];
            if(canon[raw] == finalCanon) {
                // This corner's cluster survived unchanged: keep the
                // original raw vertex, preserving its exact attributes.
                out.push_back(raw);
                continue;
            }
            // The cluster collapsed into a different (surviving) position.
            // Substitute with whichever raw vertex sharing that position has
            // the closest UV to this corner's original one, rather than an
            // arbitrary member - otherwise the face could end up sampling a
            // completely unrelated part of the texture.
            uint32_t best = finalCanon;
            if(haveUV) {
                double bestDist = std::numeric_limits<double>::max();
                for(uint32_t candidate : clusters[finalCanon]) {
                    double du = (double)uv[candidate][0] - (double)uv[raw][0];
                    double dv = (double)uv[candidate][1] - (double)uv[raw][1];
                    double dist = du * du + dv * dv;
                    if(dist < bestDist) {
                        bestDist = dist;
                        best = candidate;
                    }
                }
            }
            out.push_back(best);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Triangle strip generation
// ---------------------------------------------------------------------------

// True if (a,b,c) is an even (cyclic) permutation of (x,y,z), i.e. describes
// the same oriented (winding) triangle.
static bool same_orientation(uint32_t a, uint32_t b, uint32_t c,
                             uint32_t x, uint32_t y, uint32_t z) {
    return (a == x && b == y && c == z) ||
           (a == y && b == z && c == x) ||
           (a == z && b == x && c == y);
}

// Convert a triangle list into a single triangle strip. Adjacent triangles
// that share an edge (with consistent winding) are walked into a run; disjoint
// runs are stitched together with degenerate triangles so the whole primitive
// is one strip. Winding is validated on every extension so the result renders
// identically to the source.
//
// Adjacency is computed on position-welded (canonical) vertices via `canon`
// (indexed by raw vertex index; empty => no welding, raw indices used
// directly), for the same reason simplify() welds: glTF meshes routinely
// duplicate a vertex at every UV/normal seam, so two triangles that are
// geometrically adjacent along an edge often reference different raw vertex
// indices there. Without welding, the strip walk dies at every seam and
// fragments into many short strips, each needing degenerate triangles to
// stitch it to the next - overhead that can outgrow the savings of stripping
// in the first place. Output indices are always the original raw vertices,
// so attributes are unaffected.
static std::vector<uint32_t> stripify(const std::vector<uint32_t>& tris,
                                      const std::vector<uint32_t>& canon) {
    size_t triCount = tris.size() / 3;
    if(triCount == 0) return {};

    auto cid = [&](uint32_t raw) -> uint32_t {
        return raw < canon.size() ? canon[raw] : raw;
    };

    auto ekey = [](uint32_t a, uint32_t b) -> uint64_t {
        if(a > b) std::swap(a, b);
        return ((uint64_t)a << 32) | b;
    };

    std::unordered_multimap<uint64_t, size_t> edgeToTri;
    edgeToTri.reserve(triCount * 3);
    for(size_t t = 0; t < triCount; ++t) {
        uint32_t a = cid(tris[t * 3]), b = cid(tris[t * 3 + 1]), c = cid(tris[t * 3 + 2]);
        edgeToTri.insert({ekey(a, b), t});
        edgeToTri.insert({ekey(b, c), t});
        edgeToTri.insert({ekey(c, a), t});
    }

    std::vector<char> used(triCount, 0);
    std::vector<std::vector<uint32_t>> strips;

    for(size_t start = 0; start < triCount; ++start) {
        if(used[start]) continue;
        used[start] = 1;
        std::vector<uint32_t> strip = {tris[start * 3], tris[start * 3 + 1],
                                       tris[start * 3 + 2]};
        while(true) {
            size_t n = strip.size();
            uint32_t e0 = cid(strip[n - 2]), e1 = cid(strip[n - 1]);
            size_t p = n - 2; // index of the triangle we are about to append
            long long chosen = -1;
            uint32_t chosenD = 0;

            auto range = edgeToTri.equal_range(ekey(e0, e1));
            for(auto it = range.first; it != range.second; ++it) {
                size_t t = it->second;
                if(used[t]) continue;
                uint32_t rna = tris[t * 3], rnb = tris[t * 3 + 1], rnc = tris[t * 3 + 2];
                uint32_t na = cid(rna), nb = cid(rnb), nc = cid(rnc);
                uint32_t draw, d;
                if(na != e0 && na != e1) { draw = rna; d = na; }
                else if(nb != e0 && nb != e1) { draw = rnb; d = nb; }
                else if(nc != e0 && nc != e1) { draw = rnc; d = nc; }
                else continue; // degenerate under welding: no usable third corner

                uint32_t o0, o1, o2;
                if(p % 2 == 0) { o0 = e0; o1 = e1; o2 = d; }
                else { o0 = e1; o1 = e0; o2 = d; }

                if(same_orientation(o0, o1, o2, na, nb, nc)) {
                    chosen = (long long)t;
                    chosenD = draw;
                    break;
                }
            }

            if(chosen < 0) break;
            used[(size_t)chosen] = 1;
            strip.push_back(chosenD);
        }
        strips.push_back(std::move(strip));
    }

    // Stitch all strips into one using degenerate triangles.
    std::vector<uint32_t> out;
    for(auto& s : strips) {
        if(out.empty()) {
            out = s;
            continue;
        }
        // Stitch with degenerate triangles: repeat the last vertex of the
        // current strip, then the first vertex of the next strip enough times
        // that every transition triangle is degenerate and the next strip
        // starts on an even (forward-winding) position.
        size_t L = out.size();
        out.push_back(out.back()); // repeat last vertex of current strip
        out.push_back(s.front());  // first vertex of next strip
        out.push_back(s.front());
        if(L % 2 == 1) out.push_back(s.front()); // extra repeat to fix parity
        out.insert(out.end(), s.begin() + 1, s.end());
    }
    return out;
}

// ---------------------------------------------------------------------------
// texconv
// ---------------------------------------------------------------------------
struct Options {
    std::string texconv = "texconv";
    std::string format;    // empty => auto-select per texture from its contents
    bool mipmap = false;
    float lod = 1.0f;      // target triangle fraction (1.0 => no simplification)
};

// texconv output is always compressed (VQ). Format is chosen per texture.
static bool run_texconv(const Options& opt, const fs::path& in, const fs::path& out,
                        const std::string& format) {
    std::string cmd = "\"" + opt.texconv + "\"";
    cmd += " -i \"" + in.string() + "\"";
    cmd += " -o \"" + out.string() + "\"";
    cmd += " -f " + format;
    cmd += " -c"; // always output a compressed texture
    if(opt.mipmap) cmd += " -m";
    int rc = std::system(cmd.c_str());
    if(rc != 0) {
        std::cerr << "  texconv failed (exit " << rc << ") for " << in << "\n";
        return false;
    }
    return true;
}

static std::string ext_for_mime(const std::string& mime) {
    if(mime == "image/jpeg") return ".jpg";
    if(mime == "image/png") return ".png";
    return ".png";
}

// Does a PNG carry (potential) transparency? True for grayscale+alpha (4),
// truecolour+alpha (6), or a palette image (3) with a tRNS chunk.
static bool png_has_alpha(const std::vector<uint8_t>& d) {
    // 8 byte signature, then IHDR: len(4) "IHDR"(4) width(4) height(4)
    // bitDepth(1) colorType(1) ...  => colorType at offset 25.
    if(d.size() < 26) return false;
    uint8_t colorType = d[25];
    if(colorType == 4 || colorType == 6) return true;
    if(colorType == 3) {
        size_t off = 8; // walk chunks looking for tRNS before the first IDAT
        while(off + 8 <= d.size()) {
            uint32_t len = ((uint32_t)d[off] << 24) | ((uint32_t)d[off + 1] << 16) |
                           ((uint32_t)d[off + 2] << 8) | (uint32_t)d[off + 3];
            const uint8_t* type = &d[off + 4];
            if(std::memcmp(type, "tRNS", 4) == 0) return true;
            if(std::memcmp(type, "IDAT", 4) == 0) break;
            off += 12u + len; // len(4) + type(4) + data(len) + crc(4)
        }
    }
    return false;
}

// Pick the best-matching texconv format for a source image: ARGB4444 when the
// texture has an alpha channel, RGB565 otherwise.
static std::string choose_format(const std::vector<uint8_t>& bytes) {
    static const uint8_t PNG_SIG[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if(bytes.size() >= 8 && std::memcmp(bytes.data(), PNG_SIG, 8) == 0) {
        return png_has_alpha(bytes) ? "ARGB4444" : "RGB565";
    }
    // JPEG and everything else are opaque.
    return "RGB565";
}

// ---------------------------------------------------------------------------
// Image conversion
// ---------------------------------------------------------------------------

// Add {"SMLT_dtex_texture": {"uri": <uri>}} to an image object.
static void set_dtex_extension(Json& image, const std::string& uri) {
    image["extensions"]["SMLT_dtex_texture"]["uri"] = Json::string(uri);
}

static void ensure_extension_used(Json& root, const std::string& name) {
    Json& list = root["extensionsUsed"];
    if(!list.is_array()) list = Json::array();
    for(auto& e : list.arr) if(e.type == Json::String && e.str == name) return;
    list.arr.push_back(Json::string(name));
}

// Convert one image. Returns true if a .dtex was produced and the extension set.
static bool process_image(GltfDoc& doc, Json& image, const Options& opt,
                          size_t imageIndex) {
    const std::string uri = image.get_string("uri");

    if(!uri.empty() && uri.rfind("data:", 0) != 0) {
        // External file: write a .dtex alongside the source image.
        fs::path rel(uri);
        fs::path src = doc.source_dir / rel;
        fs::path dtexRel = rel;
        dtexRel.replace_extension(".dtex");
        fs::path dtexAbs = doc.source_dir / dtexRel;

        std::string format = opt.format;
        if(format.empty()) format = choose_format(read_file(src));

        std::cout << "  image[" << imageIndex << "] external -> "
                  << dtexRel.string() << " (" << format << ")\n";
        if(!run_texconv(opt, src, dtexAbs, format)) return false;
        set_dtex_extension(image, dtexRel.generic_string());
        return true;
    }

    // Embedded image (data URI or bufferView) -> embed the .dtex as a data URI.
    std::vector<uint8_t> bytes;
    std::string ext = ".png";
    if(!uri.empty()) {
        std::string mime;
        decode_data_uri(uri, bytes, &mime);
        ext = ext_for_mime(mime);
    } else if(image.has("bufferView")) {
        long long bvIndex = image.get_int("bufferView", -1);
        const Json& bv = doc.root.find("bufferViews")->arr[(size_t)bvIndex];
        long long bufIndex = bv.get_int("buffer", 0);
        long long bvOffset = bv.get_int("byteOffset", 0);
        long long bvLen = bv.get_int("byteLength", 0);
        const std::vector<uint8_t>& data = doc.buffers[(size_t)bufIndex];
        bytes.assign(data.begin() + bvOffset, data.begin() + bvOffset + bvLen);
        ext = ext_for_mime(image.get_string("mimeType"));
    } else {
        std::cerr << "  image[" << imageIndex << "] has no source; skipping\n";
        return false;
    }

    std::string format = opt.format.empty() ? choose_format(bytes) : opt.format;

    fs::path tmpDir = fs::temp_directory_path();
    fs::path tmpIn = tmpDir / ("optimise_gltf_" + std::to_string(imageIndex) + ext);
    fs::path tmpOut = tmpDir / ("optimise_gltf_" + std::to_string(imageIndex) + ".dtex");
    write_file(tmpIn, bytes.data(), bytes.size());

    std::cout << "  image[" << imageIndex << "] embedded -> data URI ("
              << format << ")\n";
    bool ok = run_texconv(opt, tmpIn, tmpOut, format);
    if(ok) {
        std::vector<uint8_t> dtex = read_file(tmpOut);
        std::string dataUri = "data:image/x-dtex;base64," + b64::encode(dtex);
        set_dtex_extension(image, dataUri);
    }
    std::error_code ec;
    fs::remove(tmpIn, ec);
    fs::remove(tmpOut, ec);
    return ok;
}

// ---------------------------------------------------------------------------
// GLB writing
// ---------------------------------------------------------------------------
static void write_glb(const Json& root, const std::vector<uint8_t>& bin,
                      const fs::path& outPath) {
    std::string json;
    json_write(root, json);
    // Pad JSON chunk to 4 bytes with spaces.
    while(json.size() % 4 != 0) json.push_back(' ');

    std::vector<uint8_t> binPadded = bin;
    while(binPadded.size() % 4 != 0) binPadded.push_back(0);

    bool hasBin = !binPadded.empty();
    uint32_t total = 12 + 8 + (uint32_t)json.size();
    if(hasBin) total += 8 + (uint32_t)binPadded.size();

    std::vector<uint8_t> out;
    out.reserve(total);
    wr_u32(out, GLB_MAGIC);
    wr_u32(out, 2); // version
    wr_u32(out, total);

    wr_u32(out, (uint32_t)json.size());
    wr_u32(out, GLB_CHUNK_JSON);
    out.insert(out.end(), json.begin(), json.end());

    if(hasBin) {
        wr_u32(out, (uint32_t)binPadded.size());
        wr_u32(out, GLB_CHUNK_BIN);
        out.insert(out.end(), binPadded.begin(), binPadded.end());
    }

    write_file(outPath, out.data(), out.size());
}

// ---------------------------------------------------------------------------
// Buffer consolidation + strip application
// ---------------------------------------------------------------------------
static void align4(std::vector<uint8_t>& v) {
    while(v.size() % 4 != 0) v.push_back(0);
}

struct StripJob {
    size_t mesh;
    size_t primitive;
    std::vector<uint32_t> indices;
    long long componentType;
};

// Choose an index component type large enough for `maxIndex`.
static long long index_component_type(uint32_t maxIndex) {
    if(maxIndex <= 0xFF) return 5121;    // UNSIGNED_BYTE
    if(maxIndex <= 0xFFFF) return 5123;  // UNSIGNED_SHORT
    return 5125;                         // UNSIGNED_INT
}

static void append_indices(std::vector<uint8_t>& bin, const std::vector<uint32_t>& idx,
                           long long componentType) {
    int cs = component_size(componentType);
    for(uint32_t v : idx) {
        if(cs == 1) bin.push_back((uint8_t)v);
        else if(cs == 2) { bin.push_back(v & 0xFF); bin.push_back((v >> 8) & 0xFF); }
        else wr_u32(bin, v);
    }
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
static void usage() {
    std::cout <<
        "Usage: optimise_gltf <input.gltf|input.glb> [options]\n"
        "\n"
        "Options:\n"
        "  -o, --output <file.glb>   Output path (default: <input>.optimised.glb)\n"
        "  --lod <ratio>             Simplify meshes to this fraction of their\n"
        "                            triangles before stripping, in (0, 1].\n"
        "                            e.g. 0.5 = ~half, 0.25 = ~quarter, 1 = off.\n"
        "  --format <FMT>            Force a texconv format for every texture\n"
        "                            (default: auto - ARGB4444 if the source has\n"
        "                            an alpha channel, otherwise RGB565)\n"
        "  --mipmap                  Pass -m to texconv (generate mipmaps)\n"
        "  --texconv <path>          Path to the texconv binary\n"
        "  -h, --help                Show this help\n"
        "\n"
        "All textures are always output compressed (texconv -c).\n";
}

int main(int argc, char** argv) {
    std::string inputPath;
    std::string outputPath;
    Options opt;
#ifdef DEFAULT_TEXCONV
    opt.texconv = DEFAULT_TEXCONV;
#endif

    for(int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if(i + 1 >= argc) { std::cerr << "Missing value for " << a << "\n"; std::exit(1); }
            return argv[++i];
        };
        if(a == "-h" || a == "--help") { usage(); return 0; }
        else if(a == "-o" || a == "--output") outputPath = next();
        else if(a == "--format") opt.format = next();
        else if(a == "--lod") {
            opt.lod = std::strtof(next().c_str(), nullptr);
            if(!(opt.lod > 0.0f) || opt.lod > 1.0f) {
                std::cerr << "--lod must be in the range (0, 1]\n";
                return 1;
            }
        }
        else if(a == "--mipmap") opt.mipmap = true;
        else if(a == "--texconv") opt.texconv = next();
        else if(!a.empty() && a[0] == '-') { std::cerr << "Unknown option " << a << "\n"; return 1; }
        else if(inputPath.empty()) inputPath = a;
        else { std::cerr << "Unexpected argument " << a << "\n"; return 1; }
    }

    if(inputPath.empty()) { usage(); return 1; }

    if(outputPath.empty()) {
        fs::path in(inputPath);
        outputPath = (in.parent_path() / (in.stem().string() + ".optimised.glb")).string();
    }

    try {
        std::cout << "Loading " << inputPath << "\n";
        GltfDoc doc = load_gltf(inputPath);

        // --- 1. Triangle strip conversion -------------------------------
        std::vector<StripJob> jobs;
        if(auto* meshes = doc.root.find("meshes")) {
            for(size_t mi = 0; mi < meshes->arr.size(); ++mi) {
                Json& mesh = meshes->arr[mi];
                auto* prims = mesh.find("primitives");
                if(!prims) continue;

                // Pre-read every primitive's positions up front. Separate
                // primitives in the same mesh are often separately-modelled
                // pieces (e.g. a railing or window frame) that merely rest
                // against each other without sharing vertices; simplifying
                // one in isolation could collapse a vertex that another
                // primitive is touching, silently opening a gap between
                // them. Gathering all positions first lets each primitive's
                // LOD pass lock vertices that touch its siblings, and a
                // shared weld tolerance keeps that comparison consistent.
                // Positions are gathered unconditionally (not just under
                // --lod): stripify() also needs them, to weld seam vertices
                // for adjacency (see stripify()'s comment).
                std::vector<std::vector<Vec3>> meshPositions(prims->arr.size());
                for(size_t pi = 0; pi < prims->arr.size(); ++pi) {
                    Json& prim = prims->arr[pi];
                    if(prim.get_int("mode", 4) != 4) continue;
                    auto* attrs = prim.find("attributes");
                    long long posAcc = attrs ? attrs->get_int("POSITION", -1) : -1;
                    if(posAcc >= 0) meshPositions[pi] = read_positions(doc, posAcc);
                }
                double meshEps = 0.0;
                {
                    bool any = false;
                    Vec3 lo{}, hi{};
                    for(auto& ps : meshPositions) {
                        for(auto& p : ps) {
                            if(!any) { lo = hi = p; any = true; continue; }
                            lo.x = std::min(lo.x, p.x); lo.y = std::min(lo.y, p.y);
                            lo.z = std::min(lo.z, p.z);
                            hi.x = std::max(hi.x, p.x); hi.y = std::max(hi.y, p.y);
                            hi.z = std::max(hi.z, p.z);
                        }
                    }
                    if(any) meshEps = weld_epsilon(lo, hi);
                }

                for(size_t pi = 0; pi < prims->arr.size(); ++pi) {
                    Json& prim = prims->arr[pi];
                    long long mode = prim.get_int("mode", 4); // default TRIANGLES
                    if(mode != 4) continue;

                    auto* attrs = prim.find("attributes");
                    long long posAcc = attrs ? attrs->get_int("POSITION", -1) : -1;

                    std::vector<uint32_t> tris;
                    long long indexAcc = prim.get_int("indices", -1);
                    if(indexAcc >= 0) {
                        tris = read_indices(doc, indexAcc);
                    } else {
                        // Non-indexed: synthesize a sequential index list.
                        if(posAcc < 0) continue;
                        long long count =
                            doc.root.find("accessors")->arr[(size_t)posAcc].get_int("count");
                        tris.resize((size_t)count);
                        for(long long v = 0; v < count; ++v) tris[(size_t)v] = (uint32_t)v;
                    }
                    if(tris.size() < 3) continue;

                    // Optional LOD simplification (before stripification).
                    if(opt.lod < 1.0f && posAcc >= 0 && !meshPositions[pi].empty()) {
                        std::vector<Vec3>& positions = meshPositions[pi];
                        size_t before = tris.size() / 3;

                        std::vector<Vec3> foreign;
                        for(size_t oi = 0; oi < meshPositions.size(); ++oi) {
                            if(oi == pi) continue;
                            foreign.insert(foreign.end(), meshPositions[oi].begin(),
                                          meshPositions[oi].end());
                        }

                        long long uvAcc = attrs->get_int("TEXCOORD_0", -1);
                        std::vector<std::array<float, 2>> uv = read_uvs(doc, uvAcc);

                        tris = simplify(positions, tris, opt.lod, meshEps, foreign, uv);
                        std::cout << "  mesh[" << mi << "].primitive[" << pi
                                  << "] simplified " << before << " -> "
                                  << (tris.size() / 3) << " triangles\n";
                    }
                    if(tris.size() < 3) continue;

                    std::vector<uint32_t> canon;
                    if(!meshPositions[pi].empty()) {
                        canon = weld_positions(meshPositions[pi], meshEps);
                    }
                    std::vector<uint32_t> strip = stripify(tris, canon);
                    if(strip.empty()) continue;

                    uint32_t maxIndex = 0;
                    for(uint32_t v : strip) maxIndex = std::max(maxIndex, v);

                    long long ct;
                    if(indexAcc >= 0) {
                        ct = doc.root.find("accessors")->arr[(size_t)indexAcc]
                                 .get_int("componentType", 5125);
                        // Ensure the existing type is wide enough (it always is,
                        // since strips only reuse existing vertex indices).
                        if(component_size(ct) < component_size(index_component_type(maxIndex)))
                            ct = index_component_type(maxIndex);
                    } else {
                        ct = index_component_type(maxIndex);
                    }

                    std::cout << "  mesh[" << mi << "].primitive[" << pi << "] "
                              << tris.size() << " indices -> strip of "
                              << strip.size() << "\n";
                    jobs.push_back({mi, pi, std::move(strip), ct});
                }
            }
        }

        // --- 2. Texture conversion --------------------------------------
        size_t converted = 0;
        if(auto* images = doc.root.find("images")) {
            std::cout << "Converting " << images->arr.size() << " image(s)\n";
            for(size_t ii = 0; ii < images->arr.size(); ++ii) {
                if(process_image(doc, images->arr[ii], opt, ii)) ++converted;
            }
        }
        if(converted > 0) {
            ensure_extension_used(doc.root, "SMLT_dtex_texture");
        }

        // --- Consolidate all bufferViews into a single BIN chunk --------
        std::vector<uint8_t> bin;
        if(auto* bviews = doc.root.find("bufferViews")) {
            for(auto& bv : bviews->arr) {
                long long bufIndex = bv.get_int("buffer", 0);
                long long bvOffset = bv.get_int("byteOffset", 0);
                long long bvLen = bv.get_int("byteLength", 0);
                const std::vector<uint8_t>& src = doc.buffers[(size_t)bufIndex];

                align4(bin);
                size_t newOffset = bin.size();
                bin.insert(bin.end(), src.begin() + bvOffset,
                           src.begin() + bvOffset + bvLen);

                bv["buffer"] = Json::number(0);
                bv["byteOffset"] = Json::number((double)newOffset);
            }
        }

        // --- 3. Append strip index data + patch primitives --------------
        if(!jobs.empty()) {
            Json& bviews = doc.root["bufferViews"];
            if(!bviews.is_array()) bviews = Json::array();
            Json& accessors = doc.root["accessors"];
            if(!accessors.is_array()) accessors = Json::array();
            Json& meshes = doc.root["meshes"];

            for(auto& job : jobs) {
                align4(bin);
                size_t offset = bin.size();
                append_indices(bin, job.indices, job.componentType);
                size_t byteLen = bin.size() - offset;

                Json bv = Json::object();
                bv["buffer"] = Json::number(0);
                bv["byteOffset"] = Json::number((double)offset);
                bv["byteLength"] = Json::number((double)byteLen);
                bv["target"] = Json::number(34963); // ELEMENT_ARRAY_BUFFER
                long long bvIndex = (long long)bviews.arr.size();
                bviews.arr.push_back(std::move(bv));

                Json acc = Json::object();
                acc["bufferView"] = Json::number((double)bvIndex);
                acc["byteOffset"] = Json::number(0);
                acc["componentType"] = Json::number((double)job.componentType);
                acc["count"] = Json::number((double)job.indices.size());
                acc["type"] = Json::string("SCALAR");
                long long accIndex = (long long)accessors.arr.size();
                accessors.arr.push_back(std::move(acc));

                Json& prim = meshes.arr[job.mesh].find("primitives")->arr[job.primitive];
                prim["indices"] = Json::number((double)accIndex);
                prim["mode"] = Json::number(5); // TRIANGLE_STRIP
            }
        }

        // --- Rewrite the buffers array to the single consolidated buffer -
        if(bin.empty()) {
            if(doc.root.has("buffers")) doc.root["buffers"] = Json::array();
        } else {
            Json buffers = Json::array();
            Json b = Json::object();
            b["byteLength"] = Json::number((double)bin.size());
            buffers.arr.push_back(std::move(b));
            doc.root["buffers"] = std::move(buffers);
        }

        // --- 4. Write output --------------------------------------------
        write_glb(doc.root, bin, outputPath);
        std::cout << "Wrote " << outputPath << " (" << jobs.size()
                  << " primitive(s) stripped, " << converted
                  << " texture(s) converted)\n";
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
