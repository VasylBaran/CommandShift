#ifndef KEYPRESSCATCHER_H
#define KEYPRESSCATCHER_H

#include "constants.h"
#include <functional>
#include <string>

#include <ApplicationServices/ApplicationServices.h>

@class NSUserDefaults;

class KeyPressCatcher
{
public:
    KeyPressCatcher(NSUserDefaults* defaults,
                    std::function<void (const std::string& title, const std::string& message)> showMessageCallback);
    ~KeyPressCatcher();

    void setSecondShortcutKey(CS::SecondShortcutKeyEnum keyValue);
    CS::SecondShortcutKeyEnum getSecondShortcutKey() const;

private:
    static void ScheduleLoop(void* context);
    static CGEventRef EventTapCallback(CGEventTapProxy proxy,
                                       CGEventType type,
                                       CGEventRef event,
                                       void* context);
    bool init();
    void loop();
    void stopEventTap();
    void requestInputMonitoringPermission();

    void notifyAboutSuccessfulStart();
    void sendSystemDefaultChangeLanguageShortcut();
    void noteUnrelatedInput();
    void handleModifierKeysStatusChange(bool shift_pressed_down, bool second_key_pressed_down);

    std::function<void (const std::string& title, const std::string& message)> m_showMessageCallback;
    __CFMachPort* m_eventTapPtr = nullptr;
    NSUserDefaults* m_defaults;
    bool m_successfully_started = false;
    bool m_input_monitoring_granted = false;
    bool m_requested_input_monitoring = false;
    bool m_notified_missing_input_monitoring = false;
    bool m_combo_armed = false;
    bool m_combo_used_with_other_input = false;
    CS::SecondShortcutKeyEnum m_secondShortcutKey = CS::SecondShortcutKeyEnum::Command;
};

#endif // KEYPRESSCATCHER_H
