from PyQt5 import QtWidgets, QtCore
import sys
import csv
import pyqtgraph as pg
import numpy as np
import matplotlib.pyplot as plt
import wave
import pyaudio
import math
import threading
from PyQt5.QtCore import pyqtSignal, QObject
from time import sleep

class WAVPlayer():
    def __init__(self, i, fileName, timeStep, line):
        #threading.Thread.__init__(self)
        self.h = i
        self.fileName = fileName
        self.line = line
        self.isPaused = False
        self.isStopped = True
        self.isRunning = False
        self.timeStep = timeStep
        self.playThread = threading.Thread(target=self.__wavPlayerLifeTime)
        self.__wakeUp()

    def run(self):
        print("wav player started")

    def __wavPlayerLifeTime(self):
        p = pyaudio.PyAudio()
        while(self.isRunning):
            if not self.isStopped:
                f = wave.open(self.fileName, "rb")
                # instantiate PyAudio
                # open stream
                stream = p.open(format=p.get_format_from_width(f.getsampwidth()),
                                channels=f.getnchannels(),
                                rate=f.getframerate(),
                                output=True)

                duration = f.getnframes() / float(f.getframerate())
                #start = 0
                #length = 0.1
                chunk = 1024

                while (not self.isStopped and self.line.getPos()[0] < duration):
                    if not self.isRunning:
                        return
                    while self.isPaused:
                        pass
                    n_frames = int(self.line.getPos()[0] * f.getframerate())
                    f.setpos(n_frames)

                    # write desired frames to audio buffer
                    n_frames = int(self.timeStep * f.getframerate())
                    frames = f.readframes(n_frames)
                    stream.write(frames)

                    self.line.setPos([self.line.getPos()[0]+self.timeStep,0])
                stream.close()

                f.close()
                self.__resetSignals()
        p.terminate()

    def __wakeUp(self):
        self.isRunning = True
        self.playThread.start()

    def playSound(self):
        self.isStopped = False
        #self.playThread.start()

    def __playSound(self):
        self.isStopped = False

    def pauseSound(self):
        if not self.isStopped:
            self.isPaused = not self.isPaused

    def stopStound(self):
        self.isStopped = True
        self.__resetSignals()

    def __resetSignals(self):
        self.isPaused = False
        self.isStopped = True
        self.line.setPos([0, 0])

    def closeWAVPlayer(self):
        self.isRunning = False

    def getCurrentTime(self):
        return self.currentTime

class WAVAnalyzer(QtWidgets.QWidget):
    def __init__(self, parent=None,wavFileName=None,timeStamps=[]):
        QtWidgets.QWidget.__init__(self)

        self.timeStamps = timeStamps


        self.setLayout(layout)



    def close_wavPlayer(self):
        self.wavPlayer.closeWAVPlayer()
        self.close()

