import sys
import time
import socket
import datetime as dt
import numpy as np
from sys import argv, exit
from io import StringIO
import matplotlib
import matplotlib.pyplot as plt
from functools import partial

from PyQt6.QtCore import QObject, QThread, pyqtSignal, pyqtSlot
from PyQt6 import QtWidgets

from AnalogMeterWidget import AnalogMeterWidget

from PyQt6 import QtCore, uic
from PyQt6.QtWidgets import (
    QApplication,
    QLabel,
    QPushButton,
    QMainWindow,
    QStatusBar,
    QToolBar,
    QWidget,
    QCheckBox,
)

form_class = uic.loadUiType("mainwindow.ui")[0]

# ------------------- Worker -------------------
class SocketWorker(QObject):
    statusReceived = pyqtSignal(str)   # Emits the multiline status (no END)
    error = pyqtSignal(str)
    finished = pyqtSignal()

    def __init__(self, host="127.0.0.1", port=9000):
        super().__init__()
        self.host = host
        self.port = port
        self._running = True
        self.sock = None

    @pyqtSlot()
    def run(self):
        import socket
        buffer = b""
        lines = []

        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            self.sock.settimeout(1.0)
            self.error.emit(f"Connected to {self.host}:{self.port}")
        except Exception as e:
            self.error.emit(f"Failed to connect: {e}")
            self.finished.emit()
            return

        try:
            while self._running:
                try:
                    chunk = self.sock.recv(1024)
                except socket.timeout:
                    continue
                if not chunk:
                    break

                buffer += chunk
                # Process all colotmplete lines
                while b"\n" in buffer:
                    line, buffer = buffer.split(b"\n", 1)
                    line = line.decode(errors="ignore").strip()
                    if not line:
                        continue

                    # Normal key/value line
                    if " " in line:
                        # Emit immediately for single line case
                        self.statusReceived.emit(line)
                    else:
                        # Fallback if weird data
                        print("Emitting raw:", line)
                        #self.statusReceived.emit(line)


        except Exception as e:
            self.error.emit(f"Socket read error: {e}")
        finally:
            try:
                self.sock.close()
            except Exception:
                pass
            self.finished.emit()

    @pyqtSlot(str)
    def send_command(self, cmd):
        if self.sock:
            try:
                self.sock.sendall(cmd.encode("utf-8"))
            except Exception as e:
                self.error.emit(f"Send failed: {e}")
        else:
            self.error.emit("No open socket")

    @pyqtSlot()
    def stop(self):
        """Request the run loop to stop. Closing the port will unblock readline()."""
        self._running = False
        try:
            if self.sock:
                self.sock.shutdown(socket.SHUT_RDWR)
                self.sock.close()
        except Exception:
            pass

# ------------------- Main GUI -------------------

# ______________ GLOBALS ________________
s=0
sport = 9000
serverip = '192.168.0.51'
numPoints = 25
start = 0
stop = 15000

def recv_end(the_socket, End):
    total_data=[];data=''
    while True:
        try:
            data=the_socket.recv(8192).decode()
        except:
            break
        if End in data:
            total_data.append(data[:data.find(End)])
            break
        total_data.append(data)
        if len(total_data)>1:
            #check if end_of_data was split
            last_pair=total_data[-2]+total_data[-1]
            if End in last_pair:
                total_data[-2]=last_pair[:last_pair.find(End)]
                total_data.pop()
                break
    return ''.join(total_data)


