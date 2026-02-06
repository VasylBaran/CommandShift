#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace CS
{
    static constexpr const char* setupInputMonitoringTitle = "Input Monitoring required";
    static constexpr const char* setupInputMonitoringMessage = "Allow CommandShift to receive keystrokes from other apps so it can detect modifier-only shortcuts and switch input sources. ";

    static constexpr const char* allGoodTitle = "CommandShift is now up and running";
    static constexpr const char* allGoodMessage = "Thank you for using my app! — Vasyl Baran, developer.";

    static constexpr const char* inputMonitoringLostTitle = "Input Monitoring was removed";
    static constexpr const char* inputMonitoringLostMessage = "CommandShift cannot detect modifier-only shortcuts until Input Monitoring is enabled again. ";

    static constexpr const char* secondShortcutKeySettingKeyword = "SecondShortcutKey";

    enum SecondShortcutKeyEnum
    {
        _FirstElem = 0,
        GlobalFN,
        Control,
        Option,
        Command,
        Nothing,
        _LastElem
    };
}

#endif // CONSTANTS_H
