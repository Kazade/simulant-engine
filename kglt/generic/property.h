#ifndef PROPERTY_H
#define PROPERTY_H

#include <functional>

template<typename Container, typename T>
class Property {
public:
    Property(Container* _this, T Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T& {
            return self->*member;
        }) {
    }

    Property(Container* _this, T* Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T& {
            return *(self->*member);
        }) {
    }

    Property(Container* _this, std::shared_ptr<T> Container::* member):
        this_(_this),
        getter_([member](Container* self) -> T& {
            return *(self->*member);
        }) {

    }

    Property(Container* _this, std::function<T& (Container*)> getter):
        this_(_this),
        getter_(getter) {

    }

    /*
     *  We can't allow copy construction, because 'this_' will never be initialized
     */
    Property(const Property& rhs) = delete;

    Property operator=(const Property& rhs) {
        assert(this_); //Make sure that this_ was initialized

        getter_ = rhs.getter_;
        // Intentionally don't transfer 'this_'
    }

    inline operator T&() const { return getter_(this_); }
    inline T* operator->() const { return &getter_(this_); }

    T* get() const { return &getter_(this_); }
private:
    Container* this_ = nullptr;
    std::function<T& (Container*)> getter_;

};



#endif // PROPERTY_H

