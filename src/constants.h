#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace CS
{
    static const auto setupAccessibilityTitle = QString("One more thing...");
    static const auto setupAccessibilityMessage = QString("Please add CommandShift to \"Security & Privacy -> Privacy -> Accessibility\" in order for it to work properly");

    static const auto allGoodTitle = QString("CommandShift is now up and running");
    static const auto allGoodMessage = QString("Thank you for using my app! — Vasyl Baran, developer.");

    static const auto privilegesLostTitle = QString("CommandShift will no longer work... =(");
    static const auto privilegesLostMessage = QString("Unfortunately it seems that CommandShift has been removed from \"Security & Privacy -> Privacy -> Accessibility\" list");

    static const auto secondShortcutKeySettingKeyword = QString("SecondShortcutKey");

    enum SecondShortcutKeyEnum
    {
    _FirstElem = 0,
    GlobalFN,
    Control,
    Option,
    Command,
    _LastElem
    };
}

#endif // CONSTANTS_H
