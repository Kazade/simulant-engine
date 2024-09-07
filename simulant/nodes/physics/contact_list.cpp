#include "contact_list.h"
#include "bounce/dynamics/body.h"
#include "bounce/dynamics/contacts/contact.h"
#include "bounce/dynamics/fixture.h"
#include "physics_body.h"
#include "private.h"

namespace smlt {

Contact ContactListIterator::operator*() {
    assert(list_);
    auto fixtures = list_->body_->bounce_->fixtures;
    b3Fixture* fix = fixtures[fixture_index_]->fixture;
    b3ContactEdge* contacts = fix->GetContactList();

    std::size_t i = 0;
    while(contacts) {
        if(contacts->contact->IsTouching()) {
            if(i == contact_index_) {
                break;
            }
            i++;
        }

        contacts = contacts->next;
    }

    assert(contacts);
    b3Fixture* other = contacts->other;
    Contact c(Fixture((_impl::FixtureData*)fix->GetUserData()),
              Fixture((_impl::FixtureData*)other->GetUserData()));
    return c;
}

ContactListIterator::ContactListIterator(const ContactList* list) :
    list_(list) {

    if(list_->body_->bounce_->fixtures.empty()) {
        list_ = nullptr;
    } else {
        bool ok = false;
        for(auto& fx: list_->body_->bounce_->fixtures) {
            auto c = fx->fixture->GetContactList();
            while(c) {
                if(c->contact->IsTouching()) {
                    ok = true;
                    break;
                }
                c = c->next;
            }
        }
        if(!ok) {
            list_ = nullptr;
        }
    }
}

ContactListIterator& ContactListIterator::operator++() {
    auto fixtures = list_->body_->bounce_->fixtures;
    assert(fixture_index_ < fixtures.size());

    b3Fixture* fix = fixtures[fixture_index_]->fixture;
    auto contacts = fix->GetContactList();

    std::size_t contact_count = 0;
    while(contacts) {
        if(contacts->contact->IsTouching()) {
            ++contact_count;
        }

        contacts = contacts->next;
    }

    contact_index_++;
    if(contact_index_ >= contact_count) {
        fixture_index_++;
        contact_index_ = 0;
        if(fixture_index_ >= fixtures.size()) {
            list_ = nullptr;
        }
    }

    return *this;
}

std::size_t ContactList::count() const {
    std::size_t i = 0;
    for(auto c: *this) {
        _S_UNUSED(c);
        i++;
    }

    return i;
}

smlt::optional<Contact> ContactList::operator[](std::size_t idx) const {
    std::size_t i = 0;
    for(auto c: *this) {
        if(i++ == idx) {
            return c;
        }
    }

    return no_value;
}

} // namespace smlt
