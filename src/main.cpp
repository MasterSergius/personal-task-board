#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Ensure context menus are always readable regardless of parent widget styling
    app.setStyleSheet(R"(
        QMenu {
            background: #ffffff;
            color: #111111;
            border: 1px solid #cccccc;
        }
        QMenu::item:selected {
            background: #0078d4;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background: #cccccc;
            margin: 4px 8px;
        }
    )");

    MainWindow w;
    w.show();

    return app.exec();
}
