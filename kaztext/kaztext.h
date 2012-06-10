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
void ktDrawText(KTfloat x, KTfloat y, const KTwchar* text);
void ktDrawTextCentred(KTfloat x, KTfloat y, const KTwchar* text);
void ktDrawTextWrapped(KTfloat x, KTfloat y, KTfloat width, KTfloat height, const KTwchar* text, KTuint alignment);
void ktDeleteFonts(KTsizei n, const KTuint* fonts);
void ktCacheString(const KTwchar* string);

KTfloat ktGetStringWidth(const KTwchar* text);

#define KT_FONT_HEIGHT 1000

void ktGetIntegerv(KTuint type, KTuint* out);

#endif // KAZTEXT_H_INCLUDED
