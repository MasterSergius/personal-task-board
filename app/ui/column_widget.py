from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QLabel,
    QPushButton,
    QInputDialog,
)
from PySide6.QtCore import Qt, QPropertyAnimation, QRect, QEasingCurve

from .card_widget import CardWidget


class ColumnWidget(QWidget):
    def __init__(self, title: str, cards=None, parent_board=None):
        super().__init__()

        self.title = title
        self.cards = cards or []
        self.parent_board = parent_board

        self.setAcceptDrops(True)

        self.init_ui()

    def init_ui(self):
        self.layout = QVBoxLayout()

        # Header
        self.header = QLabel(self.title)
        self.header.setStyleSheet("font-weight: bold; font-size: 16px;")
        self.layout.addWidget(self.header)

        # Add card button
        self.add_btn = QPushButton("+")
        self.add_btn.clicked.connect(self.add_card_dialog)
        self.layout.addWidget(self.add_btn)

        # Existing cards
        for card_title in self.cards:
            self.layout.addWidget(CardWidget(card_title))

        self.layout.addStretch()
        self.setLayout(self.layout)

        self.setFixedWidth(220)

    # ------------------------
    # Drag & Drop
    # ------------------------

    def dragEnterEvent(self, event):
        if event.mimeData().hasText():
            event.acceptProposedAction()

    def dropEvent(self, event):
        text = event.mimeData().text()

        self.layout.insertWidget(self.layout.count() - 1, CardWidget(text))

        event.acceptProposedAction()

    # ------------------------
    # Add card
    # ------------------------

    def add_card_dialog(self):
        text, ok = QInputDialog.getText(self, "New Task", "Task name:")

        if ok and text:
            self.layout.insertWidget(self.layout.count() - 1, CardWidget(text))
