from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout
from PySide6.QtCore import Qt


class CardWidget(QWidget):
    def __init__(self, title: str):
        super().__init__()
        self.title = title
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        self.label = QLabel(self.title)
        self.label.setAlignment(Qt.AlignCenter)
        self.label.setStyleSheet("""
            border: 1px solid #888;
            border-radius: 6px;
            padding: 8px;
            background-color: #f5f5f5;
        """)
        layout.addWidget(self.label)
        self.setLayout(layout)
