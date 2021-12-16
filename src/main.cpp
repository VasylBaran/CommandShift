#include "keypresscatcher.h"

#include "constants.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>

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

    auto showMessageCallback = [&systemTrayIcon](const QString& title, const QString& message)
    {
        systemTrayIcon.showMessage(title,
                                   message,
                                   QIcon(),
                                   30000);
    };

    KeyPressCatcher catcher(showMessageCallback);

    return a.exec();
}
