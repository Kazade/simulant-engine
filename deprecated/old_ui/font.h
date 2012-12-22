#ifndef FONT_H
#define FONT_H

#include <cstdint>
#include <string>
#include <tr1/memory>

#include "generic/identifiable.h"
#include "kaztext/kaztext.h"
#include "types.h"

namespace kglt {

class Scene;

class Font :
    public generic::Identifiable<FontID> {

public:
    typedef std::tr1::shared_ptr<Font> ptr;

    Font(Scene* scene, FontID id);
    ~Font();

    void initialize(const std::string& ttf_path, const uint32_t font_size);
    double string_width_in_pixels(const std::string& str) const;
    uint32_t size() const { return font_size_; }
    KTuint kt_font() const { return kt_font_; } //Underlying kaztext font ID

private:
    KTuint kt_font_;
    uint32_t font_size_;
};

}

#endif // FONT_H
