
#include <map>
#include <vector>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/ftglyph.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "kaztext.h"
#include "utf8/checked.h"

PFNGLCREATESHADERPROC glCreateShader = (PFNGLCREATESHADERPROC) glXGetProcAddress((GLubyte*) "glCreateShader");
PFNGLCREATEPROGRAMPROC glCreateProgram = (PFNGLCREATEPROGRAMPROC) glXGetProcAddress((GLubyte*) "glCreateProgram");
PFNGLSHADERSOURCEPROC glShaderSource = (PFNGLSHADERSOURCEPROC) glXGetProcAddress((GLubyte*) "glShaderSource");
PFNGLCOMPILESHADERPROC glCompileShader = (PFNGLCOMPILESHADERPROC) glXGetProcAddress((GLubyte*) "glCompileShader");
PFNGLGETSHADERIVPROC glGetShaderiv = (PFNGLGETSHADERIVPROC) glXGetProcAddress((GLubyte*) "glGetShaderiv");
PFNGLGETPROGRAMIVPROC glGetProgramiv = (PFNGLGETPROGRAMIVPROC) glXGetProcAddress((GLubyte*) "glGetProgramiv");
PFNGLATTACHSHADERPROC glAttachShader = (PFNGLATTACHSHADERPROC) glXGetProcAddress((GLubyte*) "glAttachShader");
PFNGLLINKPROGRAMPROC glLinkProgram = (PFNGLLINKPROGRAMPROC) glXGetProcAddress((GLubyte*) "glLinkProgram");
PFNGLUSEPROGRAMPROC glUseProgram = (PFNGLUSEPROGRAMPROC) glXGetProcAddress((GLubyte*) "glUseProgram");
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) glXGetProcAddress((GLubyte*) "glEnableVertexAttribArray");
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) glXGetProcAddress((GLubyte*) "glDisableVertexAttribArray");
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) glXGetProcAddress((GLubyte*) "glVertexAttribPointer");
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) glXGetProcAddress((GLubyte*) "glBindAttribLocation");
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) glXGetProcAddress((GLubyte*) "glGetUniformLocation");
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) glXGetProcAddress((GLubyte*) "glUniformMatrix4fv");
PFNGLUNIFORM1IPROC glUniform1i = (PFNGLUNIFORM1IPROC) glXGetProcAddress((GLubyte*) "glUniform1i");

static KTuint next_font_id = 0;

KTuint get_next_font_id() {
    return ++next_font_id;
}

static float projection[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

static float modelview[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

void ktSetProjectionMatrix(float* mat4) {
    for(int i = 0; i < 16; ++i) {
        projection[i] = mat4[i];
    }
}

void ktSetModelviewMatrix(float* mat4) {
    for(int i = 0; i < 16; ++i) {
        modelview[i] = mat4[i];
    }
}

static std::string vertex_shader() {
    static std::string vert = ""
        "#version 120\n"
        ""
        "attribute vec3 vertex_position;\n"
        "attribute vec2 vertex_texcoord_1;\n"
        "attribute vec4 vertex_diffuse;\n"
        ""
        "uniform mat4 modelview_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        ""
        "varying vec2 fragment_texcoord_1;\n"
        "varying vec4 fragment_diffuse;\n"
        ""
        "void main() { \n"
        "   gl_Position = projection_matrix * modelview_matrix * vec4(vertex_position, 1.0);\n"
        "   fragment_texcoord_1 = vertex_texcoord_1;\n"
        "   fragment_diffuse = vertex_diffuse;\n"
        "}"
    "";

    return vert;
}

static std::string fragment_shader() {
    static std::string frag = ""
        "#version 120\n"
        ""
        "uniform sampler2D texture_1;\n"
        ""
        "varying vec2 fragment_texcoord_1;\n"
        "varying vec4 fragment_diffuse;\n"
        ""
        "void main() { \n"
        "   float a = texture2D(texture_1, fragment_texcoord_1.st).a;\n"
        "   gl_FragColor = vec4(fragment_diffuse.rgb, a * fragment_diffuse.a);\n"
        "}"
    "";

    return frag;
}

static GLuint program_id = 0;
static GLuint vertex_shader_id = 0;
static GLuint frag_shader_id = 0;
static GLint proj_location = 0;
static GLint mod_location = 0;
static GLint tex_location = 0;

void compile_shader() {
    vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    program_id = glCreateProgram();

    const char* vert_shader = vertex_shader().c_str();
    glShaderSource(vertex_shader_id, 1, &vert_shader, NULL);

    const char* frag_shader = fragment_shader().c_str();
    glShaderSource(frag_shader_id, 1, &frag_shader, NULL);

    glCompileShader(vertex_shader_id);
    glCompileShader(frag_shader_id);

    GLint compiled = 0;
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &compiled);
    assert(compiled);

    glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &compiled);
    assert(compiled);

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, frag_shader_id);

    glLinkProgram(program_id);

    glBindAttribLocation(program_id, 0, "vertex_position");
    glBindAttribLocation(program_id, 1, "vertex_texcoord_1");
    glBindAttribLocation(program_id, 2, "vertex_diffuse");

    glLinkProgram(program_id);

    GLint linked = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked);
    assert(linked);

    glUseProgram(program_id);

    proj_location = glGetUniformLocation(program_id, "projection_matrix");
    mod_location = glGetUniformLocation(program_id, "modelview_matrix");
    tex_location = glGetUniformLocation(program_id, "texture_1");

    assert(proj_location > -1);
    assert(mod_location > -1);
    assert(tex_location > -1);
}

