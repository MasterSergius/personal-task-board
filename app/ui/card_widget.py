from PySide6.QtWidgets import QLabel
from PySide6.QtCore import Qt, QMimeData
from PySide6.QtGui import QDrag


class CardWidget(QLabel):
    def __init__(self, title: str):
        super().__init__(title)
        self.title = title

        self.setWordWrap(True)
        self.setStyleSheet("""
            border: 1px solid #888;
            border-radius: 6px;
            padding: 8px;
            background-color: #f5f5f5;
        """)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            drag = QDrag(self)

            # 📦 Data
            mime = QMimeData()
            mime.setText(self.title)
            drag.setMimeData(mime)

            # 🖼️ Drag preview
            pixmap = self.grab()
            drag.setPixmap(pixmap)
            drag.setHotSpot(event.pos())

            # 👻 Hide original widget during drag
            self.hide()

            # 🚀 Execute drag
            result = drag.exec(Qt.MoveAction)

            if result == Qt.MoveAction:
                # drop successful → delete original
                self.deleteLater()
            else:
                # drag cancelled → restore
                self.show()
