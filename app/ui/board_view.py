from PySide6.QtWidgets import QWidget, QHBoxLayout, QScrollArea, QFrame
from .column_widget import ColumnWidget


class BoardView(QWidget):
    def __init__(self, columns=None):
        super().__init__()
        self.columns = columns or []
        self.init_ui()

    def init_ui(self):
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        container = QFrame()
        layout = QHBoxLayout()
        container.setLayout(layout)

        for col in self.columns:
            layout.addWidget(ColumnWidget(col["title"], col.get("cards", [])))

        layout.addStretch()
        scroll.setWidget(container)

        main_layout = QHBoxLayout()
        main_layout.addWidget(scroll)
        self.setLayout(main_layout)