class KTFont {
public:
    typedef boost::shared_ptr<KTFont> ptr;

    struct Char {
        uint32_t width;
        uint32_t height;
        uint32_t texture_width;
        uint32_t texture_height;
        float advance_x;
        float advance_y;
        float tex_coord_x;
        float tex_coord_y;
        float left;
        float top;
    };

    KTFont(const std::string& ttf, const int font_size);
    bool load();
    bool generate_glyph_texture(wchar_t ch);

    int get_size() const { return font_size_; }
    float get_char_advance_y(wchar_t ch) { return char_properties_[ch].advance_y; }
    float get_char_advance_x(wchar_t ch) { return char_properties_[ch].advance_x; }
    float get_char_left(wchar_t ch) { return char_properties_[ch].left; }
    float get_char_top(wchar_t ch) { return char_properties_[ch].top; }

    GLuint get_char_texture(wchar_t ch) { return textures_[ch]; }

    uint32_t get_char_width(wchar_t ch) { return char_properties_[ch].width; }
    uint32_t get_char_height(wchar_t ch) { return char_properties_[ch].height; }
    float get_char_tex_coord_x(wchar_t ch) { return char_properties_[ch].tex_coord_x; }
    float get_char_tex_coord_y(wchar_t ch) { return char_properties_[ch].tex_coord_y; }
private:
    std::string ttf_;
    int font_size_;
    FT_Face face_;

    std::map<wchar_t, GLuint> textures_;
    std::map<wchar_t, Char> char_properties_;
};

static std::map<uint32_t, KTFont::ptr> fonts_;
static uint32_t current_font_;


struct FreeTypeInitializer {
    FreeTypeInitializer() {
        FT_Init_FreeType(&ftlib);
    }

    ~FreeTypeInitializer() {
        FT_Done_FreeType(ftlib);
    }

    FT_Library ftlib;
};

FreeTypeInitializer ft;

KTFont::KTFont(const std::string& ttf, const int font_size):
ttf_(ttf),
font_size_(font_size) {

}

static uint32_t pow2(uint32_t i) {
    if(i == 1) return 2;

    uint32_t result;

    for(result = 1; result < i; result <<= 1);
    return result;
}

bool KTFont::generate_glyph_texture(wchar_t ch) {
    if(textures_[ch] > 0) return true;

    if(FT_Load_Glyph(face_, FT_Get_Char_Index(face_, ch), FT_LOAD_DEFAULT) != 0) {
        return false;
    }

    FT_Glyph glyph;
    if(FT_Get_Glyph(face_->glyph, &glyph) != 0) {
        return false;
    }

    FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) glyph;
    FT_Bitmap& bitmap = bitmap_glyph->bitmap;

    char_properties_[ch].width = bitmap.width;
    char_properties_[ch].height = bitmap.rows;
    char_properties_[ch].texture_width = pow2(bitmap.width);
    char_properties_[ch].texture_height = pow2(bitmap.rows);
    char_properties_[ch].left = bitmap_glyph->left;
    char_properties_[ch].top = bitmap_glyph->top;

    uint32_t tex_w = char_properties_[ch].texture_width;
    uint32_t tex_h = char_properties_[ch].texture_height;

    std::vector<GLubyte> data(tex_w * tex_h * 4);
    for(KTuint j = 0; j < tex_h; ++j) {
        for(KTuint i = 0; i < tex_w; ++i) {
            int idx = 4 * (i + j * tex_w);
            data[idx] = data[idx+1] = data[idx+2] = data[idx+3] = (i >= (KTuint)bitmap.width || j >= (KTuint)bitmap.rows) ? 0 : bitmap.buffer[i + bitmap.width * j];
        }
    }

    glGenTextures(1, &textures_[ch]);
    glBindTexture(GL_TEXTURE_2D, textures_[ch]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_w, tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    char_properties_[ch].advance_x = (float) (face_->glyph->advance.x >> 6);
    char_properties_[ch].advance_y = (float) ((face_->glyph->metrics.horiBearingY - face_->glyph->metrics.height) >> 6);
    char_properties_[ch].tex_coord_x = float(bitmap.width) / float(tex_w);
    char_properties_[ch].tex_coord_y = float(bitmap.rows) / float(tex_h);

    return true;
}

