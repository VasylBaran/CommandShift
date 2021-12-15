#include "keypresscatcher.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

namespace
{
void notifyAboutSuccessfulStart(QSystemTrayIcon& systemTrayIconRef)
{
    systemTrayIconRef.showMessage("CommandShift is now up and running",
                               "Thank you for using my app! — Vasyl Baran, developer.",
                               QSystemTrayIcon::Information,
                               30000);
}

void retryInit(KeyPressCatcher& catcherRef, QSystemTrayIcon& systemTrayIconRef)
{
    QTimer::singleShot(1000, [&catcherRef, &systemTrayIconRef]
    {
        auto successfullyStartedLocal = catcherRef.init();
        if (successfullyStartedLocal != true)
            retryInit(catcherRef, systemTrayIconRef);
        else
            notifyAboutSuccessfulStart(systemTrayIconRef);
    });
}
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QMenu menu;
    auto authorAction = menu.addAction("Visit CommandShift page");
    QObject::connect(authorAction, &QAction::triggered, [] { QDesktopServices::openUrl(QUrl("https://github.com/VasylBaran/CommandShift")); });
    auto quitAction = menu.addAction("Quit");
    QObject::connect(quitAction, &QAction::triggered, &a, [&a] { a.quit(); });

    QSystemTrayIcon systemTrayIcon(QIcon(":/icons/trayicon.png"));
    systemTrayIcon.setContextMenu(&menu);
    systemTrayIcon.setToolTip("CommandShift (1.0) - developed by Vasyl Baran");
    systemTrayIcon.show();

    KeyPressCatcher catcher;
    auto successfullyStarted = catcher.init();

    if (successfullyStarted != true)
    {
        systemTrayIcon.showMessage("One more thing...",
                                   "Please add CommandShift to \"Security & Privacy -> Privacy -> Accessibility\" in order for it to work properly",
                                   QSystemTrayIcon::Warning,
                                   30000);

        retryInit(catcher, systemTrayIcon);
    }
    else
    {
        notifyAboutSuccessfulStart(systemTrayIcon);
    }

    return a.exec();
}
