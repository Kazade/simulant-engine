#ifndef GLES_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION 1
#endif

#include <algorithm>
#include <cmath>
#include <unistd.h>

#import <AVFoundation/AVFoundation.h>
#import <CoreHaptics/CoreHaptics.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include "ios_window.h"

#include "../../sound/drivers/openal_sound_driver.h"
#include "../../sound/drivers/null_sound_driver.h"
#include "../../application.h"

/* Working with iOS is a bit different to other platforms. UIKit owns the
 * main thread, so ios_main() starts it, and once there's a window the
 * user's app_main() is called from a run loop timer. That's where Simulant's
 * main loop then runs, and IOSWindow::check_events() lets UIKit handle
 * whatever's happened since the last frame by running the run loop
 * briefly. Everything happens on the main thread. */

@class SimulantView;

static int (*APP_MAIN)(int, char*[]) = nullptr;
static int ARGC = 0;
static char** ARGV = nullptr;
static bool STARTED = false;

static SimulantView* VIEW = nil;
static smlt::IOSWindow* SIMULANT_WINDOW = nullptr;

/* The scene's state, which can change before there's a Simulant window */
static bool ACTIVE = false;
static bool BACKGROUND = false;

/* Set when the system takes the audio away (e.g. for a phone call) */
static bool AUDIO_INTERRUPTED = false;
static bool AUDIO_INTERRUPTION_CHANGED = false;

namespace smlt {
const static MouseID TOUCH_MOUSE_ID(0);
}

@interface SimulantView : UIView
@end

@implementation SimulantView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
    if((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = YES;
        self.opaque = YES;

        CAEAGLLayer* layer = (CAEAGLLayer*) self.layer;
        layer.opaque = YES;
        layer.drawableProperties = @{
            kEAGLDrawablePropertyRetainedBacking: @NO,
            kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8
        };
    }
    return self;
}

- (void)didMoveToWindow {
    [super didMoveToWindow];

    /* Render at the screen's full resolution, rather than in points */
    if(self.window) {
        self.contentScaleFactor = self.window.screen.nativeScale;
    }
}

- (void)layoutSubviews {
    [super layoutSubviews];
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_layout_changed();
    }
}

- (void)safeAreaInsetsDidChange {
    [super safeAreaInsetsDidChange];
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_layout_changed();
    }
}

- (void)handleTouches:(NSSet<UITouch*>*)touches phase:(smlt::IOSWindow::TouchPhase)phase {
    if(!SIMULANT_WINDOW) {
        return;
    }

    CGSize size = self.bounds.size;
    for(UITouch* touch in touches) {
        CGPoint point = [touch locationInView:self];

        float x = (size.width > 1.0) ? float(point.x / size.width) : 0.5f;
        float y = (size.height > 1.0) ? float(point.y / size.height) : 0.5f;

        /* Only some devices can tell how hard the screen's pressed */
        float pressure = 1.0f;
        if(touch.maximumPossibleForce > 0.0) {
            pressure = std::min(1.0f, float(touch.force / touch.maximumPossibleForce));
        }

        SIMULANT_WINDOW->on_touch(
            (__bridge const void*) touch, phase,
            std::max(0.0f, std::min(1.0f, x)),
            std::max(0.0f, std::min(1.0f, y)),
            pressure
        );
    }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self handleTouches:touches phase:smlt::IOSWindow::TOUCH_PHASE_BEGAN];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self handleTouches:touches phase:smlt::IOSWindow::TOUCH_PHASE_MOVED];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self handleTouches:touches phase:smlt::IOSWindow::TOUCH_PHASE_ENDED];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self handleTouches:touches phase:smlt::IOSWindow::TOUCH_PHASE_ENDED];
}

@end

@interface SimulantViewController : UIViewController
@end

@implementation SimulantViewController

- (void)loadView {
    VIEW = [[SimulantView alloc] initWithFrame:CGRectZero];
    self.view = VIEW;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

/* So a swipe from an edge goes to the game first, rather than straight to
 * the system */
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

/* Hardware keyboards send their presses to whatever's first responder */
- (BOOL)canBecomeFirstResponder {
    return YES;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self becomeFirstResponder];
}

