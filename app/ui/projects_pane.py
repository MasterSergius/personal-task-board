from PySide6.QtWidgets import (
    QFrame,
    QVBoxLayout,
    QWidget,
    QScrollArea,
    QPushButton,
    QMenu,
    QInputDialog,
    QMessageBox,
)

from app.state import app_state


# ------------------------
# Project Button (right-click delete)
# ------------------------
class ProjectButton(QPushButton):
    def __init__(self, name, on_delete):
        super().__init__(name)
        self.project_name = name
        self.on_delete = on_delete

    def contextMenuEvent(self, event):
        menu = QMenu(self)

        delete_action = menu.addAction("Delete Project")
        action = menu.exec(event.globalPos())

        if action == delete_action:
            if self.on_delete:
                self.on_delete(self.project_name)


# ------------------------
# Projects Pane
# ------------------------
class ProjectsPane(QFrame):
    def __init__(
        self,
        projects=None,
        on_project_selected=None,
        on_project_created=None,
        on_project_deleted=None,
    ):
        super().__init__()

        self.projects = projects or []
        self.on_project_selected = on_project_selected
        self.on_project_created = on_project_created
        self.on_project_deleted = on_project_deleted

        self.project_buttons = {}

        self.init_ui()

    def init_ui(self):
        self.setFrameStyle(QFrame.StyledPanel)

        # Scroll area
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)

        self.container = QWidget()
        self.layout = QVBoxLayout()
        self.container.setLayout(self.layout)

        self.scroll.setWidget(self.container)

        # Main layout
        main_layout = QVBoxLayout()
        main_layout.addWidget(self.scroll)
        self.setLayout(main_layout)

        # Populate initial projects
        for project in self.projects:
            self.add_project(project)

        self.layout.addStretch()

    # ------------------------
    # Project management
    # ------------------------

    def add_project(self, name):
        # TODO: improve logic of checking for duplicate name
        if name in self.project_buttons:
            return

        app_state.add_project(name)

        btn = ProjectButton(name, self.handle_delete_request)
        btn.clicked.connect(lambda _, n=name: self.select_project(n))

        self.layout.insertWidget(self.layout.count() - 1, btn)
        self.project_buttons[name] = btn

    def remove_project(self, name):
        btn = self.project_buttons.pop(name, None)
        if btn:
            self.layout.removeWidget(btn)
            btn.deleteLater()

        app_state.delete_project(name)

    def select_project(self, name):
        if self.on_project_selected:
            self.on_project_selected(name)

    # ------------------------
    # Context menu (empty area)
    # ------------------------

    def contextMenuEvent(self, event):
        menu = QMenu(self)

        add_action = menu.addAction("Add Project")
        action = menu.exec(event.globalPos())

        if action == add_action:
            self.create_project_dialog()

    # ------------------------
    # Dialogs
    # ------------------------

    def create_project_dialog(self):
        name, ok = QInputDialog.getText(self, "New Project", "Project name:")

        if ok and name:
            self.add_project(name)

            if self.on_project_created:
                self.on_project_created(name)

    def handle_delete_request(self, name):
        reply = QMessageBox.question(
            self,
            "Delete Project",
            f"Delete project '{name}'?",
            QMessageBox.Yes | QMessageBox.No,
        )

        if reply == QMessageBox.Yes:
            self.remove_project(name)

            if self.on_project_deleted:
                self.on_project_deleted(name)
