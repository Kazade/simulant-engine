#ifndef IDENTIFIABLE_H
#define IDENTIFIABLE_H

namespace kglt {
namespace generic {

template<typename IDType>
class Identifiable {
public:
    Identifiable(IDType id):
        id_(id) {}

    virtual ~Identifiable() {}

    IDType id() const { return id_; }
private:
    IDType id_;
};

}
}

#endif // IDENTIFIABLE_H