- (void)handlePresses:(NSSet<UIPress*>*)presses down:(bool)down unhandled:(NSMutableSet<UIPress*>*)unhandled {
    for(UIPress* press in presses) {
        if(press.key && SIMULANT_WINDOW) {
            SIMULANT_WINDOW->on_key((int) press.key.keyCode, down);
        } else {
            [unhandled addObject:press];
        }
    }
}

- (void)pressesBegan:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
    NSMutableSet<UIPress*>* unhandled = [NSMutableSet set];
    [self handlePresses:presses down:true unhandled:unhandled];
    if(unhandled.count) {
        [super pressesBegan:unhandled withEvent:event];
    }
}

- (void)pressesEnded:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
    NSMutableSet<UIPress*>* unhandled = [NSMutableSet set];
    [self handlePresses:presses down:false unhandled:unhandled];
    if(unhandled.count) {
        [super pressesEnded:unhandled withEvent:event];
    }
}

- (void)pressesCancelled:(NSSet<UIPress*>*)presses withEvent:(UIPressesEvent*)event {
    NSMutableSet<UIPress*>* unhandled = [NSMutableSet set];
    [self handlePresses:presses down:false unhandled:unhandled];
    if(unhandled.count) {
        [super pressesCancelled:unhandled withEvent:event];
    }
}

@end

@interface SimulantSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation SimulantSceneDelegate

- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    if(![scene isKindOfClass:[UIWindowScene class]]) {
        return;
    }

    /* There's only ever one view for Simulant to draw to */
    if(VIEW) {
        NSLog(@"Simulant: ignoring an extra scene, only one is supported");
        return;
    }

    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*) scene];
    self.window.rootViewController = [[SimulantViewController alloc] init];
    [self.window makeKeyAndVisible];

    if(!STARTED) {
        STARTED = true;

        /* From a timer rather than dispatch_async, so the main queue isn't
         * blocked while the app runs (and things UIKit dispatches to it
         * still happen while check_events() runs the run loop) */
        [self performSelector:@selector(runApp) withObject:nil afterDelay:0.0];
    }
}

- (void)runApp {
    int result;
    @autoreleasepool {
        result = APP_MAIN(ARGC, ARGV);
    }
    exit(result);
}

- (void)sceneDidBecomeActive:(UIScene*)scene {
    ACTIVE = true;
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_active_changed(true);
    }
}

- (void)sceneWillResignActive:(UIScene*)scene {
    ACTIVE = false;
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_active_changed(false);
    }
}

- (void)sceneDidEnterBackground:(UIScene*)scene {
    BACKGROUND = true;
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_background_changed(true);
    }
}

- (void)sceneWillEnterForeground:(UIScene*)scene {
    BACKGROUND = false;
    if(SIMULANT_WINDOW) {
        SIMULANT_WINDOW->on_background_changed(false);
    }
}

@end

@interface SimulantAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation SimulantAppDelegate

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options {
    if(![[NSBundle mainBundle] objectForInfoDictionaryKey:@"UIApplicationSceneManifest"]) {
        NSLog(@"Simulant: the app's Info.plist has no UIApplicationSceneManifest, so it won't get a window");
    }

    [[NSNotificationCenter defaultCenter]
        addObserverForName:AVAudioSessionInterruptionNotification
        object:nil
        queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification* note) {
            NSNumber* type = note.userInfo[AVAudioSessionInterruptionTypeKey];
            AUDIO_INTERRUPTED = (type.unsignedIntegerValue == AVAudioSessionInterruptionTypeBegan);
            AUDIO_INTERRUPTION_CHANGED = true;
        }
    ];

    return YES;
}

- (UISceneConfiguration*)application:(UIApplication*)application
    configurationForConnectingSceneSession:(UISceneSession*)session
    options:(UISceneConnectionOptions*)options {

    UISceneConfiguration* config = [[UISceneConfiguration alloc]
        initWithName:nil sessionRole:session.role];
    config.delegateClass = [SimulantSceneDelegate class];
    return config;
}

