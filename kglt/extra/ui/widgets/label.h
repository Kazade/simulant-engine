#ifndef LABEL_H
#define LABEL_H

#include "../../../generic/managed.h"
#include "../widget.h"

namespace kglt {
namespace extra {
namespace ui {

class Label :
    public Managed<Label>,
    public Widget {
public:
    void set_text(const std::string& text);
};

}
}
}

#endif // LABEL_H
