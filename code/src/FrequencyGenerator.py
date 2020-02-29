#!/usr/bin/env python

import numpy
import pyaudio
import math
from PyQt5 import QtWidgets, QtCore
import sys
import functools

class ToneGenerator(object):

    def __init__(self, samplerate=44100, frames_per_buffer=4410):
        self.p = pyaudio.PyAudio()
        self.samplerate = samplerate
        self.frames_per_buffer = frames_per_buffer
        self.streamOpen = False
        self.isPlaying = False

    def sinewave(self):
        if self.buffer_offset + self.frames_per_buffer - 1 > self.x_max:
            # We don't need a full buffer or audio so pad the end with 0's
            xs = numpy.arange(self.buffer_offset,
                              self.x_max)
            tmp = self.amplitude * numpy.sin(xs * self.omega)
            out = numpy.append(tmp,
                               numpy.zeros(self.frames_per_buffer - len(tmp)))
        else:
            xs = numpy.arange(self.buffer_offset,
                              self.buffer_offset + self.frames_per_buffer)
            out = self.amplitude * numpy.sin(xs * self.omega)
        self.buffer_offset += self.frames_per_buffer
        return out

    def callback(self, in_data, frame_count, time_info, status):
        '''if self.buffer_offset < self.x_max:
            data = self.sinewave().astype(numpy.float32)
            return (data.tostring(), pyaudio.paContinue)
        else:
            #self.buffer_offset = 0
            #data = self.sinewave().astype(numpy.float32)
            #return (data.tostring(), pyaudio.paContinue)
            return (None, pyaudio.paComplete)'''
        if self.isPlaying:
            if self.buffer_offset < self.x_max:
                data = self.sinewave().astype(numpy.float32)
                return (data.tostring(), pyaudio.paContinue)
            else:
                self.buffer_offset = 0
                data = self.sinewave().astype(numpy.float32)
                return (data.tostring(), pyaudio.paContinue)
        else:
            return (None, pyaudio.paComplete)

    def is_playing(self):
        if self.stream.is_active():
            return True
        else:
            if self.streamOpen:
                self.stream.stop_stream()
                self.stream.close()
                self.streamOpen = False
            self.isPlaying = False
            return False

    def play(self, frequency, duration, amplitude):
        self.isPlaying = True
        self.omega = float(frequency) * (math.pi * 2) / self.samplerate
        self.amplitude = amplitude
        self.buffer_offset = 0
        self.streamOpen = True
        self.x_max = math.ceil(self.samplerate * duration) - 1
        self.stream = self.p.open(format=pyaudio.paFloat32,
                                  channels=1,
                                  rate=self.samplerate,
                                  output=True,
                                  frames_per_buffer=self.frames_per_buffer,
                                  stream_callback=self.callback)

    def stopTone(self):
        self.isPlaying = False

    def changeFrequency(self,frequency):
        self.omega = float(frequency) * (math.pi * 2) / self.samplerate

    def changeAmplitude(self,amplitude):
        if amplitude > 0 and amplitude <= 1.0:
            self.amplitude = amplitude


