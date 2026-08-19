#include "keypresscatcher.h"

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

namespace
{
    // An input source's unique identifier, e.g. "com.apple.keylayout.ABC"
    QString inputSourceID(TISInputSourceRef source)
    {
        if (source == nullptr)
        {
            return QString();
        }

        auto identifier = (CFStringRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
        return (identifier != nullptr) ? QString::fromCFString(identifier) : QString();
    }

    QString currentInputSourceID()
    {
        TISInputSourceRef current = TISCopyCurrentKeyboardInputSource();
        auto identifier = inputSourceID(current);

        if (current != nullptr)
        {
            CFRelease(current);
        }

        return identifier;
    }

    // Keyboard layouts (ABC, RussianWin, ...) and input modes (Chinese, Japanese, ...) are the
    // only things a user actually switches between. Character palettes, Press-and-Hold and other
    // helpers also live in the "keyboard" category, so filtering on category alone is not enough.
    bool isSwitchableKeyboardSource(TISInputSourceRef source)
    {
        auto type = (CFStringRef)TISGetInputSourceProperty(source, kTISPropertyInputSourceType);
        if (type == nullptr)
        {
            return false;
        }

        return CFEqual(type, kTISTypeKeyboardLayout) || CFEqual(type, kTISTypeKeyboardInputMode);
    }

    // Enabled, selectable keyboard sources in the order macOS reports them. Caller owns the result.
    CFArrayRef copySwitchableInputSources()
    {
        const void* keys[] = {
            kTISPropertyInputSourceCategory,
            kTISPropertyInputSourceIsEnabled,
            kTISPropertyInputSourceIsSelectCapable
        };
        const void* values[] = {
            kTISCategoryKeyboardInputSource,
            kCFBooleanTrue,
            kCFBooleanTrue
        };

        CFDictionaryRef filter = CFDictionaryCreate(kCFAllocatorDefault,
                                                    keys,
                                                    values,
                                                    3,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);
        if (filter == nullptr)
        {
            return nullptr;
        }

        CFArrayRef candidates = TISCreateInputSourceList(filter, false);
        CFRelease(filter);

        if (candidates == nullptr)
        {
            return nullptr;
        }

        CFMutableArrayRef sources = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        CFIndex candidateCount = CFArrayGetCount(candidates);
        for (CFIndex i = 0; i < candidateCount; ++i)
        {
            auto candidate = (TISInputSourceRef)CFArrayGetValueAtIndex(candidates, i);
            if (isSwitchableKeyboardSource(candidate))
            {
                CFArrayAppendValue(sources, candidate);
            }
        }
        CFRelease(candidates);

        return sources;
    }

    bool selectInputSourceWithID(const QString& wantedID)
    {
        CFArrayRef sources = copySwitchableInputSources();
        if (sources == nullptr)
        {
            return false;
        }

        bool selected = false;
        CFIndex count = CFArrayGetCount(sources);
        for (CFIndex i = 0; i < count && !selected; ++i)
        {
            auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, i);
            if (inputSourceID(source) == wantedID)
            {
                selected = (TISSelectInputSource(source) == noErr);
            }
        }

        CFRelease(sources);
        return selected;
    }

    // Used when there is no remembered previous source yet, or it has since been disabled
    bool selectNextInputSourceAfter(const QString& currentID)
    {
        CFArrayRef sources = copySwitchableInputSources();
        if (sources == nullptr)
        {
            return false;
        }

        CFIndex count = CFArrayGetCount(sources);
        if (count < 2)
        {
            CFRelease(sources);
            return false;
        }

        // Match on the identifier string: two TISInputSourceRef handles to the same
        // source are not guaranteed to compare equal, so CFEqual is not reliable here.
        CFIndex currentIndex = -1;
        for (CFIndex i = 0; i < count; ++i)
        {
            if (inputSourceID((TISInputSourceRef)CFArrayGetValueAtIndex(sources, i)) == currentID)
            {
                currentIndex = i;
                break;
            }
        }

        CFIndex nextIndex = (currentIndex >= 0) ? ((currentIndex + 1) % count) : 0;
        auto next = (TISInputSourceRef)CFArrayGetValueAtIndex(sources, nextIndex);
        bool selected = (next != nullptr) && (TISSelectInputSource(next) == noErr);

        CFRelease(sources);
        return selected;
    }
}

