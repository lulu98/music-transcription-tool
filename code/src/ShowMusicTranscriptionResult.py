from PyQt5 import QtWebEngineWidgets, QtWidgets, QtCore, QtGui
import sys
#from midi2audio import FluidSynth
#import fluidsynth
import os
import pyaudio
import threading
import wave
import pyqtgraph as pg
import math
import numpy as np
from pdf2image import convert_from_path

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

class MelodyVisualizer(QtWidgets.QMainWindow):
    def __init__(self, parent=None,pdfFileName=None,wavFileName=None):
        super(MelodyVisualizer, self).__init__(parent)
        self.showFullScreen()

        pages = convert_from_path(pdfFileName, 100)
        for page in pages:
            page.save('../output/out.jpg', 'JPEG')

        label = QtWidgets.QLabel()
        #label.setStyleSheet("border-image:url(out.jpg)")
        pixmap = QtGui.QPixmap('../output/out.jpg')
        pixmap = pixmap.scaled(pixmap.width(),pixmap.height(),QtCore.Qt.KeepAspectRatio)
        label.setPixmap(pixmap)
        #label.setFixedWidth(500)
        label.setScaledContents(True)

        scroll = QtGui.QScrollArea()
        scroll.setWidget(label)
        scroll.setFixedWidth(pixmap.width()+20)

        self.buildWAVForm(wavFileName)
        viewBoxLayout = QtWidgets.QVBoxLayout()
        viewBoxLayout.addWidget(self.win)
        viewBox = QtWidgets.QWidget()
        viewBox.setLayout(viewBoxLayout)
        viewBox.setMinimumHeight(200)
        viewBox.setMaximumHeight(200)

        self.wavPlayer = WAVPlayer(1, wavFileName, 0.1, self.inf1)
        self.play_button = QtWidgets.QPushButton("Play WAV File")
        self.play_button.clicked.connect(self.wavPlayer.playSound)
        self.pause_button = QtWidgets.QPushButton("Pause/Continue WAV File")
        self.pause_button.clicked.connect(self.wavPlayer.pauseSound)
        self.stop_button = QtWidgets.QPushButton("Stop WAV File")
        self.stop_button.clicked.connect(self.wavPlayer.stopStound)
        self.exit_button = QtWidgets.QPushButton("Exit Application.")
        self.exit_button.clicked.connect(self.exit_application)

        button_layout = QtWidgets.QHBoxLayout()
        button_layout.addWidget(self.play_button)
        button_layout.addWidget(self.pause_button)
        button_layout.addWidget(self.stop_button)

        #layout
        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(scroll)
        layout.setAlignment(scroll,QtCore.Qt.AlignHCenter)
        layout.addWidget(viewBox)
        layout.addLayout(button_layout)
        layout.addWidget(self.exit_button)

        self.mainWidget = QtWidgets.QWidget()
        self.mainWidget.setLayout(layout)

        self.setCentralWidget(self.mainWidget)
        self.inf1LastValue = self.inf1.getPos()[0]

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
        exit(0)


def main(argv):
    path = '../output/'
    lilyPondFileName = argv[1]
    pdfFileName = argv[2]
    midiFileName = argv[3]
    wavFileName = argv[4]
    #ind = argv[1].index('.')
    #lilyPondFileName = argv[1][0:ind] + ".ly"
    #pdfFileName = argv[1][0:ind] + ".pdf"
    #midiFileName = argv[1][0:ind] + ".midi"
    #wavFileName = argv[1][0:ind] + ".wav"
    #print(lilyPondFileName)
    #csvFileName = argv[1]
    #wavFileName = argv[2]
    os.system("cd {} && lilypond {} && timidity {} -Ow".format(path,lilyPondFileName,midiFileName))
    print(lilyPondFileName)
    print(midiFileName)
    print(pdfFileName)
    print(wavFileName)

    app = QtWidgets.QApplication(sys.argv)
    app.setStyleSheet("QPushButton:hover{background-color:grey;}"
                      "QPushButton{background-color:#d2d7d8}")
    main = MelodyVisualizer(pdfFileName=path+pdfFileName,wavFileName=path+wavFileName)
    main.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main(sys.argv)