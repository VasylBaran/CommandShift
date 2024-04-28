#include "keypresscatcher.h"

#include "constants.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QActionGroup>
#include <QSettings>

namespace
{
    constexpr static auto s_hide_tray_icon_setting_name = "tray_icon/hide";
    constexpr static auto s_change_language_on_release_setting_name = "language/trigger_on_key_release";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMenu menu;
    auto settings = new QSettings(&a);

    auto systemTrayIcon = new QSystemTrayIcon(QIcon(":/icons/trayicon.png"), &menu);
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

    auto hideTrayIconGroup = new QActionGroup(&menu);
    hideTrayIconGroup->setExclusive(true);

    auto hideTrayIconActionPermanently = new QAction("Permanently", hideTrayIconGroup);
    hideTrayIconActionPermanently->setCheckable(true);
    auto hideTrayIconActionUtilRestart = new QAction("Until restart", hideTrayIconGroup);
    hideTrayIconActionUtilRestart->setCheckable(true);

    // Adding actions to (hide tray icon) drop-down menu
    auto hideIconFromTrayMenuDropDownActionMenu = menu.addMenu("Hide icon from tray menu...");
    hideIconFromTrayMenuDropDownActionMenu->addAction(hideTrayIconActionPermanently);
    hideIconFromTrayMenuDropDownActionMenu->addAction(hideTrayIconActionUtilRestart);

    QObject::connect(hideTrayIconActionPermanently, &QAction::triggered, systemTrayIcon, [systemTrayIcon, settings] { systemTrayIcon->hide(); settings->setValue(s_hide_tray_icon_setting_name, true); });
    QObject::connect(hideTrayIconActionUtilRestart, &QAction::triggered, systemTrayIcon, [systemTrayIcon, settings] { systemTrayIcon->hide(); });

    auto quitAction = menu.addAction("Quit");
    QObject::connect(quitAction, &QAction::triggered, &a, [&a] { a.quit(); });

    return a.exec();
}