class Window(QMainWindow, form_class):
    def __init__(self):
        super().__init__(parent=None)
        self.setWindowTitle("QMainWindow")
        self.setCentralWidget(QLabel("I'm the Central Widget"))
        self._createMenu()
        self.setupUi(self)

        ### Setup UI
        self.AnalogMeterWidget1 = AnalogMeterWidget(parent=self.widget_1)
        self.AnalogMeterWidget2 = AnalogMeterWidget(parent=self.widget_2)

        self.data_store = {}     # dict: key -> numpy array
        self.pumped_store = {}   # dict: key -> numpy array
        self.unpumped_store = {} # dict: key -> numpy array
        self.data_Pstore = {}    # dict: key -> numpy array

        # incoming light
        self.flash_incoming()
        self.incoming.setStyleSheet("""QLabel {border-radius: 10px;
                                             background-color: transparent;
                                             border: 1px solid blue;}""")

        ### SIGNALS AND SLOTS ###
        self.pushButton.clicked.connect(self.btn_clicked)
        self.timer.toggled.connect(self.startButton_clicked)
        self.blankingBias.toggled.connect(self.blankingBias_clicked)
        self.blankingPam.toggled.connect(self.blankingPam_clicked)
        self.blankingRcvr.toggled.connect(self.blankingRcvr_clicked)


        # Vmix ADCs
        self.setVmixGain.valueChanged.connect(self.updateVmixGainOffset)
        self.setVmixOffset.valueChanged.connect(self.updateVmixGainOffset)
        self.getVgain.clicked.connect(self.getVmix_clicked)

        # Imix ADCs
        self.setImixGain.valueChanged.connect(self.updateImixGainOffset)
        self.setImixOffset.valueChanged.connect(self.updateImixGainOffset)
        self.getIgain.clicked.connect(self.getImix_clicked)

        # Vmix DAC
        self.setVmixDACOffset.valueChanged.connect(self.updateVmixDACOffset)
        self.getVDACgain.clicked.connect(self.getVmixDACOffset_clicked)

        # R values
        self.setRseries.valueChanged.connect(self.updateRseries)
        self.setRsense.valueChanged.connect(self.updateRsense)

        # EEPROM for ADCs & DACs
        self.writeEEPROM.clicked.connect(self.writeEEPROM_clicked)

        self.spinVoltage.valueChanged.connect(self.btn_setbias_clicked)

        self.spinLNA_Vd.valueChanged.connect(self.updateLNA_Vd)
        self.spinLNA_Vg.valueChanged.connect(self.updateLNA_Vg)
        self.spinIFTOTPOW.valueChanged.connect(self.updateIF)
        self.setfreq.valueChanged.connect(self.updateFREQ)
        self.setLband.valueChanged.connect(self.updateLBAND)

        self.btn_sweep.clicked.connect(self.btn_sweep_clicked)
        self.btn_Vgap.clicked.connect(self.btn_Vgap_clicked)
        self.vmodeButton.toggled.connect(self.vmodeButton_clicked)
        self.spinnumSweep.valueChanged.connect(self.update_sweepnum)
        self.sweepstart.valueChanged.connect(self.update_sweepstart)
        self.sweepstop.valueChanged.connect(self.update_sweepstop)
        self.ipAddress.returnPressed.connect(self.updateIP)
        self.TCPport.returnPressed.connect(self.updatePORT)

        # PLOT
        self.xdata_store = list(range(60))
        self.ydata_store = [int(0) for i in range(60)]
        self._plot_ref = None

        # PCA9502
        self.checkboxes = []
        for i in range(8):
          cb = getattr(self, f"dcdc{i}")
          self.checkboxes.append(cb)
          cb.toggled.connect(partial(self.btn_dcdc_clicked, i))

        # DAC
        self.spinBox1.valueChanged.connect(self.update_spinBox1)
        self.spinBox2.valueChanged.connect(self.update_spinBox2)
        self.spinBox3.valueChanged.connect(self.update_spinBox3)

        # Motors
        self.toolButton_1.clicked.connect(self.update_bumpMotor1FWD)
        self.toolButton_2.clicked.connect(self.update_bumpMotor1REV)
        self.toolButton_3.clicked.connect(self.update_bumpMotor2FWD)
        self.toolButton_4.clicked.connect(self.update_bumpMotor2REV)
        self.toolButton_5.clicked.connect(self.update_bumpMotor3FWD)
        self.toolButton_6.clicked.connect(self.update_bumpMotor3REV)

        ### THREADS ###
        # Create worker + thread
        self.thread = QThread()
        self.worker = SocketWorker()
        self.worker.moveToThread(self.thread)

        # Start worker.run when thread starts
        self.thread.started.connect(self.worker.run)

        # Connect signals
        self.worker.statusReceived.connect(self.handle_text_block)
        self.worker.error.connect(self.log_message)

        # Start thread now
        self.thread.start()

        ### TIMER ###

        self.timer = QtCore.QTimer(self)
        self.timer.setInterval(3000)
        self.timer.timeout.connect(self.askStatus)

    def startButton_clicked(self, enabled):
        self.timer.stop()
        if enabled:
            self.timer.start()

    @QtCore.pyqtSlot()
    def askStatus(self):
        cmd="status\n"
        self.worker.send_command(cmd)

          # --- Log helper ---
    def log_message(self, text):
        self.serverResponse.append(text)

    # ------------- GUI Handlers ---------------
    def flash_incoming(self):
      self.incoming.setStyleSheet("""QLabel {border-radius: 10px;
                                             background-color: green;
                                             border: 1px solid blue;}""")

      QtCore.QTimer.singleShot(100, lambda: self.incoming.setStyleSheet("""QLabel {border-radius: 10px;
                                             background-color: transparent;
                                             border: 1px solid blue;}"""))

    def handle_text_block(self, block_text: str):
        self.flash_incoming()
        try:
            status = {}
            for line in block_text.splitlines():
                line = line.strip()
                if not line:
                    continue
                parts = line.split(maxsplit=1)
                if len(parts) > 1:
                    key, value = parts
                    status[key.upper()] = value.strip()
                else:
                    # if a line doesn't split as expected, log it
                    self.serverResponse.append(f"Bad line: {line}")

            ###################  API 208 SIS Bias Control Module  ###########################

            key="VMIX_GAIN"         #0x20F
            if key in status:
                self.getVmixGain.setText(status[key])
                self.setVmixGain.blockSignals(True)
                self.setVmixGain.setValue(float(status[key]))
                self.setVmixGain.blockSignals(False)

            key="VMIX_OFFSET"       #0x20F
            if key in status:
                self.getVmixOffset.setText(status[key])
                self.setVmixOffset.blockSignals(True)
                self.setVmixOffset.setValue(float(status[key]))
                self.setVmixOffset.blockSignals(False)

            key="IMIX_GAIN"         #0x211
            if key in status:
                self.getImixGain.setText(status[key])
                self.setImixGain.blockSignals(True)
                self.setImixGain.setValue(float(status[key]))
                self.setImixGain.blockSignals(False)

            key="IMIX_OFFSET"       #0x211
            if key in status:
                self.getImixOffset.setText(status[key])
                self.setImixOffset.blockSignals(True)
                self.setImixOffset.setValue(float(status[key]))
                self.setImixOffset.blockSignals(False)

            key="VMIX_DACOFFSET"    #0x213
            if key in status:
                self.getVmixDACOffset.setText(status[key])
                self.setVmixDACOffset.blockSignals(True)
                self.setVmixDACOffset.setValue(float(status[key]))
                self.setVmixDACOffset.blockSignals(False)

            key="FETEMP"
            if key in status:
                self.FEtemp.setText((status[key]))

            key="VMON"
            if key in status:
                self.editVoltage.setText((status[key]))
                self.editVoltage_2.setText((status[key]))

            key="IMON"
            if key in status:
                self.editCurrent.setText((status[key]))
                self.editCurrent_2.setText((status[key]))

            key="VGAP"
            if key in status:
                self.editVgap.setText((status[key]))

            key="VLOOP"
            if key in status:
                mask = not bool(int(status[key]))
                self.vmodeButton.blockSignals(True)
                self.vmodeButton.setChecked(mask)
                self.vmodeButton.blockSignals(False)
                self.vmodeButton.setText("Feedback [ON]" if mask else "Feedback [OFF]")

            key="VD1"
            if key in status:
                self.LNAVd.setText((status[key]))
            key="ID1"
            if key in status:
                self.LNAId.setText((status[key]))
            key="VG1"
            if key in status:
                self.LNAVg.setText((status[key]))

            key="P"
            if key in status:
                if "END" not in status[key]:
                    try:
                        value3 = float(status[key].split()[0])
                        value4 = float(status[key].split()[1])
                    except ValueError:
                        print("error in power");
                        return

                    #Assemble numpy array
                    if key not in self.data_Pstore:
                        #print("assembling new array")
                        self.data_Pstore[key] = np.array([value3, value4])
                    else:
                        #print("appending")
                        self.data_Pstore[key] = np.vstack([self.data_Pstore[key], [value3, value4]])

                else:
                    # Clear stored array
                    del self.data_Pstore
                    self.data_Pstore = {} # dict: key -> numpy array
                


            key="IV"
            if key in status:
                if "END" not in status[key]:
                    try:
                        value1 = float(status[key].split()[0])
                        value2 = float(status[key].split()[1])
                    except ValueError:
                        #print("received END")
                        return # ignore non-numeric

                    #Assemble numpy array
                    if key not in self.data_store:
                        #print("assembling new array")
                        self.data_store[key] = np.array([value1, value2])
                    else:
                        #print("appending")
                        self.data_store[key] = np.vstack([self.data_store[key], [value1, value2]])
                else:
                    #print("END was in status")
                    #PLOT array

                    xytp = self.data_store[key]
                    self.mpl.canvas.ax.clear()
                    self.mpl.canvas.ax.set_xlim(xmin=-2, xmax=15)
                    self.mpl.canvas.ax.set_ylim(ymin=-2, ymax=150)
                    self.mpl.canvas.ax.autoscale(enable='False')
                    self.mpl.canvas.ax.get_xaxis().grid(True)
                    self.mpl.canvas.ax.get_yaxis().grid(True)
                    self.mpl.canvas.ax.plot(xytp[0:len(xytp)-1,0],xytp[0:len(xytp)-1,1], marker="None")
                    self.mpl.canvas.draw()

                    if self.saveIV.isChecked():
                        today=dt.date.today()
                        time=dt.datetime.now().time()
                        d=dt.datetime.combine(today, time)
                        fname=d.strftime("%m%d_%H%M%S.txt")
                        comment = "%s" % self.IVsweep_text.text()
                        arr = np.array(xytp[0:len(xytp)-1,0:2])
                        np.savetxt(fname, arr, fmt='%s', header=comment)

                    # Clear stored array
                    del self.data_store
                    self.data_store = {} # dict: key -> numpy array


            ###################  API 224 Pre Amplifier Module  ###########################
            key="PAMTEMP"
            if key in status:
                self.PAMtemp.setText((status[key]))

            key="IFTOTPOW"
            if key in status:
                self.IFTOTPOW.setText((status[key]))

            key="INATTEN"
            if key in status:
                self.ATTENIN.setText((status[key]))

            key="OUTATTEN"
            if key in status:
                self.ATTENOUT.setText((status[key]))

            ###################  Receiver Control Card  ###########################
            key="SERVER"
            #example
            #   0     1    2     3       4          5         6
            # SERVER  3    8     10    97650.000  97650.000  1221.250
            #  KEY   BND YIG_H Gunn_H  GunnFreq   LOFreq     L_BAND
            if key in status:
              response = status[key].split()
              ## BAND
              if response[0]=="1":
                self.select1MM.setChecked(1)
                self.select3MM.setChecked(0)
              if response[0]=="3":
                self.select1MM.setChecked(0)
                self.select3MM.setChecked(1)

              ## YIG Harmonic
              if response[1]=="8":
                self.selectN8.setChecked(1)
                self.selectN9.setChecked(0)
              if response[1]=="9":
                self.selectN8.setChecked(0)
                self.selectN9.setChecked(1)

              ## Gunn Harmonic
              if response[2]=="8":
                self.selectM8.setChecked(1)
                self.selectM9.setChecked(0)
                self.selectM10.setChecked(0)
              if response[2]=="9":
                self.selectM8.setChecked(0)
                self.selectM9.setChecked(1)
                self.selectM10.setChecked(0)
              if response[2]=="10":
                self.selectM8.setChecked(0)
                self.selectM9.setChecked(0)
                self.selectM10.setChecked(1)

              ## LO and L_band Synth values
              self.setfreq.blockSignals(True)
              self.setfreq.setValue(float(response[4]))
              self.setfreq.blockSignals(False)
              self.setLband.blockSignals(True)
              self.setLband.setValue(float(response[5]))
              self.setLband.blockSignals(False)

            key="TTL0"
            if key in status:
              mask=int(status[key], 2)
              for i in range(self.dcdc_ctrl_layout.count()):
                self.dcdc_ctrl_layout.itemAt(i).widget().blockSignals(True)
                self.dcdc_ctrl_layout.itemAt(i).widget().setChecked((mask & (1<<i))>>i)
                self.dcdc_ctrl_layout.itemAt(i).widget().blockSignals(False)
              self.dcdc1.setText("Sweep ON" if ((mask>>1)&0x1) else "Sweep OFF")

            key="TTL1"
            if key in status:
              mask=int(status[key], 2)
              for i in range(self.dcdc_input_layout.count()):
                self.dcdc_input_layout.itemAt(i).widget().blockSignals(True)
                self.dcdc_input_layout.itemAt(i).widget().setChecked((mask & (1<<i))>>i)
                self.dcdc_input_layout.itemAt(i).widget().blockSignals(False)

            for i in range(8):
                key=f"ADC{i}"
                if key in status:
                    self.adc_layout.itemAt(i).widget().setText(status[key])

                    # If it's the X-Band or MM-Band error signals, send to analog meters
                    # set_value() takes full scale +/-1.0 float
                    if (i==0):
                        self.AnalogMeterWidget1.set_value(float(status[key]))
                    if (i==2):
                        # translate adc read to -1 to +1 to display on 0-100% scale
                        # 40% = 1.27V
                        # 72% = 2.35V
                        GunnVop = float(status[key])
                        display_slope=(0.44+0.2)/(2.35-1.27)
                        display_value=max(-1.0, min(display_slope*GunnVop-1, 1.0))
                        self.AnalogMeterWidget2.set_value(display_value)
                    if (i==4):
                        m=-105.651847
                        b= 18203.879863
                        pos=f"{(float(status[key])-b)/m:.1f}"
                        self.uPosTune.setText(str(pos))
                    if (i==5):
                        m=-105.798227
                        b= 24846.692483
                        pos=f"{(float(status[key])-b)/m:.1f}"
                        self.uPosBck.setText(str(pos))

            for i in range(4):
                key=f"DAC{i}"
                if (key in status):
                    self.dac_layout.itemAt(i).widget().blockSignals(True)
                    self.dac_layout.itemAt(i).widget().setValue(int(status[key]))
                    self.dac_layout.itemAt(i).widget().blockSignals(False)


            key="IFTOTPOW"
            if (key in status):
                self.ydata_store = self.ydata_store[1:] + [np.genfromtxt(StringIO(status[key]))]
                if self._plot_ref is None:
                    self.mpl2.canvas.ax.get_xaxis().grid(True)
                    self.mpl2.canvas.ax.get_yaxis().grid(True)
                    self.mpl2.canvas.ax.set_xlim(0,60)
                    self.mpl2.canvas.ax.set_ylim(0,.2)
                    self.mpl2.canvas.ax.set_autoscale_on(False)
                    self.mpl2.canvas.fig.tight_layout()
                    self.mpl2.canvas.draw()
                    self.mpl2.canvas.flush_events()
                    self.background = self.mpl2.canvas.copy_from_bbox(self.mpl2.canvas.ax.bbox)
                    self.plot_refs  = self.mpl2.canvas.ax.plot(self.xdata_store, self.ydata_store, 'blue')
                    self._plot_ref  = self.plot_refs[0]
                else:
                    self._plot_ref.set_ydata(self.ydata_store)
                    self.mpl2.canvas.restore_region(self.background)
                    self.mpl2.canvas.ax.draw_artist(self._plot_ref)
                    self.mpl2.canvas.blit(self.mpl2.canvas.ax.bbox)
                    #print("bbox:", self.mpl2.canvas.ax.bbox)


        except Exception as e:
            self.serverResponse.append(f"Error parsing status: {e}")

    def _createMenu(self):
        menu = self.menuBar().addMenu("&Menu")
        menu.addAction("&Exit", self.close)