@end

namespace smlt {

int ios_main(int argc, char* argv[], int (*app_main)(int, char*[])) {
    APP_MAIN = app_main;
    ARGC = argc;
    ARGV = argv;

    @autoreleasepool {
        /* So that relative paths (e.g. search paths) are relative to the
         * app bundle, which is where the assets are */
        NSString* resources = [[NSBundle mainBundle] resourcePath];
        if(resources) {
            chdir([resources fileSystemRepresentation]);
        }

        return UIApplicationMain(
            argc, argv, nil, NSStringFromClass([SimulantAppDelegate class])
        );
    }
}

IOSWindow::IOSWindow() {
    /* Apps that quit themselves get rejected from the App Store */
    set_escape_to_quit(false);
}

bool IOSWindow::_init_window() {
    if(!VIEW) {
        S_ERROR("There's no view to draw to. Was the app started with smlt::ios_main()?");
        return false;
    }

    if(context_) {
        S_DEBUG("Not initializing window, already initialized");
        return false;
    }

    EAGLContext* context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if(!context) {
        S_ERROR("Couldn't create an OpenGL ES 2 context");
        return false;
    }

    if(![EAGLContext setCurrentContext:context]) {
        S_ERROR("Couldn't make the context current");
        return false;
    }

    context_ = (__bridge_retained void*) context;

    /* Make sure the view has its size before the framebuffer's made */
    [VIEW layoutIfNeeded];

    if(!create_framebuffer()) {
        return false;
    }

    SIMULANT_WINDOW = this;
    in_background_ = BACKGROUND;

    update_safe_area();

    S_DEBUG("GL initialized, initializing the context");

    return true;
}

bool IOSWindow::_init_renderer(Renderer *renderer) {
    _S_UNUSED(renderer);

    set_has_context(true); //Mark that we have a valid GL context

    /* The window's focused by default, but the app might not be active yet */
    if(!ACTIVE) {
        set_has_focus(false);
    }

    return true;
}

bool IOSWindow::create_framebuffer() {
    EAGLContext* context = (__bridge EAGLContext*) context_;
    CAEAGLLayer* layer = (CAEAGLLayer*) VIEW.layer;

    if(!framebuffer_) {
        glGenFramebuffers(1, &framebuffer_);
        glGenRenderbuffers(1, &color_renderbuffer_);
        glGenRenderbuffers(1, &depth_renderbuffer_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer_);
    if(![context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer]) {
        S_ERROR("Couldn't allocate the colour buffer");
        return false;
    }

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer_
    );

    GLint width = 0, height = 0;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);

    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer_
    );

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        S_ERROR("The framebuffer is incomplete: {0}", status);
        return false;
    }

    /* Left bound for the renderer, which treats it as the default */
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer_);

    S_INFO("Framebuffer is {0}x{1}", width, height);

    set_width(width);
    set_height(height);

    return true;
}

void IOSWindow::destroy_framebuffer() {
    if(framebuffer_) {
        glDeleteFramebuffers(1, &framebuffer_);
        glDeleteRenderbuffers(1, &color_renderbuffer_);
        glDeleteRenderbuffers(1, &depth_renderbuffer_);
    }

    framebuffer_ = color_renderbuffer_ = depth_renderbuffer_ = 0;
}

void IOSWindow::do_swap_buffers() {
    @autoreleasepool {
        EAGLContext* context = (__bridge EAGLContext*) context_;
        if(context && can_render()) {
            glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer_);
            [context presentRenderbuffer:GL_RENDERBUFFER];
        }
    }
}

void IOSWindow::update_safe_area() {
    UIEdgeInsets insets = VIEW.safeAreaInsets;
    CGFloat scale = VIEW.contentScaleFactor;

    auto to_pixels = [scale](CGFloat points) -> uint16_t {
        return (uint16_t) std::ceil(std::max(CGFloat(0), points) * scale);
    };

    SafeArea area;
    area.left = to_pixels(insets.left);
    area.top = to_pixels(insets.top);
    area.right = to_pixels(insets.right);
    area.bottom = to_pixels(insets.bottom);

    if(area != safe_area()) {
        S_INFO(
            "Safe area insets: left {0}, top {1}, right {2}, bottom {3}",
            area.left, area.top, area.right, area.bottom
        );
        set_safe_area(area);
    }
}

