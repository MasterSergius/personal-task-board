from PySide6.QtWidgets import (
    QMainWindow,
    QWidget,
    QHBoxLayout,
    QPushButton,
    QSizePolicy,
)
from .board_view import BoardView
from .projects_pane import ProjectsPane
from app.state import app_state


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Kanban Multi-Project")
        self.setGeometry(100, 100, 1200, 700)
        self.init_ui()

    def init_ui(self):
        container = QWidget()
        layout = QHBoxLayout()

        # Projects pane
        self.projects_pane = ProjectsPane(
            projects=app_state.get_projects(),
            on_project_selected=self.load_project,
        )
        self.projects_pane.setMaximumWidth(200)
        layout.addWidget(self.projects_pane)

        # Show/hide button
        self.toggle_button = QPushButton()
        self.toggle_button.setFixedSize(30, 30)
        self.toggle_button.clicked.connect(self.toggle_projects)
        self.toggle_button.setStyleSheet("""
            QPushButton {
                border-radius: 15px;
                background-color: #cccccc;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #bbbbbb;
            }
        """)
        layout.addWidget(self.toggle_button)

        # Board view (start with first project)
        self.board = BoardView(app_state.projects[list(app_state.projects.keys())[0]])
        layout.addWidget(self.board)

        container.setLayout(layout)
        self.setCentralWidget(container)
        self.toggle_projects()

    def load_project(self, project_name):
        # Replace board content
        self.centralWidget().layout().removeWidget(self.board)
        self.board.deleteLater()
        self.board = BoardView(app_state.projects[project_name])
        self.centralWidget().layout().addWidget(self.board)

    def toggle_projects(self):
        if self.projects_pane.isVisible():
            self.projects_pane.hide()
            self.toggle_button.setText("▶")
            self.toggle_button.setToolTip("Show projects")
        else:
            self.projects_pane.show()
            self.toggle_button.setText("◀")
            self.toggle_button.setToolTip("Hide projects")