######### Outgoing ACTIONS ############
    def updateIP(self):
        serverip = self.ipAddress.text()
        self.serverResponse.setText("ip address updated")

    def updatePORT(self):
        sport = self.TCPport.text()
        self.serverResponse.setText("TCP port updated")

    ############# COMMANDS TO RECEIVER CTRL #############
    def btn_dcdc_clicked(self, index: int, checked: bool):
        mask=0
        for i, cb in enumerate(self.checkboxes):
            if cb.isChecked():
                mask |= (1<<i)
        cmd="dcdc --mask %d\n" % mask
        self.worker.send_command(cmd)

    def update_spinBox1(self):
        cmd="setdac --chan 0 --dac %d\n" % self.spinBox1.value()
        self.worker.send_command(cmd)

    def update_spinBox2(self):
        cmd="setdac --chan 1 --dac %d\n" % self.spinBox2.value()
        self.worker.send_command(cmd)

    def update_spinBox3(self):
        cmd="setdac --chan 2 --dac %d\n" % self.spinBox3.value()
        self.worker.send_command(cmd)

    def update_bumpMotor1FWD(self):
        cmd="bump --chan 0 --direction %d\n" % 0
        self.worker.send_command(cmd)

    def update_bumpMotor1REV(self):
        cmd="bump --chan 0 --direction %d\n" % 1
        self.worker.send_command(cmd)

    def update_bumpMotor2FWD(self):
        cmd="bump --chan 1 --direction %d\n" % 0
        self.worker.send_command(cmd)

    def update_bumpMotor2REV(self):
        cmd="bump --chan 1 --direction %d\n" % 1
        self.worker.send_command(cmd)

    def update_bumpMotor3FWD(self):
        cmd="bump --chan 2 --direction %d\n" % 0
        self.worker.send_command(cmd)

    def update_bumpMotor3REV(self):
        cmd="bump --chan 2 --direction %d\n" % 1
        self.worker.send_command(cmd)

    ############# COMMANDS TO BIAS FRONT END #############
    def vmodeButton_clicked(self, enabled):
        cmd="setfeedback --mask 1\n"
        if enabled:
            cmd="setfeedback --mask 0\n"
        self.worker.send_command(cmd)

    def update_sweepnum(self):
        global numPoints
        numPoints = self.spinnumSweep.value()

    def update_sweepstart(self):
        global start
        start = self.sweepstart.value()

    def update_sweepstop(self):
        global stop
        stop = self.sweepstop.value()

    def btn_setbias_clicked(self):
        bias = self.spinVoltage.value()
        cmd="setbias --dac %d\n" % bias
        self.worker.send_command(cmd)

    def updateLNA_Vd(self):
        value = self.spinLNA_Vd.value()
        cmd="setLNAdrain --dac %d\n" % int(1000*value)
        self.worker.send_command(cmd)

    def updateLNA_Vg(self):
        value = self.spinLNA_Vg.value()
        cmd="setLNAgate --dac %d\n" % int(1000*value)
        self.worker.send_command(cmd)

    def updateIF(self):
        value = self.spinIFTOTPOW.value()
        cmd="setIF --val %.4f\n" % value
        self.worker.send_command(cmd)

    def updateFREQ(self):
        value = self.setfreq.value()
        cmd="setfreq --freq %.6f\n" % value
        self.worker.send_command(cmd)

    def updateLBAND(self):
        if self.selectN8.isChecked(): harmonicN = 8
        if self.selectN9.isChecked(): harmonicN = 9

        if self.selectM8.isChecked(): harmonicM = 8
        if self.selectM9.isChecked(): harmonicM = 9
        if self.selectM10.isChecked(): harmonicM = 10 

        value = self.setLband.value()
        if self.select1MM.isChecked():
            band = 1
            self.setfreq.blockSignals(True)
            self.setfreq.setRange(211.110, 272.040)
            self.setfreq.blockSignals(False)
        elif self.select3MM.isChecked():
            band = 3
            self.setfreq.blockSignals(True)
            self.setfreq.setRange(79.160, 113.350)
            self.setfreq.blockSignals(False)

            # Need logic to choose 3MM YIG harmonic here
        cmd="setLband --band %d --YIGHarmonicN %d --GunnHarmonicM %d --freq %.6f\n" % (band, harmonicN, harmonicM, value)
        self.worker.send_command(cmd)

    def btn_sweep_clicked(self):
        global start
        global stop
        global numPoints
        step = int((stop-start)/numPoints)
        if self.set_ivp.isChecked():
            power=1
        else:
            power=0
        cmd="sweep --start %d --stop %d --step %d --power %d\n" % (start, stop, step, power)
        self.worker.send_command(cmd)
        time.sleep(3)

    def btn_Vgap_clicked(self):
        msgid = f"0x{((0<<28)|(1<<27)|(0x087<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x0000000000000000\n" % msgid
        self.worker.send_command(cmd)





    def vmodeButton_clicked(self, enabled):
        cmd="setfeedback --mask 1\n"
        if enabled:
            cmd="setfeedback --mask 0\n"
        self.worker.send_command(cmd)

    def blankingBias_clicked(self, enabled):
        node = self.apiBias.value()
        state=0
        if enabled:
            state=1
        msgid = f"0x{((0<<28)|(1<<27)|(0x3FB<<17)|(208<<9)|(node)):08X}"
        cmd="sendcan --msgid %s --data 0xE11EA55AC300%02X00\n" % (msgid, state)
        self.worker.send_command(cmd)

    def blankingPam_clicked(self, enabled):
        node = 0
        state=0
        if enabled:
            state=1
        msgid = f"0x{((0<<28)|(1<<27)|(0x3FB<<17)|(224<<9)|(node)):08X}"
        cmd="sendcan --msgid %s --data 0xE11EA55AC300%02X00\n" % (msgid, state)
        self.worker.send_command(cmd)

    def blankingRcvr_clicked(self, enabled):
        state=0
        if enabled:
            state=1
        msgid = f"0x082"
        cmd="sendcan --msgid %s --data 0xE11EA55AC300%02X00\n" % (msgid, state)
        self.worker.send_command(cmd)






    def btn_clicked(self):
        cmd="can\n"
        self.worker.send_command(cmd)

