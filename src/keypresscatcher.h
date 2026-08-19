#ifndef KEYPRESSCATCHER_H
#define KEYPRESSCATCHER_H

#include "constants.h"

#include <QString>
#include <QSettings>

#include <functional>

#include <ApplicationServices/ApplicationServices.h>

class KeyPressCatcher
{
public:
    KeyPressCatcher(std::function<void (const QString& title, const QString& message)> showMessageCallback);
    ~KeyPressCatcher();

    void setSecondShortcutKey(CS::SecondShortcutKeyEnum keyValue);
    CS::SecondShortcutKeyEnum getSecondShortcutKey() const;

    void setChangeLanguageOnRelease(bool change_language_on_release);
    bool changeLanguageOnRelease() const;

private:
    bool init();

    // Notify user that we've started successfully
    void notifyAboutSuccessfulStart();
    // Notify user that we lost 'Privileges' (i.e. removed from Accessibility)
    void notifyUserAboutLostPrivileges();
    // Ask for a language change; the switch itself runs outside the event-tap callback
    void requestInputSourceSwitch();
    // Switch to the previously used input source (or the next one, if there isn't one yet)
    void toggleInputSource();
    // Keep track of the most-recently-used pair of input sources
    void startObservingInputSource();
    void stopObservingInputSource();
    void onInputSourceChanged();
    // Turn the event tap back on after macOS has disabled it
    void reenableEventTap();
    // Handle modifiers state change (pressed/released)
    void handleModifierKeysStatusChange(bool shift_pressed_down, bool second_key_pressed_down);
    // Perpetual loop checking (every 1 sec) if we still have Accessibility permissions
    void loop();

    std::function<void (const QString& title, const QString& message)> m_showMessageCallback;
    __CFMachPort*                                                      m_eventTapPtr = nullptr;
    QSettings                                                          m_settings;
    bool                                                               m_successfully_started = false;
    bool                                                               m_accessibility_granted = false;
    bool                                                               m_change_language_on_release = false;
    bool                                                               m_pending = false;
    bool                                                               m_switch_queued = false;
    bool                                                               m_observing_input_source = false;
    // Identifiers ("com.apple.keylayout.ABC" and friends) of the two most recently used sources
    QString                                                            m_currentSourceID;
    QString                                                            m_previousSourceID;
    CS::SecondShortcutKeyEnum                                          m_secondShortcutKey = CS::SecondShortcutKeyEnum::Command;
};

#endif // KEYPRESSCATCHER_H
