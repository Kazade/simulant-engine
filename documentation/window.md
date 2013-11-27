# The Window

The Window is the root of all things. The Window class controls access to the Scene, RenderSequence, MessageBar, and resource loading.

## Signals

The following signals are available for the Window class:

 - signal<void ()> signal_frame_started();
 - signal<void ()> signal_frame_finished();
 - signal<void ()> signal_pre_swap();
 - signal<void (double)> signal_step();
 - signal<void ()> signal_shutdown();

# The Message Bar

Sometimes, when writing a game it is necessary to display a short message to the player, this could be something like "Your team mate is under attack", or "Base captured!" etc.

The global MessageBar instance is accessible from the Window instance using the WindowBase::message_bar() method. The MessageBar provides the following messaging functions:
    
 - alert(unicode);
 - warn(unicode);
 - inform(unicode);
 - notify_left(unicode);
 - notify_right(unicode);

alert, warn and inform, display messages at the top-center of the screen with red, yellow and blue background respectively. The notify_left and notify_right methods display messages at the top-left and top-right respectively, without any background colouring.

Calling any of the messaging functions will queue the message for display. One message will be displayed per second, and the bar will disappear once the queue is empty. It will of course beredisplayed if you queue more messages.