bool KTFont::load() {
    if(FT_New_Face(ft.ftlib, ttf_.c_str(), 0, &face_) != 0) {
        return false;
    }

    FT_Set_Char_Size(face_, font_size_ << 6, font_size_ << 6, 96, 96);

    //Cache the common ascii chars to prevent slowdown, perhaps a better way would be
    //to pre-generate strings e.g.
    /*
        ktCacheString(L"This is my string");
    */
    for(wchar_t i = 32; i <= 127; ++i) {
        assert(generate_glyph_texture(i));
    }

    return true;
}

void ktGenFonts(uint32_t n, uint32_t* fonts) {
    for(uint32_t i = 0; i < n; ++i) {
        uint32_t new_id = get_next_font_id();
        fonts_[new_id] = KTFont::ptr();
        fonts[i] = new_id;
    }
}

void ktDeleteFonts(KTsizei n, const KTuint* fonts) {
    for(uint32_t i = 0; i < n; ++i) {
        fonts_.erase(fonts[i]);
    }
}

void ktBindFont(KTuint n) {        
    assert(fonts_.find(n) != fonts_.end() && "Invalid font id");
    current_font_ = n;
}

void ktLoadFont(const char* name, KTsizei size) {
    fonts_[current_font_] = KTFont::ptr(new KTFont(name, size));
    assert(fonts_[current_font_]->load());
}

void ktDrawText(float x, float y, const KTchar* text_in) {
    if(!program_id) {
        compile_shader();
    }

    glPushAttrib(GL_ENABLE_BIT);

    std::string text_utf8(text_in);
    std::vector<uint32_t> text;
    utf8::utf8to32(text_utf8.begin(), text_utf8.end(), std::back_inserter(text));

    KTFont::ptr font = fonts_.at(current_font_);

    std::vector<float> positions;
    std::vector<float> texcoords;
    std::vector<float> colours;

    positions.resize(3 * 4, 0);
    texcoords.resize(2 * 4, 0);
    colours.resize(4 * 4, 1.0);

    float x_offset = x;
    float y_offset = y;

    assert(program_id);

    glUseProgram(program_id);

    glUniformMatrix4fv(proj_location, 1, false, (GLfloat*)projection);
    glUniformMatrix4fv(mod_location, 1, false, (GLfloat*)modelview);
    glUniform1i(tex_location, 0);

    glClientActiveTexture(GL_TEXTURE0);

    for(std::vector<uint32_t>::const_iterator it = text.begin(); it != text.end(); ++it) {
        uint32_t ch = (*it);

        font->generate_glyph_texture(ch);

        //Calculate the position for this char
        float this_x_offset = x_offset + font->get_char_left(ch);
        float this_y_offset = y_offset + (font->get_char_top(ch) - font->get_char_height(ch));

        positions[(0 * 3) + 0] = this_x_offset; //X
        positions[(0 * 3) + 1] = this_y_offset + font->get_char_height(ch); //Y
        texcoords[(0 * 2) + 0] = 0.0; //U
        texcoords[(0 * 2) + 1] = 0.0; //V

        positions[(1 * 3) + 0] = this_x_offset; //X
        positions[(1 * 3) + 1] = this_y_offset; //Y
        texcoords[(1 * 2) + 0] = 0.0; //U
        texcoords[(1 * 2) + 1] = font->get_char_tex_coord_y(ch); //V

        positions[(2 * 3) + 0] = this_x_offset + font->get_char_width(ch); //X
        positions[(2 * 3) + 1] = this_y_offset; //Y
        texcoords[(2 * 2) + 0] = font->get_char_tex_coord_x(ch); //U
        texcoords[(2 * 2) + 1] = font->get_char_tex_coord_y(ch); //V

        positions[(3 * 3) + 0] = this_x_offset + font->get_char_width(ch); //X
        positions[(3 * 3) + 1] = this_y_offset + font->get_char_height(ch); //Y
        texcoords[(3 * 2) + 0] = font->get_char_tex_coord_x(ch); //U
        texcoords[(3 * 2) + 1] = 0.0; //V

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, &positions[0]);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, &texcoords[0]);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, &colours[0]);

        glBindTexture(GL_TEXTURE_2D, font->get_char_texture(ch));
        glDrawArrays(GL_QUADS, 0, 4);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        x_offset += font->get_char_advance_x(ch); //Move right for the next character
    }
    glPopAttrib();
}

