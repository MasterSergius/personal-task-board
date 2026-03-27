from PySide6.QtWidgets import QWidget, QHBoxLayout, QScrollArea, QFrame
from .column_widget import ColumnWidget


class BoardView(QWidget):
    def __init__(self, columns=None):
        super().__init__()
        self.columns = columns or []
        self.init_ui()

    def init_ui(self):
        self.scroll = QScrollArea()
        self.scroll.setWidgetResizable(True)

        self.container = QFrame()
        self.layout = QHBoxLayout()
        self.layout.setSpacing(12)
        self.layout.setContentsMargins(10, 10, 10, 10)

        self.container.setLayout(self.layout)
        self.scroll.setWidget(self.container)

        main_layout = QHBoxLayout()
        main_layout.addWidget(self.scroll)
        self.setLayout(main_layout)

        self.build_columns(self.columns)

    def build_columns(self, columns):
        # clear existing
        while self.layout.count():
            item = self.layout.takeAt(0)
            widget = item.widget()
            if widget:
                widget.deleteLater()

        # rebuild
        for col in columns:
            column = ColumnWidget(col["title"], col.get("cards", []), parent_board=self)
            self.layout.addWidget(column)

        self.layout.addStretch()

    def set_columns(self, columns):
        self.columns = columns
        self.build_columns(columns)
