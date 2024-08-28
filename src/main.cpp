#include "keypresscatcher.h"

#include "constants.h"

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>

#include <filesystem>


namespace
{
    constexpr static auto s_hide_tray_icon_setting_name = "tray_icon/hide";
    constexpr static auto s_change_language_on_release_setting_name = "language/trigger_on_key_release";
    constexpr static auto s_white_tray_icon_preference_setting_name = "tray_icon/white_tray_icon";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMenu menu;

    auto homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    auto settingsPath = std::filesystem::path(homePath.toStdString()) / ".config" / "commandShift.ini";
    auto settings = new QSettings(QString::fromStdString(settingsPath.string()), QSettings::Format::IniFormat, &a);

    auto prefer_white_tray_icon = settings->value(s_white_tray_icon_preference_setting_name, false).toBool();
    auto systemTrayIcon = new QSystemTrayIcon(prefer_white_tray_icon ? QIcon(":/icons/trayicon_white.png") : QIcon(":/icons/trayicon.png"), &menu);

    systemTrayIcon->setContextMenu(&menu);
    systemTrayIcon->setToolTip("CommandShift (1.0) - developed by Vasyl Baran");

    auto hide_tray_icon = settings->value(s_hide_tray_icon_setting_name, false).toBool();
    if (!hide_tray_icon)
    {
        systemTrayIcon->show();
    }

    auto showMessageCallback = [systemTrayIcon](const QString& title, const QString& message)
    {
        systemTrayIcon->showMessage(title,
                                    message,
                                    QIcon(),
                                    30000);
    };

    KeyPressCatcher catcher(showMessageCallback);
    auto authorAction = menu.addAction("Visit CommandShift page");
    QObject::connect(authorAction, &QAction::triggered, [] { QDesktopServices::openUrl(QUrl("https://github.com/VasylBaran/CommandShift")); });

    /* Group of shortcuts (mutually exclusive) that should be used to change language
       i.e. we allow user to chose between different shortcuts */
    auto secondShortcutKeyGroup = new QActionGroup(&menu);
    secondShortcutKeyGroup->setExclusive(true);

    // 'Shift + FN' action
    auto fnAction = new QAction("Shift + FN", secondShortcutKeyGroup);
    fnAction->setCheckable(true);
    // 'Shift + Control' action
    auto controlAction = new QAction("Shift + Control", secondShortcutKeyGroup);
    controlAction->setCheckable(true);
    // 'Shift + Option' action
    auto optionAction = new QAction("Shift + Option", secondShortcutKeyGroup);
    optionAction->setCheckable(true);
    // 'Shift + Command' action
    auto commandAction = new QAction("Shift + Command", secondShortcutKeyGroup);
    commandAction->setCheckable(true);
    // 'Shift' action
    auto shiftAction = new QAction("Shift", secondShortcutKeyGroup);
    shiftAction->setCheckable(true);

    auto secondShortcutKey = catcher.getSecondShortcutKey();
    // Toggling currently selected shortcut
    if (secondShortcutKey == CS::SecondShortcutKeyEnum::GlobalFN)
        fnAction->toggle();
    else if (secondShortcutKey == CS::SecondShortcutKeyEnum::Control)
        controlAction->toggle();
    else if (secondShortcutKey == CS::SecondShortcutKeyEnum::Option)
        optionAction->toggle();
    else if (secondShortcutKey == CS::SecondShortcutKeyEnum::Command)
        commandAction->toggle();
    else if (secondShortcutKey == CS::SecondShortcutKeyEnum::Nothing)
        shiftAction->toggle();

    // Connections
    QObject::connect(fnAction, &QAction::triggered, [&catcher] (bool checked) { if (checked) { catcher.setSecondShortcutKey(CS::GlobalFN); } });
    QObject::connect(controlAction, &QAction::triggered, [&catcher] (bool checked) { if (checked) { catcher.setSecondShortcutKey(CS::Control); } });
    QObject::connect(optionAction, &QAction::triggered, [&catcher] (bool checked) { if (checked) { catcher.setSecondShortcutKey(CS::Option); } });
    QObject::connect(commandAction, &QAction::triggered, [&catcher] (bool checked) { if (checked) {  catcher.setSecondShortcutKey(CS::Command); } });
    QObject::connect(shiftAction, &QAction::triggered, [&catcher] (bool checked) { if (checked) {  catcher.setSecondShortcutKey(CS::Nothing); } });