KeyPressCatcher::KeyPressCatcher(std::function<void (const QString& title, const QString& message)> showMessageCallback)
: m_showMessageCallback{showMessageCallback}
{    
    auto secondShortcutKeyQVariant = m_settings.value(CS::secondShortcutKeySettingKeyword);
    if (!secondShortcutKeyQVariant.isNull())
    {
        auto keyEnumValue = secondShortcutKeyQVariant.toInt();
        // Value should be within expected range
        if (CS::SecondShortcutKeyEnum::_FirstElem < keyEnumValue && keyEnumValue < CS::SecondShortcutKeyEnum::_LastElem)
        {
            m_secondShortcutKey = static_cast<CS::SecondShortcutKeyEnum>(keyEnumValue);
        }
    }

    startObservingInputSource();

    m_successfully_started = init();
    m_accessibility_granted = AXIsProcessTrusted();
    if (!m_successfully_started || !m_accessibility_granted)
    {
        m_showMessageCallback(CS::setupAccessibilityTitle,
                              CS::setupAccessibilityMessage);
    }
    else
    {
        notifyAboutSuccessfulStart();
    }

    loop();
}

KeyPressCatcher::~KeyPressCatcher()
{
    // Saving user preference
    m_settings.setValue(CS::secondShortcutKeySettingKeyword, m_secondShortcutKey);

    stopObservingInputSource();

    if (m_eventTapPtr != nullptr)
    {
        CGEventTapEnable(m_eventTapPtr, false);
        CFRelease(m_eventTapPtr);
    }
}

void KeyPressCatcher::setSecondShortcutKey(CS::SecondShortcutKeyEnum keyValue)
{
    qDebug() << "set secondary key to" << keyValue;
    m_secondShortcutKey = keyValue;
}

CS::SecondShortcutKeyEnum KeyPressCatcher::getSecondShortcutKey() const
{
    return m_secondShortcutKey;
}

void KeyPressCatcher::notifyUserAboutLostPrivileges()
{
    m_showMessageCallback(CS::privilegesLostTitle,
                          CS::privilegesLostMessage);
}

void KeyPressCatcher::notifyAboutSuccessfulStart()
{
    m_showMessageCallback(CS::allGoodTitle,
                          CS::allGoodMessage);
}

void KeyPressCatcher::setChangeLanguageOnRelease(bool change_language_on_release)
{
    m_change_language_on_release = change_language_on_release;
}

bool KeyPressCatcher::changeLanguageOnRelease() const
{
    return m_change_language_on_release;
}

void KeyPressCatcher::loop()
{
    if (!AXIsProcessTrusted())
    {
        if (m_accessibility_granted)
        {
            notifyUserAboutLostPrivileges();
            m_accessibility_granted = false;
        }
    }
    else
    {
        if (!m_accessibility_granted)
        {
            notifyAboutSuccessfulStart();
            m_accessibility_granted = true;
        }
    }

    if (!m_successfully_started)
    {
        m_successfully_started = init();
    }

    QTimer::singleShot(1000, [this] { loop(); });
}

void KeyPressCatcher::startObservingInputSource()
{
    if (m_observing_input_source)
    {
        return;
    }

    m_currentSourceID = currentInputSourceID();

    // Listening for the system's own notification means switches made from the menu bar
    // or by another app are remembered too, not just the ones we make ourselves.
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    [] (CFNotificationCenterRef, void* observer, CFNotificationName, const void*, CFDictionaryRef)
                                    {
                                        static_cast<KeyPressCatcher *>(observer)->onInputSourceChanged();
                                    },
                                    kTISNotifySelectedKeyboardInputSourceChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    m_observing_input_source = true;
}

void KeyPressCatcher::stopObservingInputSource()
{
    if (!m_observing_input_source)
    {
        return;
    }

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifySelectedKeyboardInputSourceChanged,
                                       nullptr);

    m_observing_input_source = false;
}

