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

private:
    bool init();
    void retryInit();

    void notifyAboutSuccessfulStart();
    void notifyUserAboutLostPrivileges();
    void sendSystemDefaultChangeLanguageShortcut();

    std::function<void (const QString& title, const QString& message)> m_showMessageCallback;
    __CFMachPort*                                                      m_eventTapPtr = nullptr;
    QSettings                                                          m_settings;
    CS::SecondShortcutKeyEnum                                          m_secondShortcutKey = CS::SecondShortcutKeyEnum::Command;
};

#endif // KEYPRESSCATCHER_H