class AudioDataAnalyzer(QtWidgets.QMainWindow):
    def __init__(self, parent=None,csvFileName=None,wavFileName=None):
        super(AudioDataAnalyzer, self).__init__(parent)
        self.showFullScreen()

        rows, cols = self.getCSVDimesions(csvFileName)
        numSamples = rows - 1
        numBins = cols - 1

        self.t, self.f, self.Sxx = self.extractCSVFile(csvFileName, numSamples, numBins)
        self.SxxT = self.Sxx.transpose()

        self.buildWAVForm(wavFileName)
        viewBoxLayout = QtWidgets.QVBoxLayout()
        viewBoxLayout.addWidget(self.win)
        viewBox = QtWidgets.QWidget()
        viewBox.setLayout(viewBoxLayout)
        viewBox.setMinimumHeight(200)
        viewBox.setMaximumHeight(200)

        self.buildHistogram()
        self.buildBinPlot()

        # ------------------slider
        self.slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.slider.setMinimum(0)
        self.slider.setMaximum(numSamples - 1)
        self.slider.setValue(0)
        self.slider.setTickPosition(QtWidgets.QSlider.TicksBelow)
        self.slider.setTickInterval(1)
        self.slider.valueChanged.connect(self.updateBinPlot)
        # ---------------------
        self.currentInformation = QtWidgets.QLabel()
        self.currentInformation.setText(
            'Current Time: {}\t\t Current Bin: {}\t\t #Bins: {}\t\t Main Frequency: {}\t\t Amplitude: {}'.format(
                self.t[self.slider.value()], self.slider.value() + 1, self.slider.maximum() + 1,
                self.f[np.argmax(self.SxxT[self.slider.value()])], max(self.SxxT[self.slider.value()])))



        self.wavPlayer = WAVPlayer(1, wavFileName, 0.1, self.inf1)
        # ---------------------
        self.play_button = QtWidgets.QPushButton("Play WAV File")
        self.play_button.clicked.connect(self.wavPlayer.playSound)
        self.pause_button = QtWidgets.QPushButton("Pause/Continue WAV File")
        self.pause_button.clicked.connect(self.wavPlayer.pauseSound)
        self.stop_button = QtWidgets.QPushButton("Stop WAV File")
        self.stop_button.clicked.connect(self.wavPlayer.stopStound)
        # ---------------------
        button_layout = QtWidgets.QHBoxLayout()
        button_layout.addWidget(self.play_button)
        button_layout.addWidget(self.pause_button)
        button_layout.addWidget(self.stop_button)

        self.exit_button = QtWidgets.QPushButton("Exit Application.")
        self.exit_button.clicked.connect(self.exit_application)

        #layout
        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.histogram)
        layout.addWidget(self.slider)
        layout.addWidget(self.binPlot)
        layout.addWidget(self.currentInformation)
        layout.addWidget(viewBox)
        layout.addLayout(button_layout)
        layout.addWidget(self.exit_button)

        self.mainWidget = QtWidgets.QWidget()
        self.mainWidget.setLayout(layout)

        self.inf1LastValue = self.inf1.getPos()[0]

        self.setCentralWidget(self.mainWidget)
        self.currentPositionThread = threading.Thread(target=self.__waitForTimeChange)
        self.isCurrentPositionThreadRunning = True
        self.currentPositionThread.start()

    def __waitForTimeChange(self):
        while self.isCurrentPositionThreadRunning:
            if self.inf1LastValue != self.inf1.getPos()[0]:
                self.slider.setValue(self.getCorrespondingSliderValue(self.inf1.getPos()[0]))
                self.inf2.setPos([self.inf1.getPos()[0],0])
                #self.updateBinPlot()
            self.inf1LastValue = self.inf1.getPos()[0]
            sleep(0.05)


    def buildBinPlot(self):
        #build bar chart
        self.binPlot = pg.GraphicsLayoutWidget()
        self.p2 = self.binPlot.addPlot()
        y1 = np.transpose(self.Sxx)[0]

        # create horizontal list
        x = self.f

        # create bar chart
        self.bg1 = pg.BarGraphItem(x=x, height=y1, width=0.0001, brush='r')
        self.p2.setMouseEnabled(1, 0)
        self.p2.setLimits(xMin=0, xMax=self.f[-1])
        self.p2.setLabel('bottom', "Frequency", units='Hz')
        # If you include the units, Pyqtgraph automatically scales the axis and adjusts the SI prefix (in this case kHz)
        self.p2.setLabel('left', "Power Level", units='dB')
        self.p2.setYRange(0, 0.15)
        self.p2.addItem(self.bg1)

    def buildHistogram(self):
        pg.setConfigOption('background', 'k')
        pg.setConfigOptions(imageAxisOrder='row-major')
        self.histogram = pg.GraphicsLayoutWidget()

        self.inf2 = pg.InfiniteLine(movable=False, angle=90, label='t={value:0.2f}',
                                    labelOpts={'position': 0.5, 'color': (0, 0, 0), 'fill': (200, 200, 200, 50),
                                               'movable': False})
        self.inf3 = pg.InfiniteLine(movable=True, angle=0, label='t={value:0.2f}',
                                    labelOpts={'position': 0.5, 'color': (0, 0, 0), 'fill': (200, 200, 200, 50),
                                               'movable': True})
        self.inf3.setPos([0,1000])
        self.histogramPlot = self.histogram.addPlot()
        img = pg.ImageItem()
        self.histogramPlot.addItem(img)
        self.histogramPlot.addItem(self.inf2)
        self.histogramPlot.addItem(self.inf3)
        hist = pg.HistogramLUTItem()
        hist.setImageItem(img)
        self.histogram.addItem(hist)
        hist.setLevels(np.min(self.Sxx), np.max(self.Sxx))

        hist.gradient.restoreState(
            {'mode': 'rgb',
             'ticks': [(0.5, (0, 182, 188, 255)),
                       (1.0, (246, 111, 0, 255)),
                       (0.0, (75, 0, 113, 255))]})
        img.setImage(self.Sxx)
        img.scale(self.t[-1] / np.size(self.Sxx, axis=1),
                  self.f[-1] / (np.size(self.Sxx, axis=0)))
        self.histogramPlot.setLimits(xMin=0, xMax=self.t[-1], yMin=0, yMax=self.f[-1])
        self.histogramPlot.setLabel('bottom', "Time", units='s')
        self.histogramPlot.setLabel('left', "Frequency", units='Hz')

        plt.pcolormesh(self.t, self.f, self.Sxx)
        plt.ylabel('Frequency [Hz]')
        plt.xlabel('Time [sec]')
        plt.colorbar()

    def mouseDoubleClickEvent(self, event):
        mousePos = event.pos()
        if (mousePos.x() > self.histogram.pos().x() and
            mousePos.x() < self.histogram.pos().x() + self.histogram.width() and
            mousePos.y() > self.histogram.pos().y() and
            mousePos.y() < self.histogram.pos().y() + self.histogram.height()):
            newPos = (self.histogramPlot.getAxis('left').range[1]-self.histogramPlot.getAxis('left').range[0])/2 + self.histogramPlot.getAxis('left').range[0]
            self.inf3.setPos([0,newPos])

    def extractCSVFile(self,fileName,numSamples,numBins):
        Sxx = []
        t = []
        f = []
        with open(fileName, newline='', encoding='utf-8') as ifile:
            fileInput = csv.reader(ifile, delimiter=';')
            for i, row in enumerate(fileInput):
                if i == 0:
                    # numBins = len(row[1:])
                    for cell in row[1:]:
                        f.append(cell)
                    continue
                t.append(row[0])
                for cell in row[1:]:
                    Sxx.append(cell)
                # numSamples = i

        if len(Sxx) % numBins > 0:
            while (len(Sxx) % numBins > 0):
                Sxx.append(0)
        Sxx = np.array(Sxx).reshape(numSamples, numBins)
        Sxx = np.transpose(Sxx).astype(np.float)
        t = np.array(t).astype(np.float)
        f = np.array(f).astype(np.float)
        return t,f,Sxx

    def updateBinPlot(self):
        y1 = self.SxxT[self.slider.value()]
        self.bg1.setOpts(height=y1)
        self.p2.setYRange(0,max(y1)+0.01)
        self.currentInformation.setText(
            'Current Time: {}\t\t Current Bin: {}\t\t #Bins: {}\t\t Main Frequency: {}\t\t Amplitude: {}'.format(
                self.t[self.slider.value()], self.slider.value() + 1, self.slider.maximum() + 1,
                self.f[np.argmax(self.SxxT[self.slider.value()])], max(self.SxxT[self.slider.value()])))
        if self.wavPlayer.isStopped:
            self.inf1LastValue = self.inf1.getPos()[0]
            self.inf1.setPos([self.t[self.slider.value()], 0])
            self.inf2.setPos([self.t[self.slider.value()], 0])

    def getCSVDimesions(self,fileName):
        rows = 0
        cols = 0
        with open(fileName) as f:
            rows = sum(1 for line in f)

        with open(fileName, newline='', encoding='utf-8') as ifile:
            fileInput = csv.reader(ifile, delimiter=';')
            N = 0
            for i, row in enumerate(fileInput):
                if i == N:
                    cols = len(row)
                    break

        return rows,cols

    def getCorrespondingSliderValue(self,value):
        for i in range(len(self.t)):
            if value < self.t[i]:
                return i-1
        return 0

    def buildWAVForm(self,fileName):
        pg.setConfigOption('background', 'w')
        #pg.setConfigOption('foreground', 'k')
        self.win = pg.GraphicsLayoutWidget()

        p1 = self.win.addPlot()
        self.inf1 = pg.InfiniteLine(movable=True, angle=90, label='t={value:0.2f}',
                               labelOpts={'position': 0.1, 'color': (0, 0, 0), 'fill': (200, 200, 200, 50),
                                          'movable': True})
        with wave.open(fileName, 'r') as wav_file:
            # Extract Raw Audio from Wav File
            duration = wav_file.getnframes()/float(wav_file.getframerate())
            print(duration)
            p1.setLimits(xMin=0, xMax=duration,yMin=-math.pow(2,16),yMax=math.pow(2,16))

            signal = wav_file.readframes(-1)
            signal = np.fromstring(signal, 'Int16')

            # Split the data into channels
            channels = [[] for channel in range(wav_file.getnchannels())]
            for index, datum in enumerate(signal):
                channels[index % len(channels)].append(datum)

            # Get time from indices
            fs = wav_file.getframerate()
            Time = np.linspace(0, len(signal) / len(channels) / fs, num=len(signal) / len(channels))

            # Plot
            for channel in channels:
                p1.plot(Time, channel,pen=pg.mkPen("3366ff",width=1))

            p1.setYRange(-math.pow(2,16), math.pow(2,16))
            self.inf1.setPos([0, 0])
            p1.addItem(self.inf1)
            p1.hideButtons()
            p1.setMouseEnabled(x=False,y=False)
            p1.getAxis('left').hide()
            p1.setLabel(axis='bottom',text='Time',units='s')
            self.inf1.setBounds([0,duration])

    def exit_application(self):
        self.wavPlayer.closeWAVPlayer()
        self.isCurrentPositionThreadRunning = False
        self.close()
        exit(0)


def main(argv):
    csvFileName = argv[1]
    wavFileName = argv[2]

    app = QtWidgets.QApplication(sys.argv)
    app.setStyleSheet("QPushButton:hover{background-color:grey;}"
                      "QPushButton{background-color:#d2d7d8}")
    main = AudioDataAnalyzer(csvFileName=csvFileName,wavFileName=wavFileName)
    main.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main(sys.argv)