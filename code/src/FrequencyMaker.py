#!/usr/bin/env python3
import numpy
import pyaudio
import math
from PyQt5 import QtWidgets, QtCore
import sys
import functools
from time import sleep

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

def main(argv):
    playTime = 0
    frequencies = []
    if len(argv) > 1:
        playTime = int(argv[1])
        frequencies = argv[2:]
        numNotes = len(frequencies)
        for i in range(len(frequencies)):
            frequencies[i] = float(frequencies[i])
    else:
        playTime = 5
        numNotes = 3
        frequencies.append(200.0)
        frequencies.append(400.0)
        frequencies.append(500.0)
    print(frequencies)
    generators = []
    for i in range(numNotes):
        generators.append(ToneGenerator())
    for i in range(numNotes):
        generators[i].play(frequencies[i],playTime,0.3)
    sleep(playTime)

if __name__ == '__main__':
    main(sys.argv)