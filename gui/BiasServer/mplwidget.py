from PyQt6 import QtGui, QtCore, QtWidgets

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

from matplotlib.figure import Figure

class MplCanvas(FigureCanvas):
    def __init__(self):
        self.fig = Figure()
        self.ax = self.fig.add_subplot(111)
        FigureCanvas.__init__(self, self.fig)
        FigureCanvas.setSizePolicy(self,
                                   QtWidgets.QSizePolicy.Policy.MinimumExpanding,
                                   QtWidgets.QSizePolicy.Policy.MinimumExpanding)
        FigureCanvas.updateGeometry(self)

class MplWidget(QtWidgets.QWidget):
    def __init__(self, parent = None):
        QtWidgets.QWidget.__init__(self, parent)

        self.canvas = MplCanvas()
        self.canvas.mpl_connect('button_press_event', cursorValue)
        self.toolbar = NavigationToolbar(self.canvas, self)
        self.vbl = QtWidgets.QVBoxLayout()
        self.vbl.addWidget(self.canvas)
        self.setLayout(self.vbl)
        self.vbl.addWidget(self.toolbar)

def cursorValue(event):
    QtGui.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.CrossCursor))
    print('Your x and y mouse positions are ', event.xdata, event.ydata)