void IOSWindow::on_layout_changed() {
    layout_changed_ = true;
}

void IOSWindow::on_active_changed(bool active) {
    S_INFO(active ? "Focus gained" : "Focus lost");
    set_has_focus(active);

    if(!active) {
        // We won't hear about any fingers being lifted now
        release_all_fingers();
    }
}

void IOSWindow::on_background_changed(bool background) {
    S_INFO(background ? "Entered background" : "Entering foreground");

    if(background) {
        /* Nothing can be left for the GPU to do once we're in the
         * background */
        glFinish();
    }

    in_background_ = background;

    // Nothing stops the sound playing while we're in the background
    // otherwise
    if(ALCcontext* context = alcGetCurrentContext()) {
        if(background) {
            alcDevicePauseSOFT(alcGetContextsDevice(context));
        } else if(!AUDIO_INTERRUPTED) {
            alcDeviceResumeSOFT(alcGetContextsDevice(context));
        }
    }
}

void IOSWindow::on_touch(const void* touch, TouchPhase phase, float x, float y, float pressure) {
    int finger_id;

    auto it = finger_ids_.find(touch);
    if(phase == TOUCH_PHASE_BEGAN) {
        if(it != finger_ids_.end()) {
            finger_id = it->second;
        } else {
            finger_id = 0;
            while(std::any_of(finger_ids_.begin(), finger_ids_.end(),
                    [finger_id](const std::pair<const void* const, int>& p) {
                        return p.second == finger_id;
                    })) {
                ++finger_id;
            }

            finger_ids_[touch] = finger_id;
        }
    } else {
        if(it == finger_ids_.end()) {
            return;
        }

        finger_id = it->second;
        if(phase == TOUCH_PHASE_ENDED) {
            finger_ids_.erase(it);
        }
    }

    touch_events_.push_back({finger_id, phase, x, y, pressure});
}

void IOSWindow::on_key(int hid_usage, bool down) {
    key_events_.push_back({hid_usage, down});
}

void IOSWindow::release_all_fingers() {
    for(auto& finger: finger_states_) {
        auto& state = finger.second;
        if(!state.down) {
            continue;
        }

        on_finger_up(finger.first, state.x, state.y);
        if(finger.first == 0) {
            on_mouse_up(
                TOUCH_MOUSE_ID,
                finger.first,
                state.x * float(width()),
                state.y * float(height()),
                true
            );
        }

        state.down = false;
        state.pressure = 0.0f;
    }

    finger_ids_.clear();
    touch_events_.clear();
}

/* The keyboard codes are in the same order as USB HID usages (which is what
 * UIKey gives us) from A to keypad equals, and the modifiers are separate */
static KeyboardCode hid_usage_to_keyboard_code(int usage) {
    static_assert(KEYBOARD_CODE_KP_EQUALS - KEYBOARD_CODE_A == 0x67 - 0x04,
                  "Keyboard codes no longer follow HID usages");

    if(usage >= 0x04 && usage <= 0x67) {
        return (KeyboardCode) (KEYBOARD_CODE_A + (usage - 0x04));
    }

    switch(usage) {
        case 0xE0: return KEYBOARD_CODE_LCTRL;
        case 0xE1: return KEYBOARD_CODE_LSHIFT;
        case 0xE2: return KEYBOARD_CODE_LALT;
        case 0xE3: return KEYBOARD_CODE_LGUI;
        case 0xE4: return KEYBOARD_CODE_RCTRL;
        case 0xE5: return KEYBOARD_CODE_RSHIFT;
        case 0xE6: return KEYBOARD_CODE_RALT;
        case 0xE7: return KEYBOARD_CODE_RGUI;
        default: return KEYBOARD_CODE_NONE;
    }
}

