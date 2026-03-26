from PySide6.QtWidgets import QWidget, QVBoxLayout, QLabel
from .card_widget import CardWidget


class ColumnWidget(QWidget):
    def __init__(self, title: str, cards=None):
        super().__init__()
        self.title = title
        self.cards = cards or []
        self.init_ui()

    def init_ui(self):
        layout = QVBoxLayout()
        self.header = QLabel(self.title)
        self.header.setStyleSheet("font-weight: bold; font-size: 16px;")
        layout.addWidget(self.header)

        for card_title in self.cards:
            layout.addWidget(CardWidget(card_title))

        layout.addStretch()
        self.setLayout(layout)
        self.setStyleSheet("""
            background-color: #e0e0e0;
            border-radius: 8px;
            padding: 4px;
        """)
