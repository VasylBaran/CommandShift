#include "keypresscatcher.h"

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <ApplicationServices/ApplicationServices.h>

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

void KeyPressCatcher::sendSystemDefaultChangeLanguageShortcut()
{
    // Creating a 'Shift + Alt' event
    CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    CGEventRef spaceDown = CGEventCreateKeyboardEvent(src, 0x31, true);
    CGEventRef spaceUp = CGEventCreateKeyboardEvent(src, 0x31, false);

    CGEventSetFlags(spaceDown, kCGEventFlagMaskAlternate);
    CGEventSetFlags(spaceUp, kCGEventFlagMaskAlternate);
    CGEventSetFlags(spaceDown, kCGEventFlagMaskControl);
    CGEventSetFlags(spaceUp, kCGEventFlagMaskControl);

    CGEventTapLocation loc = kCGHIDEventTap;

    CGEventPost(loc, spaceDown);
    CGEventPost(loc, spaceUp);

    CFRelease(src);
    CFRelease(spaceDown);
    CFRelease(spaceUp);
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
            sendSystemDefaultChangeLanguageShortcut();
            m_pending = false;
        }
    }
    else
    {
        if (shift_pressed_down && second_key_pressed_down)
        {
            sendSystemDefaultChangeLanguageShortcut();
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

    CFRunLoopAddSource(CFRunLoopGetCurrent(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_eventTapPtr, 0),
                       kCFRunLoopCommonModes);

     CGEventTapEnable(m_eventTapPtr, true);
     return true;
}
