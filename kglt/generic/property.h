#ifndef PROPERTY_H
#define PROPERTY_H

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

    Property(const Property& rhs):
        getter_(rhs.getter_) {
        // Intentionally don't transfer 'this_'
    }

    Property operator=(const Property& rhs) {
        getter_ = rhs.getter_;
        // Intentionally don't transfer 'this_'
    }

    inline operator T&() const { return getter_(this_); }
    inline T* operator->() const { return &getter_(this_); }
    inline operator T() const { return getter_(this_); }
private:
    Container* this_ = nullptr;
    std::function<T& (Container*)> getter_;
};



#endif // PROPERTY_H

