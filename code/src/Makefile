
CFLAGS = -g -D_GNU_SOURCE=1 -W -Wall -O3 -std=c99 -fno-math-errno -ffinite-math-only -fno-rounding-math -fno-signaling-nans -fno-trapping-math -fcx-limited-range -fsingle-precision-constant $(shell sdl-config --cflags) $(shell pkg-config fftw3f --cflags)
LDLIBS = -lm -lasound -lpthread $(shell sdl-config --libs) $(shell pkg-config fftw3f --libs) -lportaudio

all: main

clean:
	rm -f main *.o

main: main.o mmap_file.o pcm.o wav.o alsa.o HelperFunctions.o AudioTranscription.o AudioDataQueue.o AudioCapturePoint.o CapturedDataPoints.o MusicalDataPoint.o FFT.o AudioPreProcessing.o
