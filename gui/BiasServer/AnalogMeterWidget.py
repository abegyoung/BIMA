from PyQt6.QtWidgets import QWidget
from PyQt6.QtGui import QPainter, QColor, QPen, QFont, QPolygonF, QBrush
from PyQt6.QtCore import Qt, QPointF, QTimer
import math

scale=1

class AnalogMeterWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(80, 80)
        self.value = 0
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(100) # Update rate for animations

    def set_value(self, value):
        if self.value != value:
            self.value = value
            self.update() # Trigger a redraw

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Draw the meter background
        painter.save()
        brush = QBrush()
        brush.setColor(QColor('white'))
        brush.setStyle(Qt.BrushStyle.SolidPattern)
        painter.setBrush(brush)
        pen = QPen()
        pen.setColor(QColor('black'))
        pen.setWidth(1)
        painter.setPen(pen)

        painter.drawRect(0, 0, 80, 80)
        painter.restore()

        # Draw the meter ticks
        painter.save()
        painter.setPen(pen)
        painter.setFont(QFont("Arial", 8))
        painter.translate(40, 60)
        for i in range(-45, 60, 15):
            painter.save()
            painter.rotate(i)
            painter.drawLine(0, -45, 0, -55)
            painter.restore()

            #if i % 30 == 0:
            #    painter.save()
            #    painter.rotate(i)
            #    text = str(int((i + 45) / 2.7))
            #    painter.drawText(85, 0, text)
            #    painter.restore()
        painter.restore()

        needle = QPolygonF([
            QPointF(2.5, 0),
            QPointF(0, -40),
            QPointF(-2.5, 0),
        ])

        # Draw the needle (polygon)
        painter.save()
        painter.translate(40, 60)
        painter.rotate(self.value*45)
        painter.setBrush(QBrush(QColor('black'), Qt.BrushStyle.SolidPattern))
        painter.drawPolygon(needle)
        painter.restore()

