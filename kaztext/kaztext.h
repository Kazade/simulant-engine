#ifndef KAZTEXT_H_INCLUDED
#define KAZTEXT_H_INCLUDED

#include <string>

typedef unsigned int KTsizei;
typedef unsigned int KTuint;
typedef char KTchar;
typedef wchar_t KTwchar;
typedef float KTfloat;

#define KT_ALIGN_LEFT 0
#define KT_ALIGN_CENTRE 1
#define KT_ALIGN_RIGHT 2

void ktGenFonts(KTsizei n, KTuint* fonts);
void ktBindFont(KTuint font);
void ktLoadFont(const KTchar* filename, const KTsizei font_size);
void ktDrawText(KTfloat x, KTfloat y, const KTchar* text);
void ktDrawTextCentred(KTfloat x, KTfloat y, const KTchar* text);
void ktDrawTextWrapped(KTfloat x, KTfloat y, KTfloat width, KTfloat height, const KTchar* text, KTuint alignment);
void ktDeleteFonts(KTsizei n, const KTuint* fonts);
void ktCacheString(const KTwchar* string);

uint32_t ktStringLength(const KTchar* utf8_text);
KTfloat ktStringWidthInPixels(const KTchar* text);

void ktSetProjectionMatrix(float* mat4);
void ktSetModelviewMatrix(float* mat4);
void ktSetColour(float r, float g, float b, float a);

#define KT_FONT_HEIGHT 1000

void ktGetIntegerv(KTuint type, KTuint* out);

#endif // KAZTEXT_H_INCLUDED
