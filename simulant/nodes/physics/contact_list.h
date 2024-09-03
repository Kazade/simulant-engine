#pragma once

#include "../../generic/optional.h"
#include "fixture.h"

namespace smlt {

class PhysicsBody;

struct Contact {
    Contact(const Fixture& a, const Fixture& b) :
        fixtures{a, b} {}

    Fixture fixtures[2];
};

class ContactList;

class ContactListIterator {
private:
    friend class ContactList;

    const ContactList* list_ = nullptr;

    ContactListIterator() = default;
    ContactListIterator(const ContactList* list);

    std::size_t fixture_index_ = 0;
    std::size_t contact_index_ = 0;

public:
    bool operator==(const ContactListIterator& rhs) {
        if(list_ == nullptr && rhs.list_ == nullptr) {
            return true;
        }

        return list_ == rhs.list_ && fixture_index_ == rhs.fixture_index_ &&
               contact_index_ == rhs.contact_index_;
    }

    bool operator!=(const ContactListIterator& rhs) {
        return !(*this == rhs);
    }

    ContactListIterator& operator++();
    Contact operator*();
};

class ContactList {
public:
    ContactList(PhysicsBody* body) :
        body_(body) {}

    ContactListIterator begin() const {
        return ContactListIterator(this);
    }
    ContactListIterator end() const {
        return ContactListIterator();
    }

    std::size_t count() const;

    smlt::optional<Contact> operator[](std::size_t idx) const;

private:
    friend class ContactListIterator;
    PhysicsBody* body_;
};

} // namespace smlt
