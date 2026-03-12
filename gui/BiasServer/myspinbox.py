from PyQt6.QtWidgets import QSpinBox

class MySpinBox(QSpinBox):
    def __init__(self, *args, **kwargs):
      super().__init__(*args, **kwargs)
      self.action_callback = None

    def stepBy(self, steps):
      if self.action_callback:
        if steps > 0:
          self.action_callback("FWD")  # motor FWD to higher ADC POS values
        else:
          self.action_callback("REV")  # motor REV to lower ADC POS values

      super().stepBy(steps)