void IOSWindow::check_events() {
    @autoreleasepool {
        /* Let UIKit handle everything that's happened since the last frame.
         * In the background there's nothing to draw, so wait for something
         * to happen rather than spinning. */
        CFTimeInterval wait = (in_background_) ? 0.1 : 0.0;
        while(CFRunLoopRunInMode(kCFRunLoopDefaultMode, wait, TRUE) == kCFRunLoopRunHandledSource) {
            wait = 0.0;
        }
    }

    if(AUDIO_INTERRUPTION_CHANGED) {
        AUDIO_INTERRUPTION_CHANGED = false;

        if(ALCcontext* context = alcGetCurrentContext()) {
            if(AUDIO_INTERRUPTED) {
                S_INFO("Audio interrupted");
                alcDevicePauseSOFT(alcGetContextsDevice(context));
            } else {
                S_INFO("Audio interruption ended");
                [[AVAudioSession sharedInstance] setActive:YES error:nil];
                if(!in_background_) {
                    alcDeviceResumeSOFT(alcGetContextsDevice(context));
                }
            }
        }
    }

    if(layout_changed_ && !in_background_) {
        layout_changed_ = false;

        /* e.g. the device has been turned around */
        uint16_t old_width = width(), old_height = height();
        if(create_framebuffer() && (width() != old_width || height() != old_height)) {
            S_INFO("Window resized to {0}x{1}", width(), height());
        }

        update_safe_area();
    }

    for(auto& evt: key_events_) {
        auto key = hid_usage_to_keyboard_code(evt.hid_usage);
        if(key == KEYBOARD_CODE_NONE) {
            continue;
        }

        if(evt.down) {
            input_state->_handle_key_down(KeyboardID(0), key);
            on_key_down(key, ModifierKeyState());
        } else {
            input_state->_handle_key_up(KeyboardID(0), key);
            on_key_up(key, ModifierKeyState());
        }
    }

    key_events_.clear();

    for(auto& evt: touch_events_) {
        auto pointer = evt.finger_id;
        float x = evt.x;
        float y = 1.0f - evt.y;

        auto& state = finger_states_[pointer];

        if(evt.phase == TOUCH_PHASE_BEGAN) {
            // A finger that never came up (we missed it), so lift it first
            // rather than leaving it down twice
            if(state.down) {
                on_finger_up(pointer, state.x, state.y);
                state.down = false;
            }

            on_finger_down(pointer, x, y, evt.pressure);

            if(pointer == 0) {
                on_mouse_down(
                    TOUCH_MOUSE_ID,
                    pointer,
                    x * float(width()),
                    y * float(height()),
                    true
                );
            }

            state.x = x;
            state.y = y;
            state.pressure = evt.pressure;
            state.down = true;
        } else if(evt.phase == TOUCH_PHASE_ENDED) {
            if(state.down) {
                on_finger_up(pointer, x, y);

                if(pointer == 0) {
                    on_mouse_up(
                        TOUCH_MOUSE_ID,
                        pointer,
                        x * float(width()),
                        y * float(height()),
                        true
                    );
                }

                state.x = x;
                state.y = y;
                state.pressure = 0.0f;
                state.down = false;
            }
        } else if(evt.phase == TOUCH_PHASE_MOVED) {
            if(!state.down) {
                continue;
            }

            float dx = x - state.x;
            float dy = y - state.y;

            on_finger_motion(pointer, x, y, dx, dy);

            if(pointer == 0) {
                on_mouse_move(
                    TOUCH_MOUSE_ID,
                    x * float(width()),
                    y * float(height()),
                    true
                );
            }

            state.x = x;
            state.y = y;
            state.pressure = evt.pressure;
        }
    }

    touch_events_.clear();
}

