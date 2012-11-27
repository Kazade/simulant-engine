#ifndef PARAGRAPH_H
#define PARAGRAPH_H

#include "../generic/managed.h"

/**
 *  The Paragraph class represents 1 or more lines of text. Positions and widths are
 *  measured in percentages of the parent. (e.g. percentage of the window width/height)
 */

namespace kglt {
namespace extra {

typedef float Percent;
typedef std::wstring Unicode;

class Paragraph :
    public Managed<Paragraph> {

    Paragraph(Percent left, Percent top, Percent max_width, Percent max_height=Percent(100));

    void set_text(const Unicode& text);
    Unicode text() const;
};

}
}

#endif // PARAGRAPH_H