void ktCacheString(const KTwchar* s) {
    std::wstring text(s);

    for(std::wstring::size_type i = 0; i < text.length(); ++i) {
        fonts_[current_font_]->generate_glyph_texture(i);
    }
}

KTfloat ktStringWidthInPixels(const KTchar* text_in) {
    KTFont::ptr font = fonts_[current_font_];
    
    std::string text_utf8(text_in);
    std::vector<uint32_t> text;
    utf8::utf8to32(text_utf8.begin(), text_utf8.end(), std::back_inserter(text));
    int len = 0;
    for(std::vector<uint32_t>::const_iterator it = text.begin(); it != text.end(); ++it) {
        uint32_t ch = (*it);
        font->generate_glyph_texture(ch);
        len += font->get_char_advance_x(ch);
    }
    return (float) len;
}

void ktDrawTextCentred(float x, float y, const KTchar* text) {
    KTfloat length = ktStringWidthInPixels(text) / 2.0f;
    ktDrawText(x - int(length), y, text);
}

void ktDrawTextWrapped(KTfloat x, KTfloat y, KTfloat width, KTfloat height, const KTchar* text, KTuint alignment) {
	KTuint line_height;
	ktGetIntegerv(KT_FONT_HEIGHT, &line_height);
	line_height *= 1.5f; //FIXME: HACK, need to get actual line height from FT
	
	assert(height >= line_height);
	
	KTfloat cx = 0.0f, cy = line_height;

    std::string text_utf8(text);
    std::string buffer;
    std::vector<std::string> lines;
    KTuint last_space = 0;
    std::string::iterator it = text_utf8.begin();
    while(it != text_utf8.end()) {
        uint32_t c = utf8::next(it, text_utf8.end());

         KTfloat char_width = fonts_[current_font_]->get_char_advance_x(c);
        if(c == L'\n') {
            lines.push_back(buffer);
            buffer.clear();
            cx = 0.0f;
            cy += line_height;
            continue;
        }

        if(cx + char_width >= width) {
            std::string new_line(buffer.begin(), buffer.end() + last_space);
            std::string remainder(buffer.begin() + last_space, buffer.end());
            lines.push_back(new_line);

            //Append this char to remainder
            utf8::append(c, std::back_inserter(remainder));

            std::string::iterator it2 = remainder.begin();
            while(it2 != remainder.end()) {
                uint32_t rc = utf8::next(it2, remainder.end());
                cx += fonts_[current_font_]->get_char_advance_x(rc);
            }

            buffer = remainder;
            cy += line_height;

            if(cy >= height) {
                //If we have gone past the available height, clear the temp string and the add
                //three dots (...) to the finally displayed line
                buffer.clear();
                std::string last_line = lines[lines.size() - 1];
                std::string new_last_line(last_line.begin(), last_line.begin() + (last_line.size() - 3));
                new_last_line += "...";
                lines[lines.size() - 1] = new_last_line;
                break;
            }
        } else {
            utf8::append(c, std::back_inserter(buffer));
            cx += char_width;
            if(c == L' ') {
                last_space = buffer.length();
            }
        }
    }

    if(!buffer.empty()) {
        lines.push_back(buffer);
	}
	
    float y_offset = 0.0f;
    for(KTuint i = 0; i < lines.size(); ++i) {
        lines[i] = boost::algorithm::trim_copy(lines[i]);
        y_offset += line_height;
        if(alignment == KT_ALIGN_LEFT) {
            ktDrawText(x, y_offset, lines[i].c_str());
        } else if(alignment == KT_ALIGN_CENTRE) {
            ktDrawTextCentred(x + (width / 2), y_offset, lines[i].c_str());
        } else {
            assert(0 && "Right align not implemented");
        }
    }
}

void ktGetIntegerv(KTuint type, KTuint* out) {
    KTFont::ptr font = fonts_[current_font_];
	if(type == KT_FONT_HEIGHT) {
		*out = font->get_size();
	}
}
