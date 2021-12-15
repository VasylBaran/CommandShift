#include "keypresscatcher.h"

#include <ApplicationServices/ApplicationServices.h>

namespace
{
void sendSystemDefaultChangeLanguageShortcut()
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

   CFRelease(spaceDown);
   CFRelease(spaceUp);
}
}

bool KeyPressCatcher::init()
{
    CGEventMask modifiersPressedMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);

    auto eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, modifiersPressedMask,
                        [] (CGEventTapProxy, CGEventType type, CGEventRef event, void*)
                        {
                           CGEventFlags flags = CGEventGetFlags(event);

                           if (flags & kCGEventFlagMaskControl && flags & kCGEventFlagMaskShift)
                           {
                           sendSystemDefaultChangeLanguageShortcut();
                           }

                        return event;
                        }, this);

    if (eventTap == nullptr)
    {
       return false;
    }

    CFRunLoopAddSource(CFRunLoopGetCurrent(),
                       CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0),
                       kCFRunLoopCommonModes);

     CGEventTapEnable(eventTap, true);
     return true;
}
