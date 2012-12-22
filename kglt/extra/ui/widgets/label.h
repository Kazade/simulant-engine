#ifndef LABEL_H
#define LABEL_H

#include "../../../generic/identifiable.h"
#include "../../../generic/managed.h"
#include "../widget.h"
#include "../types.h"

namespace kglt {
namespace extra {
namespace ui {

class Label :
    public Managed<Label>,
    public Widget,
    public generic::Identifiable<LabelID> {
public:
    Label(Interface* interface, LabelID id);
    void set_text(const std::string& text);

    std::string text_;
};

}
}
}

#endif // LABEL_H