### CHANGE VALUES ###
    def updateVmixGainOffset(self):                                                                 # 0x30E Vmix ADC Gain & Offset
        gain =   int(self.setVmixGain.value()*10.)
        offset = int(self.setVmixOffset.value()*1000.)
        msgid = f"0x{((0<<28)|(1<<27)|(0x30E<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x%04X%04X00000000\n" % (msgid, gain, offset)
        self.worker.send_command(cmd)

    def updateImixGainOffset(self):                                                                 # 0x310 Vmix ADC Gain & Offset
        gain = int(self.setImixGain.value()*10.)
        offset = int(self.setImixOffset.value()*1000.)
        msgid = f"0x{((0<<28)|(1<<27)|(0x310<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x%04X%04X00000000\n" % (msgid, gain, offset)
        self.worker.send_command(cmd)

    def updateVmixDACOffset(self):                                                                  # 0x312 Vmix ADC Gain & Offset
        offset = int(self.setVmixDACOffset.value()*1000.) 
        msgid = f"0x{((0<<28)|(1<<27)|(0x312<<17)|(208<<9)|(29)):08X}" 
        cmd="sendcan --msgid %s --data 0x%04X000000000000\n" % (msgid, offset)
        self.worker.send_command(cmd)

    def updateRseries(self):                                                                        # 0x304 Vmix ADC Gain & Offset
        Rseries = int(self.setRseries.value()*1000.)
        msgid = f"0x{((0<<28)|(1<<27)|(0x304<<17)|(208<<9)|(29)):08X}" 
        cmd="sendcan --msgid %s --data 0x%04X000000000000\n" % (msgid, Rseries)
        self.worker.send_command(cmd)

    def updateRsense(self):                                                                         # 0x305 Vmix ADC Gain & Offset
        Rsense = int(self.setRsense.value()*1000.) 
        msgid = f"0x{((0<<28)|(1<<27)|(0x305<<17)|(208<<9)|(29)):08X}" 
        cmd="sendcan --msgid %s --data 0x%04X000000000000\n" % (msgid, Rsense)
        self.worker.send_command(cmd)

