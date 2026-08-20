#include "keypresscatcher.h"

#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>

#include <dispatch/dispatch.h>

namespace
{
    constexpr const char* kInputMonitoringGrantedStateSettingKey = "permissions/input_monitoring_granted";

    NSString* Key(const char* key)
    {
        return [NSString stringWithUTF8String:key];
    }

    bool HasInputMonitoringPermission()
    {
        return CGPreflightListenEventAccess();
    }

    void SelectNextInputSource()
    {
        const void* keys[] = {
            kTISPropertyInputSourceIsEnabled,
            kTISPropertyInputSourceIsSelectCapable,
            kTISPropertyInputSourceCategory
        };
        const void* values[] = {
            kCFBooleanTrue,
            kCFBooleanTrue,
            kTISCategoryKeyboardInputSource
        };
        CFDictionaryRef filter = CFDictionaryCreate(kCFAllocatorDefault,
                                                     keys,
                                                     values,
                                                     3,
                                                     &kCFTypeDictionaryKeyCallBacks,
                                                     &kCFTypeDictionaryValueCallBacks);
        CFArrayRef sources = TISCreateInputSourceList(filter, false);
        if (filter != nullptr)
        {
            CFRelease(filter);
        }

        if (sources == nullptr || CFArrayGetCount(sources) == 0)
        {
            if (sources != nullptr)
            {
                CFRelease(sources);
            }
            return;
        }

        TISInputSourceRef current = TISCopyCurrentKeyboardInputSource();
        CFIndex count = CFArrayGetCount(sources);
        CFIndex currentIndex = -1;
        for (CFIndex index = 0; index < count; ++index)
        {
            TISInputSourceRef source = static_cast<TISInputSourceRef>(
                const_cast<void*>(CFArrayGetValueAtIndex(sources, index)));
            if (current != nullptr && CFEqual(current, source))
            {
                currentIndex = index;
                break;
            }
        }

        CFIndex nextIndex = currentIndex >= 0 ? (currentIndex + 1) % count : 0;
        TISInputSourceRef nextSource = static_cast<TISInputSourceRef>(
            const_cast<void*>(CFArrayGetValueAtIndex(sources, nextIndex)));
        if (nextSource != nullptr)
        {
            OSStatus status = TISSelectInputSource(nextSource);
            if (status != noErr)
            {
                NSLog(@"CommandShift: TISSelectInputSource failed with status %d", static_cast<int>(status));
            }
        }

        if (current != nullptr)
        {
            CFRelease(current);
        }
        CFRelease(sources);
    }
}

KeyPressCatcher::KeyPressCatcher(
    NSUserDefaults* defaults,
    std::function<void(const std::string& title, const std::string& message)> showMessageCallback)
: m_showMessageCallback(std::move(showMessageCallback))
, m_defaults(defaults)
{
    NSInteger storedKey = [m_defaults integerForKey:Key(CS::secondShortcutKeySettingKeyword)];
    if (CS::SecondShortcutKeyEnum::_FirstElem < storedKey &&
        storedKey < CS::SecondShortcutKeyEnum::_LastElem)
    {
        m_secondShortcutKey = static_cast<CS::SecondShortcutKeyEnum>(storedKey);
    }

    NSString* permissionStateKey = Key(kInputMonitoringGrantedStateSettingKey);
    bool hadStoredPermissionState = [m_defaults objectForKey:permissionStateKey] != nil;
    bool hadPermissionBefore = hadStoredPermissionState && [m_defaults boolForKey:permissionStateKey];

    m_input_monitoring_granted = HasInputMonitoringPermission();
    requestInputMonitoringPermission();
    m_successfully_started = m_input_monitoring_granted && init();

    if (!m_input_monitoring_granted)
    {
        m_showMessageCallback(CS::setupInputMonitoringTitle, CS::setupInputMonitoringMessage);
        m_notified_missing_input_monitoring = true;
    }
    else if (!hadPermissionBefore)
    {
        notifyAboutSuccessfulStart();
    }

    [m_defaults setBool:m_input_monitoring_granted forKey:permissionStateKey];
    loop();
}

KeyPressCatcher::~KeyPressCatcher()
{
    [m_defaults setInteger:static_cast<NSInteger>(m_secondShortcutKey)
                    forKey:Key(CS::secondShortcutKeySettingKeyword)];
    stopEventTap();
}

void KeyPressCatcher::ScheduleLoop(void* context)
{
    static_cast<KeyPressCatcher*>(context)->loop();
}

void KeyPressCatcher::setSecondShortcutKey(CS::SecondShortcutKeyEnum keyValue)
{
    m_secondShortcutKey = keyValue;
}

CS::SecondShortcutKeyEnum KeyPressCatcher::getSecondShortcutKey() const
{
    return m_secondShortcutKey;
}

void KeyPressCatcher::notifyAboutSuccessfulStart()
{
    m_showMessageCallback(CS::allGoodTitle, CS::allGoodMessage);
}

