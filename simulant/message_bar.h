#ifndef MESSAGEBAR_H
#define MESSAGEBAR_H

#include <queue>
#include "generic/managed.h"
#include "types.h"

namespace smlt {

class WindowBase;

class MessageBar : public Managed<MessageBar> {
public:
    MessageBar(WindowBase& parent);
    ~MessageBar();

    void notify_left(const unicode& message);
    void notify_right(const unicode& message);

    void inform(const unicode& message);
    void warn(const unicode& warning);
    void alert(const unicode& alert);

    bool init() override;

private:
    enum MessageType {
        NOTIFY_LEFT = 0,
        NOTIFY_RIGHT,
        INFORM,
        WARN,
        ALERT
    };

    struct Message {
        MessageType type;
        unicode text;
    };

    void display_message(Message next_message);
    void create_stage_and_element();
    void update(float dt);

    WindowBase& window_;
    std::queue<Message> message_queue_;

    OverlayID stage_;
    CameraID camera_;

    float time_message_visible_ = 0.0;

    sig::connection update_conn_;
};

}
#endif // MessageBar_H