### GETS ###
    def getVmix_clicked(self):                                                                      # 0x30F Vmix ADC Gain & Offset
        msgid = f"0x{((0<<28)|(1<<27)|(0x30F<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x0000000000000000\n" % msgid
        self.worker.send_command(cmd)
        # returned values in 0x20F displayed in textBox

    def getImix_clicked(self):                                                                      # 0x311 Vmix ADC Gain & Offset
        msgid = f"0x{((0<<28)|(1<<27)|(0x311<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x0000000000000000\n" % msgid
        self.worker.send_command(cmd)
        # returned values in 0x211 displayed in textBox

    def getVmixDACOffset_clicked(self):                                                             # 0x313 Vmix ADC Gain & Offset
        msgid = f"0x{((0<<28)|(1<<27)|(0x313<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x0000000000000000\n" % msgid
        self.worker.send_command(cmd)
        # returned values in 0x213 displayed in textBox

### SAVE VALUES ###
    def writeEEPROM_clicked(self):                                                                  # 0x314 Vmix ADC Gain & Offset
        msgid = f"0x{((0<<28)|(1<<27)|(0x314<<17)|(208<<9)|(29)):08X}"
        cmd="sendcan --msgid %s --data 0x0000000000000000\n" % msgid
        self.worker.send_command(cmd)


def main():
    app = QtGui.QApplication(sys.argv)
    myWindow = MyWindowClass(None)
    myWindow.show()
    app.exec_()

if __name__ == "__main__":
    app = QApplication([])
    window = Window()
    window.show()
    sys.exit(app.exec())
