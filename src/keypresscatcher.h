#ifndef KEYPRESSCATCHER_H
#define KEYPRESSCATCHER_H

#include <ApplicationServices/ApplicationServices.h>
#include <QString>
#include <functional>

class KeyPressCatcher
{
public:
    KeyPressCatcher(std::function<void (const QString& title, const QString& message)> showMessageCallback);
    ~KeyPressCatcher();

private:
    bool init();
    void retryInit();

    void notifyAboutSuccessfulStart();
    void notifyUserAboutLostPrivileges();
    void sendSystemDefaultChangeLanguageShortcut();

    std::function<void (const QString& title, const QString& message)> m_showMessageCallback;
    __CFMachPort*                                                      m_eventTapPtr = nullptr;
};

#endif // KEYPRESSCATCHER_H