bool IOSWindow::has_haptics() const {
    if(haptics_checked_) {
        return haptic_engine_ != nullptr;
    }
    haptics_checked_ = true;

    @autoreleasepool {
        if([CHHapticEngine capabilitiesForHardware].supportsHaptics) {
            NSError* error = nil;
            CHHapticEngine* engine = [[CHHapticEngine alloc] initAndReturnError:&error];
            if(engine) {
                engine.playsHapticsOnly = YES;

                /* It stops itself when it's not used for a while, and is
                 * started again when it's next needed */
                engine.autoShutdownEnabled = YES;

                __weak CHHapticEngine* weak_engine = engine;
                engine.resetHandler = ^{
                    [weak_engine startAndReturnError:nil];
                };

                haptic_engine_ = (__bridge_retained void*) engine;
            } else {
                S_WARN("Couldn't create the haptic engine: {0}", error.localizedDescription.UTF8String);
            }
        }
    }

    S_INFO("Haptics: {0}", haptic_engine_ ? "yes" : "no");
    return haptic_engine_ != nullptr;
}

void IOSWindow::start_haptics(NormalizedFloat strength, const Seconds& duration) {
    if(!has_haptics() || strength <= 0.0f) {
        return;
    }

    stop_haptics();

    @autoreleasepool {
        CHHapticEngine* engine = (__bridge CHHapticEngine*) haptic_engine_;

        NSError* error = nil;
        if(![engine startAndReturnError:&error]) {
            S_WARN("Couldn't start the haptic engine: {0}", error.localizedDescription.UTF8String);
            return;
        }

        CHHapticEventParameter* intensity = [[CHHapticEventParameter alloc]
            initWithParameterID:CHHapticEventParameterIDHapticIntensity
            value:std::min(1.0f, float(strength))];

        /* Continuous events can be at most 30 seconds */
        NSTimeInterval seconds = std::max(0.001f, std::min(30.0f, duration.to_float()));

        CHHapticEvent* event = [[CHHapticEvent alloc]
            initWithEventType:CHHapticEventTypeHapticContinuous
            parameters:@[intensity]
            relativeTime:0
            duration:seconds];

        CHHapticPattern* pattern = [[CHHapticPattern alloc]
            initWithEvents:@[event] parameters:@[] error:&error];

        id<CHHapticPatternPlayer> player = (pattern) ?
            [engine createPlayerWithPattern:pattern error:&error] : nil;

        if(!player || ![player startAtTime:CHHapticTimeImmediate error:&error]) {
            S_WARN("Couldn't play haptics: {0}", error.localizedDescription.UTF8String);
            return;
        }

        haptic_player_ = (__bridge_retained void*) player;
    }
}

void IOSWindow::stop_haptics() {
    if(!haptic_player_) {
        return;
    }

    @autoreleasepool {
        id<CHHapticPatternPlayer> player = (__bridge_transfer id<CHHapticPatternPlayer>) haptic_player_;
        haptic_player_ = nullptr;
        [player stopAtTime:CHHapticTimeImmediate error:nil];
    }
}

void IOSWindow::destroy_window() {
    S_DEBUG("Destroying the EAGL context");

    stop_haptics();

    if(haptic_engine_) {
        CHHapticEngine* engine = (__bridge_transfer CHHapticEngine*) haptic_engine_;
        haptic_engine_ = nullptr;
        [engine stopWithCompletionHandler:nil];
    }

    if(context_) {
        destroy_framebuffer();

        [EAGLContext setCurrentContext:nil];
        CFRelease(context_);
        context_ = nullptr;
    }

    SIMULANT_WINDOW = nullptr;
}

void IOSWindow::initialize_input_controller(InputState &controller) {
    KeyboardDeviceInfo keyboard;
    keyboard.id = 0;
    keyboard.type = KEYBOARD_TYPE_SOFTWARE;
    controller._update_keyboard_devices({keyboard});
}

std::shared_ptr<SoundDriver> IOSWindow::create_sound_driver(const std::string& from_config) {
    const char* from_env = std::getenv("SIMULANT_SOUND_DRIVER");

    std::string selected = (from_env) ? from_env :
                               (from_config.empty()) ? "openal" : from_config;

    if(selected == "null") {
        S_DEBUG("Null sound driver activated");
        return std::make_shared<NullSoundDriver>(this);
    } else {
        if(selected != "openal") {
            S_WARN("Unknown sound driver ({0}) falling back to OpenAL", selected);
        }
        S_DEBUG("OpenAL sound driver activated");
        return std::make_shared<OpenALSoundDriver>(this);
    }
}

}
