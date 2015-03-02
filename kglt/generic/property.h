#ifndef PROPERTY_H
#define PROPERTY_H

template<typename Container, typename T>
class Property {
public:
    Property(std::function<T& (void)> getter):
        getter_(getter) {

    }

    inline operator T() const { return getter_(); }
    inline operator T&() const { return getter_(); }

    inline T* operator->() { return &getter_(); }
    inline const T* operator->() const { return &getter_(); }
private:
    std::function<T& (void)> getter_;
};



#endif // PROPERTY_H

