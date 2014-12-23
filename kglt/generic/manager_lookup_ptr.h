#ifndef MANAGER_LOOKUP_PTR_H
#define MANAGER_LOOKUP_PTR_H

#include <memory>
#include "manager.h"

template<typename Manager, typename ID>
class ManagerLookupPtr {
public:
    ManagerLookupPtr(Manager& manager, ID identifier):
        manager_(manager),
        identifier_(identifier) {}

    ManagerLookupPtr(const ManagerLookupPtr<Manager, ID>& other):
        manager_(other.manager_),
        identifier_(other.identifier_) {

    }

    ManagerLookupPtr& operator=(const ManagerLookupPtr<Manager, ID>&) = delete;

    std::shared_ptr<typename Manager::Type> operator->() {
        return manager_.manager_get(identifier_).lock();
    }

    const std::shared_ptr<typename Manager::Type> operator->() const {
        return manager_.manager_get(identifier_).lock();
    }

    explicit operator bool() const {
        return manager_.manager_contains(identifier_);
    }
private:
    Manager& manager_;
    ID identifier_;
};

#endif // MANAGER_LOOKUP_PTR_H
