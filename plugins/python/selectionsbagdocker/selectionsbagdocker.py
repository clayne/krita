# SPDX-License-Identifier: CC0-1.0

try:
    from PyQt6.QtWidgets import QWidget

    from PyQt6 import uic
except:
    from PyQt5.QtWidgets import QWidget

    from PyQt5 import uic
from krita import DockWidget
from builtins import i18n
import os


class SelectionsBagDocker(DockWidget):

    def __init__(self):
        super().__init__()
        widget = QWidget(self)
        ui_filename = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            'selectionsbagdocker.ui')
        uic.loadUi(ui_filename, widget)
        self.setWidget(widget)
        self.setWindowTitle(i18n("Selections Bag"))

    def canvasChanged(self, canvas):
        print("Canvas", canvas)
