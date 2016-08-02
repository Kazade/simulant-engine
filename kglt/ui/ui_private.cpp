#include <mutex>
#include <thread>
#include <iostream>
#include <kazbase/unicode.h>
#include <kazbase/logging.h>
#include "ui_private.h"
#include "interface.h"

namespace kglt {
namespace ui {

Element ElementImpl::append(const unicode& tag) {


}

float ElementImpl::left() const {

}

float ElementImpl::top() const {

}

float ElementImpl::width() const {

}

float ElementImpl::height() const {

}

void ElementImpl::set_event_callback(const unicode& event_type, std::function<bool (Event)> func) {
    event_callbacks_[event_type] = func;
}

void ElementImpl::set_text(const unicode& text) {

}



}
}
