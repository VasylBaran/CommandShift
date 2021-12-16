#include "keypresscatcher.h"

#include "constants.h"

#include <QCoreApplication>
#include <QTimer>

KeyPressCatcher::KeyPressCatcher(std::function<void (const QString& title, const QString& message)> showMessageCallback)
: m_showMessageCallback{showMessageCallback}
{
auto successfullyStarted = init();
if (successfullyStarted != true)
{
    m_showMessageCallback(CS::setupAccessibilityTitle,
                          CS::setupAccessibilityMessage);

    retryInit();
}
else
{
    notifyAboutSuccessfulStart();
}
}

KeyPressCatcher::~KeyPressCatcher()
{
    if (m_eventTapPtr != nullptr)
    {
        CGEventTapEnable(m_eventTapPtr, false);
        CFRelease(m_eventTapPtr);
    }
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

void KeyPressCatcher::retryInit()
{
    QTimer::singleShot(1000, [this]
    {
        auto successfullyStartedLocal = init();
        if (successfullyStartedLocal != true)
            retryInit();
        else
            notifyAboutSuccessfulStart();
    });
}

void KeyPressCatcher::sendSystemDefaultChangeLanguageShortcut()
{
    if (m_eventTapPtr != nullptr)
    {
        CGEventTapEnable(m_eventTapPtr, false);
        CFRelease(m_eventTapPtr);
        m_eventTapPtr = nullptr;
    }

    // We lost our privileges (i.e. user removed CommandShift from Accessibility list)
    // TODO: is this overkill to do this just to handle the case when we were removed from Accessibility list?
    if (init() != true)
    {
        notifyUserAboutLostPrivileges();
        retryInit();
        return;
    }

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

bool KeyPressCatcher::init()
{
    CGEventMask modifiersPressedMask = CGEventMaskBit(kCGEventFlagsChanged);

    m_eventTapPtr = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, modifiersPressedMask,
                        [] (CGEventTapProxy, CGEventType type, CGEventRef event, void *keyPressCatcherWarPtr)
                        {            
                           CGEventFlags flags = CGEventGetFlags(event);

                           if (flags & kCGEventFlagMaskControl && flags & kCGEventFlagMaskShift)
                           {
                           static_cast<KeyPressCatcher *>(keyPressCatcherWarPtr)->sendSystemDefaultChangeLanguageShortcut();
                           }

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