void KeyPressCatcher::loop()
{
    NSString* permissionStateKey = Key(kInputMonitoringGrantedStateSettingKey);
    bool hadPermission = m_input_monitoring_granted;
    m_input_monitoring_granted = HasInputMonitoringPermission();

    if (m_input_monitoring_granted)
    {
        m_requested_input_monitoring = false;
        m_notified_missing_input_monitoring = false;
        if (!hadPermission)
        {
            notifyAboutSuccessfulStart();
        }
        if (!m_successfully_started)
        {
            m_successfully_started = init();
        }
    }
    else
    {
        if (hadPermission || !m_notified_missing_input_monitoring)
        {
            m_showMessageCallback(hadPermission ? CS::inputMonitoringLostTitle : CS::setupInputMonitoringTitle,
                                  hadPermission ? CS::inputMonitoringLostMessage : CS::setupInputMonitoringMessage);
            m_notified_missing_input_monitoring = true;
        }
        stopEventTap();
        m_successfully_started = false;
        requestInputMonitoringPermission();
    }

    [m_defaults setBool:m_input_monitoring_granted forKey:permissionStateKey];
    dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(NSEC_PER_SEC)),
                     dispatch_get_main_queue(),
                     this,
                     ScheduleLoop);
}

void KeyPressCatcher::requestInputMonitoringPermission()
{
    if (m_input_monitoring_granted || m_requested_input_monitoring)
    {
        return;
    }

    m_requested_input_monitoring = true;
    CGRequestListenEventAccess();
    m_input_monitoring_granted = HasInputMonitoringPermission();
}

void KeyPressCatcher::sendSystemDefaultChangeLanguageShortcut()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        SelectNextInputSource();
    });
}

void KeyPressCatcher::noteUnrelatedInput()
{
    if (m_combo_armed)
    {
        m_combo_used_with_other_input = true;
    }
}

void KeyPressCatcher::handleModifierKeysStatusChange(bool shiftPressedDown, bool secondKeyPressedDown)
{
    bool comboActive = shiftPressedDown && secondKeyPressedDown;
    if (comboActive)
    {
        if (!m_combo_armed)
        {
            m_combo_armed = true;
            m_combo_used_with_other_input = false;
        }
        return;
    }

    if (!m_combo_armed)
    {
        return;
    }

    m_combo_armed = false;
    if (!m_combo_used_with_other_input)
    {
        sendSystemDefaultChangeLanguageShortcut();
    }
}

CGEventRef KeyPressCatcher::EventTapCallback(CGEventTapProxy proxy,
                                             CGEventType type,
                                             CGEventRef event,
                                             void* context)
{
    (void)proxy;
    auto catcher = static_cast<KeyPressCatcher*>(context);
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput)
    {
        CGEventTapEnable(catcher->m_eventTapPtr, true);
        return event;
    }

    if (type != kCGEventFlagsChanged)
    {
        catcher->noteUnrelatedInput();
        return event;
    }

    CGEventFlags flags = CGEventGetFlags(event);
    auto secondTriggerKey = catcher->getSecondShortcutKey();
    bool secondKeyPressedDown =
        (secondTriggerKey == CS::SecondShortcutKeyEnum::GlobalFN && flags & kCGEventFlagMaskSecondaryFn) ||
        (secondTriggerKey == CS::SecondShortcutKeyEnum::Control && flags & kCGEventFlagMaskControl) ||
        (secondTriggerKey == CS::SecondShortcutKeyEnum::Option && flags & kCGEventFlagMaskAlternate) ||
        (secondTriggerKey == CS::SecondShortcutKeyEnum::Command && flags & kCGEventFlagMaskCommand) ||
        secondTriggerKey == CS::SecondShortcutKeyEnum::Nothing;
    catcher->handleModifierKeysStatusChange((flags & kCGEventFlagMaskShift) != 0,
                                            secondKeyPressedDown);

    return event;
}

bool KeyPressCatcher::init()
{
    if (m_eventTapPtr != nullptr)
    {
        return true;
    }

    CGEventMask eventMask = CGEventMaskBit(kCGEventFlagsChanged) |
                            CGEventMaskBit(kCGEventKeyDown) |
                            CGEventMaskBit(kCGEventLeftMouseDown) |
                            CGEventMaskBit(kCGEventRightMouseDown) |
                            CGEventMaskBit(kCGEventOtherMouseDown) |
                            CGEventMaskBit(kCGEventScrollWheel);

    m_eventTapPtr = CGEventTapCreate(kCGSessionEventTap,
                                     kCGHeadInsertEventTap,
                                     kCGEventTapOptionListenOnly,
                                     eventMask,
                                     EventTapCallback,
                                     this);
    if (m_eventTapPtr == nullptr)
    {
        return false;
    }

    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTapPtr, 0);
    if (source == nullptr)
    {
        CFRelease(m_eventTapPtr);
        m_eventTapPtr = nullptr;
        return false;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
    CFRelease(source);
    CGEventTapEnable(m_eventTapPtr, true);
    return true;
}

void KeyPressCatcher::stopEventTap()
{
    if (m_eventTapPtr == nullptr)
    {
        return;
    }

    CGEventTapEnable(m_eventTapPtr, false);
    CFMachPortInvalidate(m_eventTapPtr);
    CFRelease(m_eventTapPtr);
    m_eventTapPtr = nullptr;
}