void KeyPressCatcher::onInputSourceChanged()
{
    auto nowSelected = currentInputSourceID();
    if (nowSelected.isEmpty() || nowSelected == m_currentSourceID)
    {
        return;
    }

    m_previousSourceID = m_currentSourceID;
    m_currentSourceID = nowSelected;
}

void KeyPressCatcher::requestInputSourceSwitch()
{
    // Text Input Services must not be called from the event-tap callback: the tap runs on a
    // latency budget and macOS disables it if we take too long. Hand the work to the event loop.
    if (m_switch_queued)
    {
        return;
    }

    m_switch_queued = true;
    QTimer::singleShot(0, [this]
    {
        m_switch_queued = false;
        toggleInputSource();
    });
}

void KeyPressCatcher::toggleInputSource()
{
    auto current = currentInputSourceID();

    // The notification can be missed (it does not fire for the source we start up on),
    // so reconcile what we think is current before deciding where to go.
    if (!current.isEmpty() && current != m_currentSourceID)
    {
        m_previousSourceID = m_currentSourceID;
        m_currentSourceID = current;
    }

    if (!m_previousSourceID.isEmpty() &&
         m_previousSourceID != current &&
         selectInputSourceWithID(m_previousSourceID))
    {
        return;
    }

    // Nothing worth going back to yet, or it was removed in System Settings
    selectNextInputSourceAfter(current);
}

void KeyPressCatcher::reenableEventTap()
{
    if (m_eventTapPtr != nullptr)
    {
        CGEventTapEnable(m_eventTapPtr, true);
    }
}

void KeyPressCatcher::handleModifierKeysStatusChange(bool shift_pressed_down, bool second_key_pressed_down)
{
    if (m_change_language_on_release)
    {
        if (shift_pressed_down && second_key_pressed_down)
        {
            m_pending = true;
        }
        else if (!shift_pressed_down && m_pending)
        {
            requestInputSourceSwitch();
            m_pending = false;
        }
    }
    else
    {
        if (shift_pressed_down && second_key_pressed_down)
        {
            requestInputSourceSwitch();
        }
    }
}

bool KeyPressCatcher::init()
{
    CGEventMask modifiersPressedMask = CGEventMaskBit(kCGEventFlagsChanged);

    m_eventTapPtr = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, modifiersPressedMask,
                        [] (CGEventTapProxy, CGEventType type, CGEventRef event, void *keyPressCatcherRawPtr)
                        {
                           auto catcher = static_cast<KeyPressCatcher *>(keyPressCatcherRawPtr);

                           // These two arrive regardless of the mask we asked for. Without handling
                           // them the app goes quietly deaf until it is restarted.
                           if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput)
                           {
                               catcher->reenableEventTap();
                               return event;
                           }

                           CGEventFlags flags = CGEventGetFlags(event);
                           auto secondTriggerKey = catcher->getSecondShortcutKey();
                           // Checking whether a second key that we expected (depending on configuration) was pressed
                           auto second_key_pressed_down = ((secondTriggerKey == CS::SecondShortcutKeyEnum::GlobalFN && flags & kCGEventFlagMaskSecondaryFn) ||
                                                          (secondTriggerKey == CS::SecondShortcutKeyEnum::Control && flags & kCGEventFlagMaskControl) ||
                                                          (secondTriggerKey == CS::SecondShortcutKeyEnum::Option && flags & kCGEventFlagMaskAlternate) ||
                                                          (secondTriggerKey == CS::SecondShortcutKeyEnum::Command && flags & kCGEventFlagMaskCommand) ||
                                                          (secondTriggerKey == CS::SecondShortcutKeyEnum::Nothing));

                           // If Shift and second key were pressed (released)
                           catcher->handleModifierKeysStatusChange((flags & kCGEventFlagMaskShift), second_key_pressed_down);
                           return event;
                        }, this);

    if (m_eventTapPtr == nullptr)
    {
       return false;
    }

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTapPtr, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CFRelease(runLoopSource);

    CGEventTapEnable(m_eventTapPtr, true);
    return true;
}
