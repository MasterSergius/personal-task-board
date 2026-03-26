from PySide6.QtWidgets import QWidget, QVBoxLayout, QPushButton, QSizePolicy


class ProjectWidget(QWidget):
    def __init__(self, project_name, on_click=None):
        super().__init__()
        self.project_name = project_name
        self.on_click = on_click
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        btn = QPushButton(self.project_name)
        btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        if self.on_click:
            btn.clicked.connect(lambda: self.on_click(self.project_name))
        layout.addWidget(btn)
        layout.addStretch()
        self.setLayout(layout)