    // Adding actions to the group to make them mutually exclusive (group already owns them)
    secondShortcutKeyGroup->addAction(fnAction);
    secondShortcutKeyGroup->addAction(controlAction);
    secondShortcutKeyGroup->addAction(optionAction);
    secondShortcutKeyGroup->addAction(commandAction);
    secondShortcutKeyGroup->addAction(shiftAction);

    // Adding actions to drop-down menu
    auto secondShortcutKeyDropDownActionMenu = menu.addMenu("Change language with...");
    secondShortcutKeyDropDownActionMenu->addAction(fnAction);
    secondShortcutKeyDropDownActionMenu->addAction(controlAction);
    secondShortcutKeyDropDownActionMenu->addAction(optionAction);
    secondShortcutKeyDropDownActionMenu->addAction(commandAction);
    secondShortcutKeyDropDownActionMenu->addAction(shiftAction);

    auto change_language_on_release = settings->value(s_change_language_on_release_setting_name, false).toBool();
    catcher.setChangeLanguageOnRelease(change_language_on_release);

    auto changeLanguageOnKeyReleasedAction = menu.addAction("Change language after Shift release");
    changeLanguageOnKeyReleasedAction->setCheckable(true);
    changeLanguageOnKeyReleasedAction->setChecked(change_language_on_release);
    QObject::connect(changeLanguageOnKeyReleasedAction, &QAction::triggered, changeLanguageOnKeyReleasedAction, [&catcher, settings] { catcher.setChangeLanguageOnRelease(!catcher.changeLanguageOnRelease()); settings->setValue(s_change_language_on_release_setting_name, catcher.changeLanguageOnRelease()); });

    auto trayIconGroup = new QActionGroup(&menu);
    trayIconGroup->setExclusive(false);

    auto hideTrayIconActionPermanently = new QAction("Hide icon permanently", trayIconGroup);
    hideTrayIconActionPermanently->setCheckable(true);
    auto hideTrayIconActionUtilRestart = new QAction("Hide icon until restart", trayIconGroup);
    hideTrayIconActionUtilRestart->setCheckable(true);
    auto changeIconsColor = new QAction("Make it white", trayIconGroup);
    changeIconsColor->setCheckable(true);
    changeIconsColor->setChecked(prefer_white_tray_icon);

    // Adding actions to (hide tray icon) drop-down menu
    auto trayMenuIconDropDownActionMenu = menu.addMenu("Tray menu icon...");
    trayMenuIconDropDownActionMenu->addAction(hideTrayIconActionPermanently);
    trayMenuIconDropDownActionMenu->addAction(hideTrayIconActionUtilRestart);
    trayMenuIconDropDownActionMenu->addAction(changeIconsColor);

    QObject::connect(hideTrayIconActionPermanently, &QAction::triggered, systemTrayIcon, [systemTrayIcon, settings] { systemTrayIcon->hide(); settings->setValue(s_hide_tray_icon_setting_name, true); });
    QObject::connect(hideTrayIconActionUtilRestart, &QAction::triggered, systemTrayIcon, [systemTrayIcon] { systemTrayIcon->hide(); });
    QObject::connect(changeIconsColor, &QAction::triggered, systemTrayIcon, [systemTrayIcon, settings] (bool checked) { systemTrayIcon->setIcon(checked ? QIcon(":/icons/trayicon_white.png") : QIcon(":/icons/trayicon.png")); settings->setValue(s_white_tray_icon_preference_setting_name, checked); });

    auto quitAction = menu.addAction("Quit");
    QObject::connect(quitAction, &QAction::triggered, &a, [&a] { a.quit(); });

    return a.exec();
}