class FrequencyControlWidget(QtWidgets.QWidget):
    def __init__(self):
        QtWidgets.QWidget.__init__(self)

        self.generator = ToneGenerator()

        self.isPlaying = False

        self.setUpFrequencies()

        layout = QtWidgets.QHBoxLayout()

        self.frequencySlider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.frequencySlider.setMaximum(1000)
        self.frequencySlider.setMinimum(1)
        self.frequencySlider.setValue(200)

        self.frequencyLabel = QtWidgets.QLabel()
        self.frequencyLabel.setText(str(self.frequencySlider.value()))

        self.musicalNoteDropDownMenu = QtWidgets.QComboBox()
        self.musicalNoteDropDownMenu.addItems(["c","cis/des","d","dis/es","e","f","fis/ges","g","gis/as","a","ais/b","h"])
        self.octaveDropDownMenu = QtWidgets.QComboBox()
        self.octaveDropDownMenu.addItems(["1","2","3","4","5","6","7"])

        self.amplitudeSlider = QtWidgets.QSlider()
        self.amplitudeSlider.setMaximum(100)
        self.amplitudeSlider.setMinimum(0)
        self.amplitudeSlider.setValue(10)
        self.amplitudeSlider.setSingleStep(2)

        self.play_button = QtWidgets.QPushButton("Play")
        self.stop_button = QtWidgets.QPushButton("Stop")

        self.play_button.clicked.connect(self.playTone)
        self.stop_button.clicked.connect(self.stopTone)
        self.frequencySlider.valueChanged.connect(self.updateFrequency)
        self.amplitudeSlider.valueChanged.connect(self.updateAmplitude)
        self.musicalNoteDropDownMenu.currentIndexChanged.connect(self.setFrequencyAccordingToMusicalNote)
        self.octaveDropDownMenu.currentIndexChanged.connect(self.setFrequencyAccordingToMusicalNote)

        layout.addWidget(self.frequencySlider)
        layout.addWidget(self.frequencyLabel)
        layout.addWidget(self.musicalNoteDropDownMenu)
        layout.addWidget(self.octaveDropDownMenu)
        layout.addWidget(self.amplitudeSlider)
        layout.addWidget(self.play_button)
        layout.addWidget(self.stop_button)

        self.setLayout(layout)

    def setUpFrequencies(self):
        tuningPitch = 440.0
        self.basicFrequencies = []
        for i in range(12):
            freq = tuningPitch * math.pow(math.pow(2, 1 / 12), i-9) * math.pow(2, -3)
            self.basicFrequencies.append(freq)


    def playTone(self):
        if not self.isPlaying:
            amplitude = self.amplitudeSlider.value()/100
            self.generator.play(self.frequencySlider.value(),5,amplitude)
            self.isPlaying = True

    def stopTone(self):
        self.isPlaying = False
        self.generator.stopTone()

    def setFrequencyAccordingToMusicalNote(self,i):
        frequency = self.basicFrequencies[self.musicalNoteDropDownMenu.currentIndex()] * math.pow(2,self.octaveDropDownMenu.currentIndex())
        self.frequencySlider.setValue(round(frequency))

    def updateFrequency(self):
        self.frequencyLabel.setText(str(self.frequencySlider.value()))
        self.generator.changeFrequency(self.frequencySlider.value())

    def updateAmplitude(self):
        self.generator.changeAmplitude(self.amplitudeSlider.value()/100)

class FrequencyPage(QtWidgets.QMainWindow):
    def __init__(self, numNotes=3, parent=None):
        super(FrequencyPage, self).__init__(parent)
        self.showFullScreen()

        frequencyControls = []
        for i in range(numNotes):
            frequencyControls.append(FrequencyControlWidget())

        #frequencyControl1 = FrequencyControlWidget()
        #frequencyControl2 = FrequencyControlWidget()
        #frequencyControl3 = FrequencyControlWidget()

        self.exit_button = QtWidgets.QPushButton("Close Application")
        self.exit_button.clicked.connect(self.exit_application)

        layout = QtWidgets.QVBoxLayout()
        for control in frequencyControls:
            layout.addWidget(control)
        #layout.addWidget(frequencyControl1)
        #layout.addWidget(frequencyControl2)
        #layout.addWidget(frequencyControl3)
        layout.addWidget(self.exit_button)

        self.mainWidget = QtWidgets.QWidget()
        self.mainWidget.setLayout(layout)

        self.setCentralWidget(self.mainWidget)

    def exit_application(self):
        self.close()
        exit(0)



def main(argv):
    if len(argv) > 1:
        numNotes = int(argv[1])
    else:
        numNotes = 3
    app = QtWidgets.QApplication(sys.argv)
    app.setStyleSheet("QPushButton:hover{background-color:grey;}"
                      "QPushButton{background-color:#d2d7d8}"
                      ".QSlider {min-height: 68px;max-height: 68px;background: white;}"
                      ".QSlider::groove:horizontal {border: 1px solid #262626;height: 5px;background: #393939;margin: 0 12px;}"
                      ".QSlider::handle:horizontal {background: #22B14C;border: 5px solid #B5E61D;width: 23px;height: 100px;margin: -24px -12px;}"
                      )
    main = FrequencyPage(numNotes)
    main.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main(sys.argv)