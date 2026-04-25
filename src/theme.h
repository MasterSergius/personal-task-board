#pragma once
#include <QString>

/**
 * Holds all stylesheet strings for a single visual theme.
 *
 * Use Theme::dark() or Theme::light() to get a ready-made theme,
 * then pass it (by const-ref or value) to widgets at construction time.
 * Because BoardView::refresh() rebuilds all column/card widgets on every
 * model mutation, they always receive the current theme.
 */
struct Theme {
    bool isDark = true;

    // Sidebar panel
    QString sidebarBg;
    QString sidebarTitleStyle;
    QString projectListStyle;

    // Board background
    QString boardBg;

    // Column
    QString columnStyle;
    QString columnHeaderStyle;
    QString addTaskBtnStyle;

    // Card frame + label
    QString cardStyle;
    QString cardLabelStyle;

    // Buttons & menus
    QString addColumnBtnStyle;
    QString menuStyle;
    QString toggleBtnStyle;

    static Theme dark()
    {
        Theme t;
        t.isDark = true;

        t.sidebarBg          = "background: #1e1e1e;";
        t.sidebarTitleStyle  = "color: #888; font-size: 11px; font-weight: bold; padding: 4px 8px;";
        t.projectListStyle   = R"(
            QListWidget { background: transparent; border: none; color: #ddd; }
            QListWidget::item { padding: 6px 12px; }
            QListWidget::item:selected { background: #3a3a3a; color: #fff; border-radius: 4px; }
            QListWidget::item:hover:!selected { background: #2a2a2a; }
        )";

        t.boardBg = "background: #141414;";

        t.columnStyle      = "QWidget#column { background: #2d2d2d; border-radius: 8px; }";
        t.columnHeaderStyle =
            "font-weight: bold; font-size: 14px; background: transparent;"
            "border: none; padding: 2px; color: #ddd;";
        t.addTaskBtnStyle  =
            "QPushButton { text-align: left; background: transparent; border: none;"
            "  color: #888; padding: 2px; }"
            "QPushButton:hover { color: #ccc; }";

        t.cardStyle =
            "QFrame { border: 1px solid #555; border-radius: 6px; background: #3a3a3a; }"
            "QFrame:hover { background: #444; border-color: #777; }";
        t.cardLabelStyle = "border: none; background: transparent; color: #ddd;";

        t.addColumnBtnStyle =
            "QPushButton { color: #888; border: 2px dashed #555; border-radius: 8px;"
            "  padding: 10px; background: transparent; }"
            "QPushButton:hover { color: #bbb; border-color: #777; }";
        t.menuStyle =
            "QMenu { background: #2d2d2d; color: #ddd; border: 1px solid #555; }"
            "QMenu::item:selected { background: #0078d4; color: #ffffff; }"
            "QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }";
        t.toggleBtnStyle =
            "QPushButton { background: transparent; border: 1px solid #555; border-radius: 4px;"
            "  color: #aaa; font-size: 11px; padding: 2px 6px; }"
            "QPushButton:hover { background: #3a3a3a; color: #ddd; }";

        return t;
    }

    static Theme light()
    {
        Theme t;
        t.isDark = false;

        t.sidebarBg         = "background: #f0f0f0;";
        t.sidebarTitleStyle = "color: #666; font-size: 11px; font-weight: bold; padding: 4px 8px;";
        t.projectListStyle  = R"(
            QListWidget { background: transparent; border: none; color: #333; }
            QListWidget::item { padding: 6px 12px; }
            QListWidget::item:selected { background: #0078d4; color: #fff; border-radius: 4px; }
            QListWidget::item:hover:!selected { background: #e0e0e0; }
        )";

        t.boardBg = "background: #d4d4d4;";

        t.columnStyle      = "QWidget#column { background: #e8e8e8; border-radius: 8px; }";
        t.columnHeaderStyle =
            "font-weight: bold; font-size: 14px; background: transparent;"
            "border: none; padding: 2px; color: #111;";
        t.addTaskBtnStyle  =
            "QPushButton { text-align: left; background: transparent; border: none;"
            "  color: #555; padding: 2px; }"
            "QPushButton:hover { color: #000; }";

        t.cardStyle =
            "QFrame { border: 1px solid #b0b0b0; border-radius: 6px; background: #fafafa; }"
            "QFrame:hover { background: #f0f0f0; border-color: #888; }";
        t.cardLabelStyle = "border: none; background: transparent; color: #111;";

        t.addColumnBtnStyle =
            "QPushButton { color: #666; border: 2px dashed #aaa; border-radius: 8px;"
            "  padding: 10px; background: transparent; }"
            "QPushButton:hover { color: #333; border-color: #777; }";
        t.menuStyle =
            "QMenu { background: #ffffff; color: #111111; border: 1px solid #cccccc; }"
            "QMenu::item:selected { background: #0078d4; color: #ffffff; }"
            "QMenu::separator { height: 1px; background: #cccccc; margin: 4px 8px; }";
        t.toggleBtnStyle =
            "QPushButton { background: transparent; border: 1px solid #ccc; border-radius: 4px;"
            "  color: #555; font-size: 11px; padding: 2px 6px; }"
            "QPushButton:hover { background: #e0e0e0; color: #222; }";

        return t;
    }
};
