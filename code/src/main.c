/**
@file main.c
@author Lukas Graber
@date 14 August 2019
@brief Implementation of the music transcription pipeline.
**/
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <assert.h>

#include "../include/Structures.h"
#include "../include/pcm.h"
//#include "../include/trans.h"
#include "../include/HelperFunctions.h"
#include "../include/ApplicationMacros.h"
#include "../include/AudioTranscription.h"
#include "../include/AudioPreProcessing.h"
#include "../include/FFT.h"

SongConfiguration songConfiguration;
RunTimeInformation runTimeInformation;
AudioDataQueue audioDataQueue;

struct timespec start_timer, current_timer;

struct timespec whole_run_start_t, single_run_start_t, fft_run_start_t, current_time_t, start_audio_capture_t, start_audio_preprocessing_t, start_audio_transcription_t;
int runs;
double runTime;
double fftRunTime;
double audioPreProcessingTime;
double audioTranscriptionTime;
double audioCaptureTime;

static int numBins = 1;
//static char* PATH = "../output/";
static char* WAV_FILE_NAME = "../output/wavfile.wav";
static char* CSV_FILE_NAME = "../output/wavfile.csv";
//static char* LILYPOND_FILE_NAME = "melody.ly";
//static char* PDF_FILE_NAME = "melody.pdf";
//static char* MIDI_FILE_NAME = "melody.midi";
//static char* MELODY_FILE_NAME = "melody.wav";
static char* BEAT_AUDIO_FILE = "../assets/MetroBar1.wav";

char *benchmarkingMelody = "";
char *benchmarkingMode = "";
char *benchmarkingFileName = "";

float __rate = 0;
int __channels = 0;
static int __isAudioInterfaceReady = 0;
static int __isRecording = 0;
//static int __isMidiFilePlaying = 0;
//static int __isCapturingAudio = 0;
static int __isAudioProcessing = 0;
int retError = 0;

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /*!< mutex to manage access no queue where data is shared between threads.**/
pthread_mutex_t mutex;

void fail(){
  printf("%s\n", "Something went wrong!");
  exit(0);
}

/**
@brief This function compares measured melody with test melody.
At first, the function will translate the lilypond string into frequencies and note durations stored in arrays. This is done, such that
the captured audio data can be easier compared to the test melody. The values go through some metrics.
**/
void compareCapturedDataToOriginal(char *testMelody, char *testIdentifier, char *csvFileName, CapturedDataPoints *capturedDataPoints){
    int base = 0;
    int offset = 0;
    int length = 1024;
    double frequencies[length];
    double noteLength[length];
    int ind = 0;
    char *melodyExpression = (char *)calloc(length,sizeof(char));
    char *noteLengthExpression = (char *)calloc(length,sizeof(char));
    while((unsigned)(base + offset) < strlen(testMelody)){
        while((unsigned)(base + offset) < strlen(testMelody) && testMelody[base+offset] != ' '){
            offset++;
        }
        int temp = 0;

        while(temp < offset && testMelody[base+temp] != '1' && testMelody[base+temp] != '2' && testMelody[base+temp] != '4' && testMelody[base+temp] != '8'){
            melodyExpression[temp] = testMelody[base+temp];
            temp++;
        }
        int k = 0;
        while(temp < offset){
            noteLengthExpression[k] = testMelody[base + temp];
            temp++;
            k++;
        }
        frequencies[ind] = getFrequencyToMusicalNote(melodyExpression,runTimeInformation.tuningPitch);
        noteLength[ind] = getNoteLength(noteLengthExpression,runTimeInformation.beatsPerMinute);
        ind++;
        base = base + offset;
        offset = 0;
        base++;
        memset(melodyExpression, '\0', sizeof(char) * length);
        memset(noteLengthExpression,'\0',sizeof(char) * length);
    }
    free(melodyExpression);
    free(noteLengthExpression);

    printf("%s\n\n", "###################################################################");
    printf("Benchmarking started.\n\n");

    double frequencySampleVector[ind];
    double centSampleVector[ind];
    double rhythmSampleVector[ind];

    double absoluteFrequencyErrorValue = 0;
    double absoluteExponentialFrequencyErrorValue = 0;
    double absoluteCentErrorValue = 0;
    double absoluteExponentialCentErrorValue = 0;
    double absoluteRhythmErrorValue = 0;
    double absoluteExponentialRhythmErrorValue = 0;

    //double correctnessValue = 0;

    //--------------------------------------------------
    double timeStamp = 0;
    double timeThreshold = 0;
    int referencePos = 0;
    int recordedPos = 0;
    int numCorrectDetected = 0;

    double realTimeFrequency = 0;
    double postProcessingFrequency = 0;
    double frequencyDifference =0;
    double centDifference = 0;
    double timeDifference = 0;

    for(referencePos = 0; referencePos < ind; referencePos++){
        timeThreshold = timeThreshold + noteLength[referencePos];
        int isCorrectDetected = 0;

        if((unsigned)recordedPos < capturedDataPoints->pos){
          realTimeFrequency = capturedDataPoints->arr[recordedPos].frequency;
          postProcessingFrequency = frequencies[referencePos];
          frequencyDifference = fabs(realTimeFrequency - postProcessingFrequency);
          centDifference = fabs(1200 * log(realTimeFrequency/postProcessingFrequency)/log(2));
          timeDifference = noteLength[referencePos];
        }
        while((unsigned)recordedPos < capturedDataPoints->pos && timeStamp < timeThreshold){
            double currentDifference = fabs(1200 * log(capturedDataPoints->arr[recordedPos].frequency/postProcessingFrequency)/log(2));
            //if(strcmp(getMusicalNote(frequencies[referencePos],runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents),getMusicalNote(capturedDataPoints->arr[recordedPos].frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents))==0){
            if(currentDifference < runTimeInformation.pitchResolutionInCents){
                isCorrectDetected = 1;
                frequencyDifference = capturedDataPoints->arr[recordedPos].frequency - postProcessingFrequency;
                centDifference = currentDifference;
                timeDifference = fabs(timeDifference - capturedDataPoints->arr[recordedPos].duration);
            }
            timeStamp = timeStamp + capturedDataPoints->arr[recordedPos].duration;
            recordedPos++;
        }
        numCorrectDetected = numCorrectDetected + isCorrectDetected;

        absoluteFrequencyErrorValue = absoluteFrequencyErrorValue + fabs(frequencyDifference);
        absoluteExponentialFrequencyErrorValue = absoluteExponentialFrequencyErrorValue + powl(fabs(frequencyDifference),2.0);
        absoluteCentErrorValue = absoluteCentErrorValue + fabs(centDifference);
        absoluteExponentialCentErrorValue = absoluteExponentialCentErrorValue + powl(fabs(centDifference),2.0);
        absoluteRhythmErrorValue  = absoluteRhythmErrorValue + fabs(timeDifference);
        absoluteExponentialRhythmErrorValue = absoluteExponentialRhythmErrorValue + powl(timeDifference,2.0);

        frequencySampleVector[referencePos] = frequencyDifference;
        centSampleVector[referencePos] = centDifference;
        rhythmSampleVector[referencePos] = timeDifference;

        realTimeFrequency = 0;
        postProcessingFrequency = 0;
        frequencyDifference =0;
        centDifference = 0;
        timeDifference = 0;
    }
    printf("%d/%d notes correctly detected!\n\n",numCorrectDetected,ind);
    //--------------------------------------------------------

    double frequencyVariance = 0;
    double centVariance = 0;
    double rhythmVariance = 0;

    double frequencyMean = absoluteFrequencyErrorValue/(double)ind;
    double centMean = absoluteCentErrorValue/(double)ind;
    double rhythmMean = absoluteRhythmErrorValue / (double)ind;

    for (int i = 0; i < ind; i++) {
      frequencyVariance += powl(frequencySampleVector[i]-frequencyMean,2.0);
      centVariance += powl(centSampleVector[i]-centMean, 2.0);
      rhythmVariance += powl(rhythmSampleVector[i]-rhythmMean,2.0);
    }

    frequencyVariance = frequencyVariance / (double)(ind-1);
    centVariance = centVariance / (double)(ind-1);
    rhythmVariance = rhythmVariance / (double)(ind-1);

    printf("%s\n\n", "###################################################################");

    printf("Absolute Frequency Error: %f\n", absoluteFrequencyErrorValue);
    printf("Average Frequency Error: %f\n\n",frequencyMean);

    printf("Absolute Exponential Frequency Error: %f\n", absoluteExponentialFrequencyErrorValue);
    printf("Average Exponential Frequency Error: %f\n\n",absoluteExponentialFrequencyErrorValue/(double)pow(ind,2));

    printf("Frequency Variance: %f\n", frequencyVariance);
    printf("Frequency Standard Deviation: %f\n\n", sqrt(frequencyVariance));

    printf("%s\n\n", "###################################################################");

    printf("Absolute Cent Error: %f\n", absoluteCentErrorValue);
    printf("Average Cent Error: %f\n\n", centMean);

    printf("Absolute Exponential Cent Error: %f\n", absoluteExponentialCentErrorValue);
    printf("Average Exponential Cent Error: %f\n\n",absoluteExponentialCentErrorValue/(double)pow(ind,2));

    printf("Cents Variance: %f\n", centVariance);
    printf("Cents Standard Deviation: %f\n\n", sqrt(centVariance));

    printf("%s\n\n", "###################################################################");

    printf("Absolute Rhythm Error:%f\n",absoluteRhythmErrorValue);
    printf("Average Rhythm Error:%f\n\n",rhythmMean);

    printf("Absolute Exponential Rhythm Error: %f\n", absoluteExponentialRhythmErrorValue);
    printf("Average Exponential Rhythm Error: %f\n\n",absoluteExponentialRhythmErrorValue/(double)pow(ind,2));

    printf("Rhythm Variance: %f\n", rhythmVariance);
    printf("Rhythm Standard Deviation: %f\n\n", sqrt(rhythmVariance));

    printf("%s\n\n", "###################################################################");
    printf("%s\n\n", "Benchmarking finished.");
    printf("%s\n\n", "###################################################################");

    FILE *fp;
    //filename=strcat(filename,".csv");
    long int size=0;
    /*Open file in Read Mode*/
    fp=fopen(csvFileName,"r");
    /*Move file point at the end of file.*/
    fseek(fp,0,SEEK_END);
    /*Get the current position of the file pointer.*/
    size=ftell(fp);
    fp=fopen(csvFileName,"a+");

    if(size!=-1){
      if(size == 0){
          fprintf(fp,"Identifier; Melody; Tempo; Instrument; Windowing Function; Num Notes; Notes Correct Detected; Notes Incorrect Detected; Frequency Mean; Frequency Standard Deviation; Cent Mean; Cent Standard Deviation; Rhythm Mean; Rhythm Standard Deviation\n");
      }
    }
    else{
      printf("There is some ERROR.\n");
      exit(0);
    }
    fprintf(fp,"%s; %s; %d; %s; %s; %d; %d; %d; %f; %f; %f; %f; %f; %f\n",testIdentifier, testMelody, runTimeInformation.beatsPerMinute, runTimeInformation.instrument, runTimeInformation.windowingFunction, ind, numCorrectDetected, (ind-numCorrectDetected), frequencyMean, sqrt(frequencyVariance), centMean, sqrt(centVariance), rhythmMean, sqrt(rhythmVariance));
    fclose(fp);
}

/**
@brief This function creates and displays a notesheet.
The function will receive a melody string encoded in the lilypond format. A lilypond
file will be created out of this string. Depending on the underlying technical system,
a notesheet will be created and displayed by calling a python file. If the underlying
system is an arm system (raspberry pi), a lilypond web interface is called with the
melody string url encoded as GET parameter.
@param musicalExpression string containing the recognized melody in lilypond format
**/
void GenerateNoteSheet(char *musicalExpression){
  printf("Melody: %s\n", musicalExpression);
  printf("%s", "Enter title of song: ");
  char title[100];
  char *inputResult;
  inputResult = fgets(title, 100, stdin);
  if (inputResult == NULL) {
    fail();
  }
  if (title[0] == '\n') {
    inputResult = fgets(title, 100, stdin);
    if (inputResult == NULL) {
      fail();
    }
  }
  title[strlen(title)-1] = '\0';
  strcpy(runTimeInformation.title, title);
  printf("Title: '%s'\n\n", runTimeInformation.title);
  printf("%s", "Enter name of composer: ");
  char composer[100];
  inputResult = fgets(composer, 100, stdin);
  if (inputResult == NULL) {
    fail();
  }
  if (title[0] == '\n') {
    inputResult = fgets(composer, 100, stdin);
    if (inputResult == NULL) {
      fail();
    }
  }
  composer[strlen(composer)-1] = '\0';
  strcpy(runTimeInformation.composer, composer);
  printf("Composer: '%s'\n\n", runTimeInformation.composer);

  char *path = (char *)calloc(strlen(runTimeInformation.path) + 1, sizeof(char));
  strcpy(path, runTimeInformation.path);
  char *fileLocation = append(path,runTimeInformation.lilyPondFileName);
  GenerateBenchmarkingLilyPondFile(musicalExpression, fileLocation, runTimeInformation.beatsPerMinute, runTimeInformation.instrument, runTimeInformation.title, runTimeInformation.composer);
  //GenerateLilyPondFile(musicalExpression,fileLocation);
  free(fileLocation);
  printf("%s\n", "hello");
  if (!runTimeInformation.raspi){
    FILE *tempFile;
    char path[1035];
    tempFile = popen("command -v lilypond", "r");
    if(tempFile == NULL) {
        printf("cleaning up...\n");
        exit(0);
    }
    if(fgets(path, sizeof(path)-1, tempFile) != NULL){
        if(strcmp(path, "")!=0){
            printf("Create midi output file and notesheet as pdf.\n");
            char *command = (char *) calloc(strlen("cd ../output && ") + strlen("lilypond ") + strlen(runTimeInformation.lilyPondFileName) + sizeof(NULL),sizeof(char));
            strcpy(command, "cd ../output && ");
            strcat(command,"lilypond ");
            strcat(command,runTimeInformation.lilyPondFileName);
            retError = system(command);
            if (retError == -1) {
              fail();
            }
            char *guiCommand = (char *) calloc(1024,sizeof(char));
            sprintf(guiCommand, "python3 ShowMusicTranscriptionResult.py %s %s %s %s",runTimeInformation.lilyPondFileName,runTimeInformation.pdfFileName,runTimeInformation.midiFileName,runTimeInformation.melodyFileName);
            retError = system(guiCommand);
            if (retError == -1) {
              fail();
            }
            free(command);
            free(guiCommand);
        }else{
            printf("Lilypond is not installed!\n");
        }
    }
  }else{
    char *webCommand = (char *) calloc(1024,sizeof(char));
    sprintf(webCommand, "python3 OpenLilyPondFrontend.py %s%s", runTimeInformation.path, runTimeInformation.lilyPondFileName);
    retError = system(webCommand);
    if (retError == -1) {
      fail();
    }
    free(webCommand);
  }
}

/**
@brief This function implements a metronome.
The function receives the location of the WAV file as an argument. While the audio capture is still recording data, play the metronome tick.
The time between two ticks is calculated, also considering the time to play the wav file. This achieves an accurate metronome tick.
**/
void *metronome_entry_point(void *arg){
    char *beatAudioFile = (char *)arg;
    if(runTimeInformation.headPhones && !runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking){
        printf("metronome started.\n");
        double timePerBeat = 60.0*1000.0/(double)runTimeInformation.beatsPerMinute;
        double timeForSound = getDurationOfAudioFileInSeconds(beatAudioFile) * 1000.0;
        double sleepTime = timePerBeat-timeForSound;
        char command[1024];
        sprintf(command,"play -q %s",beatAudioFile);
        //while(__isRecording){
        while(runTimeInformation.isCapturingAudio){
            retError = system(command);
            if (retError == -1) {
              fail();
            }
            //system("play -q MetroBar1.wav");
            msleep(sleepTime);
        }
        printf("metronome stopped.\n");
    }
    return NULL;
}

/**
@brief This function is the entry point for the audio capturing/recording thread.
A thread will be created by calling this function as the entry point of the corresponding
thread. It will collect audio samples from the audio interface as long as the recording
time is not up. The collected samples will be put in a queue that is shared data structure with
the audio processing thread. Before writing to the queue, the thread needs to contact
the mutex to block other threads from accessing the queue. As soon as the writing operation
is done, the thread needs to unlock the queue by contacting the mutex again.
**/
void *audio_capture_entry_point(void *arg){
  struct pcm *pcm;
  char *pcm_name = (char *)arg;
  if (!open_pcm_read(&pcm, pcm_name))
    exit(0);

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  __rate = rate;
  __channels = channels;

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));

  pthread_t metronomeThread;
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    double timePerBeat = 60.0*1000.0/(double)runTimeInformation.beatsPerMinute;
    double timeForSound = getDurationOfAudioFileInSeconds(BEAT_AUDIO_FILE) * 1000.0;
    double sleepTime = timePerBeat-timeForSound;
    char command[1024];
    sprintf(command,"play -q %s",BEAT_AUDIO_FILE);
    printf("Start Audio Capture:\n");
    for(int i = 1; i <= RHYTHM_NUMERATOR;i++){
        printf("%d\n",i);
        retError = system(command);
        if (retError == -1) {
          fail();
        }
        //system("play -q MetroBar1.wav"); //pass as argument
        msleep(sleepTime);
    }
  }
  pthread_create(&metronomeThread,NULL,metronome_entry_point,BEAT_AUDIO_FILE);

  __isAudioInterfaceReady = 1;

  runTimeInformation.isCapturingAudio = 1;

  if (runTimeInformation.melodyBenchmarking) {
    while (!runTimeInformation.isMidiFilePlaying) {
      msleep(5);
    }
  }

  struct timespec start_t, current_t;
	clock_gettime(CLOCK_MONOTONIC_RAW,&start_t);
	clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);

  double currentTime = (current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0;
  __isRecording = (currentTime < runTimeInformation.recordingTime*1000);
  while (__isRecording) {
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_capture_t);
    if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
      memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    audioCaptureTime += (current_time_t.tv_sec - start_audio_capture_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_capture_t.tv_nsec)/1000000.0;

    AudioCapturePoint *dataPoint = initAudioCapturePoint(channels * runTimeInformation.stepSize);
    dataPoint->captureTime = (current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0;
    for(int i = 0; i < channels * runTimeInformation.stepSize;i++){
      insertAudioCapturePoint(dataPoint, buff[i]);
    }
    pthread_mutex_lock(&mutex);
    //queue_enqueue(&audioDataQueue, &dataPoint);
    queue_enqueue(&audioDataQueue, dataPoint);
    pthread_mutex_unlock(&mutex);
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);
    double currentTime = (current_t.tv_sec - start_t.tv_sec)*1000.0 + (current_t.tv_nsec - start_t.tv_nsec)/1000000.0;
    __isRecording = (currentTime < runTimeInformation.recordingTime*1000);
  }
  runTimeInformation.isCapturingAudio = 0;
  pthread_join(metronomeThread,NULL);
  __isAudioInterfaceReady = 0;
  while (__isAudioProcessing) {
    msleep(10);
  }
  free(buff);
  close_pcm(pcm);
  return NULL;
}

/**
@brief This function is the entry point for the audio processing thread.
A thread will be created by calling this function as the entry point of the corresponding
thread. It will collect the save samples from the queue and process them.
The collected samples will be be read from the queue that is shared data structure with
the audio capturing thread. Before reading from the queue, the thread needs to contact
the mutex to block other threads from accessing the queue. As soon as the reading operation
is done, the thread needs to unlock the queue by contacting the mutex again.
**/
void *audio_processing_entry_point(){
  int isQueueEmpty = FALSE;

  while(!__isAudioInterfaceReady){
    msleep(10);
  }
  float rate = __rate;
  int channels = __channels;

  runTimeInformation.rate = rate;
  runTimeInformation.channels = channels;

  CapturedDataPoints capturedDataPoints;
  initCapturedDataPoints(&capturedDataPoints);

  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  //char *musicalExpression = "";
  double currentTime = 0;
  double lastTime = currentTime;
	char *lastNote = "";
  char *currentNote = "";
	double duration = 0;
  float decibel = -60.0;
  int oldBin = 0;

  __isAudioProcessing = 1;
  while(__isRecording || !isQueueEmpty){
    pthread_mutex_lock(&mutex);
    isQueueEmpty = queue_empty(&audioDataQueue);
    pthread_mutex_unlock(&mutex);
    if (!isQueueEmpty) {
      clock_gettime(CLOCK_MONOTONIC_RAW,&single_run_start_t);
      AudioCapturePoint dataPoint;
      //initAudioCapturePoint(&dataPoint, runTimeInformation.stepSize);
      pthread_mutex_lock(&mutex);
      dataPoint = *queue_dequeue(&audioDataQueue);
      pthread_mutex_unlock(&mutex);
      currentTime = dataPoint.captureTime;
      shiftWrite(inputReal, dataPoint.arr, runTimeInformation.sampleSize, runTimeInformation.stepSize);

      //Audio Preprocessing
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_preprocessing_t);
      double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
      double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));

      applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);

      clock_gettime(CLOCK_MONOTONIC_RAW,&fft_run_start_t);
      Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      fftRunTime += (current_time_t.tv_sec - fft_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - fft_run_start_t.tv_nsec)/1000000.0;

      getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
      applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      audioPreProcessingTime += (current_time_t.tv_sec - start_audio_preprocessing_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_preprocessing_t.tv_nsec)/1000000.0;

      //Audio Transcription
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_transcription_t);
      int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);
      //double amplitude = amps[frequencyBin];
      double frequency = (double)frequencyBin * rate/runTimeInformation.sampleSize;
      currentNote = getMusicalNote(frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);

      duration = currentTime - lastTime;
      if(duration > (60.0*1000)/(runTimeInformation.beatsPerMinute*(RHYTHM_RESOLUTION/RHYTHM_DENOMINATOR))/2.0 && strcmp(currentNote, lastNote)!=0 && strcmp(currentNote, "") != 0){
        double freq = oldBin * rate/runTimeInformation.sampleSize;
        char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
        char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

        if (strlen(currentExpression)>0) {
          MusicalDataPoint dP;
          initMusicalDataPoint(&dP, freq, duration, decibel);
          insertMusicalDataPoint(&capturedDataPoints, dP);
          //musicalExpression = append(musicalExpression, currentExpression);
          //musicalExpression = append(musicalExpression, " ");
          if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
            printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
          }
          lastTime = currentTime;
          lastNote = currentNote;
        }
        //decibel = 10.0f * log10f(fmaxf(0.000001f, out[oldBins[0]] * out[oldBins[0]]));
        //lastTime = currentTime;
        oldBin = frequencyBin;
        duration = 0;
        free(musicalNote);
        free(currentExpression);
      }
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      audioTranscriptionTime += (current_time_t.tv_sec - start_audio_transcription_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_transcription_t.tv_nsec)/1000000.0;
      free(actualoutreal);
      free(actualoutimag);

      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      runTime += (current_time_t.tv_sec - single_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - single_run_start_t.tv_nsec)/1000000.0;

      runs++;
    }
    pthread_mutex_lock(&mutex);
    isQueueEmpty = queue_empty(&audioDataQueue);
    pthread_mutex_unlock(&mutex);
  }
  if(duration > 0){
    double freq = oldBin * rate/runTimeInformation.sampleSize;
    char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
    char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

    MusicalDataPoint dP;
    initMusicalDataPoint(&dP, freq, duration, decibel);
    insertMusicalDataPoint(&capturedDataPoints, dP);
    //musicalExpression = append(musicalExpression, currentExpression);
    //musicalExpression = append(musicalExpression, " ");
    if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
      printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
    }
    lastNote = currentNote;
    oldBin = 0;
    duration = 0;
    free(musicalNote);
    free(currentExpression);
  }
  clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    char *musicalExpression = (char *)calloc(32, sizeof(char));
    strcpy(musicalExpression, "");
    for(size_t i = 0; i < capturedDataPoints.pos; i++){
        char *musicalNote = getMusicalNote(capturedDataPoints.arr[i].frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);
        char *currentExpression = calculateNoteLength(musicalNote,capturedDataPoints.arr[i].duration,RHYTHM_RESOLUTION,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);
        musicalExpression = append(musicalExpression, currentExpression);
        musicalExpression = append(musicalExpression, " ");
        free(musicalNote);
        free(currentExpression);
    }
    GenerateNoteSheet(musicalExpression);
    free(musicalExpression);
  }
  if (runTimeInformation.melodyBenchmarking) {
    compareCapturedDataToOriginal(benchmarkingMelody, benchmarkingMode, benchmarkingFileName, &capturedDataPoints);
  }
  freeCapturedDataPoints(&capturedDataPoints);
  free(inputReal);
  free(inputImag);
  free(amps);
  __isAudioProcessing = 0;
  runTimeInformation.quit = 0;
  return NULL;
}

/**
@brief This function implements the parallel implementation of the Music Transcription Pipeline.
The function creates two threads that will be running in parallel. One of the threads is capturing
audio, the other one is processing the audio samples and transcribes it to a melody. Both threads use
a queue as shared data structure as well as a mutex to keep being synchronized and for the queue
not becoming inconsistent.
@param pcmDeviceName the sound card name
**/
void threadedRealTimeVersion(char *pcmDeviceName){
  pthread_t audioCaptureThread;
  pthread_t audioProcessingThread;
  pthread_mutex_init(&mutex, NULL);

  runs = 0;
  fftRunTime = 0;
  runTime = 0;
  audioCaptureTime = 0;
  audioPreProcessingTime = 0;
  audioTranscriptionTime = 0;

  initAudioDataQueue(&audioDataQueue);

  pthread_create(&audioCaptureThread,NULL,audio_capture_entry_point,pcmDeviceName);
  pthread_create(&audioProcessingThread,NULL,audio_processing_entry_point,NULL);

  pthread_join(audioCaptureThread,NULL);
  pthread_join(audioProcessingThread,NULL);
  pthread_mutex_destroy(&mutex);
  freeAudioDataQueue(&audioDataQueue);
}

/**
@brief Thread entry point for chord generator tool.
This function serves as the entry point for a thread that will only be used for
executing the chord generator python tool.
**/
void *chord_generator_tool_entry_point(void *arg){
  int *numNotes = (int *)arg;
  char *command = (char *) calloc(1024,sizeof(char));
  sprintf(command, "python3 FrequencyGenerator.py %d",*numNotes);
  retError = system(command);
  if (retError == -1) {
    fail();
  }
  free(command);
  runTimeInformation.quit = 1;
  return NULL;
}

/**
@brief This function implements a note detector for n notes.
The function identifies n main frequencies and transcribes them to musical notes.
**/
void chordDetector(char *soundCardName){
  struct pcm *pcm;
  char *pcm_name = soundCardName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  //numBins = 1;
  int *bins = (int *) calloc(numBins,sizeof(int));
  for (int i = 0; i < numBins; i++) {
    bins[i] = 0;
  }

  __isRecording = 1;

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  runTimeInformation.quit = 0;
  while(!runTimeInformation.quit){
      if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
        memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
      shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);

      double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
      double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));
      applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);
      Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);

      //audio preprocessing
      getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
      applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

      //audio processing
      getBins(bins,numBins,amps,runTimeInformation.sampleSize,rate,runTimeInformation.tuningPitch);
      //int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);

      for (int i = 0; i < numBins; i++) {
        //double amplitude = amps[bins[i]];
        double frequency = (double)bins[i] * rate/runTimeInformation.sampleSize;
        char* currentNote = getMusicalNote(frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);
        printf("%d. %f Hz (%s) - ",i+1,frequency,currentNote);
      }
      printf("%s\n", "");

      free(actualoutreal);
      free(actualoutimag);
      for (int i = 0; i < numBins; i++) {
        bins[i] = 0;
      }
      __isRecording = runTimeInformation.quit;
  }
  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief This function inserts the data for the audio spectrogram into a csv file.
The function puts all the audio data at a specific time point in a csv file. This
data will later be used by a python script to be visualized.
**/
void insertIntoCSVFile(char *csvFileName, double *out, int sampleSize, double currentTime){
  int arraySize = (int) sampleSize/2;
  FILE *fp;
  //fp=fopen(csvFileName,"a+");
  //filename=strcat(filename,".csv");
  long int size=0;
  /*Open file in Read Mode*/
  fp=fopen(csvFileName,"r");
  /*Move file point at the end of file.*/
  fseek(fp,0,SEEK_END);
  /*Get the current position of the file pointer.*/
  size=ftell(fp);
  fp=fopen(csvFileName,"a+");

  if(size!=-1){
    if(size == 0){
      double curFreq = 0;
      int lineSize = arraySize;
      char *line = (char *) calloc(lineSize, sizeof(char));
      strcpy(line, "timeStamp\\frequency");
      for (int i = 0; i < arraySize; i++) {
        curFreq = (double)i * SAMPLE_RATE/runTimeInformation.sampleSize;
        sprintf(line, "%s; %f",line,curFreq);
        if(strlen(line) > 0.9 * lineSize){
          fprintf(fp,"%s",line);
          sprintf(line,"%s","");
          //sprintf(line,"");
          //line = "";
        }
        /*if(strlen(line) > 0.9 * lineSize){
          printf("%s\n", "malloc");
          lineSize *= 2;
          line = realloc(line, lineSize * sizeof(char));
        }*/
      }
      fprintf(fp,"%s\n",line);
      free(line);
    }
  }
  else{
    printf("There is some ERROR.\n");
    exit(0);
  }
  if(out == NULL){
    return;
  }
  //double curFreq = 0;
  int lineSize = arraySize;
  char *line = (char *) calloc(lineSize, sizeof(char));
  sprintf(line, "%f",currentTime);
  for (int i = 0; i < arraySize; i++) {
    //float power = out[i] * out[i];
    //float decibel = 10.0f * log10f(fmaxf(0.000001f, power)) + 60.0;
    //sprintf(line, "%s; %f",line,decibel);
    sprintf(line, "%s; %f",line,out[i]);
    if(strlen(line) > 0.9 * lineSize){
      fprintf(fp,"%s",line);
      sprintf(line,"%s","");
      //sprintf(line,"");
      //line = "";
    }
  }
  //printf("%s\n", line);
  fprintf(fp,"%s\n",line);
  free(line);
  fclose(fp);
}

/**
@brief This function creates an audio spectrogram.
@param wavFileName wav file that contains the audio data
@param csvFileName csv file that will contain the audio spectrogram data
**/
void buildAudioSpectrogram(char *wavFileName, char *csvFileName){
  //char *csvFileName = "hello.csv";
  FILE *fp;
  fp=fopen(csvFileName,"w");
  fclose(fp);
  struct pcm *pcm;
  char *pcm_name = wavFileName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  double currentTime = 0;
  runTimeInformation.quit = !(currentTime < runTimeInformation.recordingTime);

  while(!runTimeInformation.quit) {
    if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
      memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
    shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);
    currentTime += (runTimeInformation.stepSize/rate);
    double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
    double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));
    applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);
    Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);

    //audio preprocessing
    getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
    //applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);
    insertIntoCSVFile(csvFileName, amps, runTimeInformation.sampleSize, currentTime);

    //printf("Detected %f Hz frequency (%s) with amplitude %f.\n",frequency,musicalNote,amplitude);
    free(actualoutreal);
    free(actualoutimag);
    //chunkIndex = 0;
    runTimeInformation.quit = currentTime > runTimeInformation.recordingTime;
  }
  char *startPythonScriptCommand = (char *) calloc(1024,sizeof(char));
  sprintf(startPythonScriptCommand, "python3 AudioDataAnalyzer.py %s %s", csvFileName, wavFileName);
  retError = system (startPythonScriptCommand);
  if (retError == -1) {
    fail();
  }
  free(startPythonScriptCommand);
  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief The sequential version of the Music Transcription Pipeline.
The function will record data as long as the recording time is not up. In every
recording step, the recorded data will be processed and transcribed.
**/
void sequentialVersion(char *pcmDeviceName){
  char *pcm_name = pcmDeviceName;
  struct pcm *pcm;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  CapturedDataPoints capturedDataPoints;
  initCapturedDataPoints(&capturedDataPoints);

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  struct timespec start_t, current_t;

  pthread_t metronomeThread;
  __isRecording = 1;
  //char *beatAudioFile = "../assets/MetroBar1.wav";
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    double timePerBeat = 60.0*1000.0/(double)runTimeInformation.beatsPerMinute;
    double timeForSound = getDurationOfAudioFileInSeconds(BEAT_AUDIO_FILE) * 1000.0;
    double sleepTime = timePerBeat-timeForSound;
    char command[1024];
    sprintf(command,"play -q %s",BEAT_AUDIO_FILE);
    printf("Start Audio Capture:\n");
    for(int i = 1; i <= RHYTHM_NUMERATOR;i++){
        printf("%d\n",i);
        retError = system(command);
        if (retError == -1) {
          fail();
        }
        //system("play -q MetroBar1.wav"); //pass as argument
        msleep(sleepTime);
    }
  }
  runTimeInformation.isCapturingAudio = 1;

  pthread_create(&metronomeThread,NULL,metronome_entry_point,BEAT_AUDIO_FILE);

  if (runTimeInformation.melodyBenchmarking) {
    while (!runTimeInformation.isMidiFilePlaying) {
      msleep(5);
    }
  }

  clock_gettime(CLOCK_MONOTONIC_RAW,&start_t);
	clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);

  double currentTime = (current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0;
  runTimeInformation.quit = currentTime > runTimeInformation.recordingTime*1000;

  //char *musicalExpression = "";
	double lastTime = currentTime;
	char *lastNote = "";
  char *currentNote = "";
	double duration = 0;
  float decibel = -60.0;
  int oldBin = 0;

  runs = 0;
  fftRunTime = 0;
  runTime = 0;
  audioCaptureTime = 0;
  audioPreProcessingTime = 0;
  audioTranscriptionTime = 0;

  while(!runTimeInformation.quit){
      clock_gettime(CLOCK_MONOTONIC_RAW,&single_run_start_t);
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_capture_t);
      if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
        memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
      shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      audioCaptureTime += (current_time_t.tv_sec - start_audio_capture_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_capture_t.tv_nsec)/1000000.0;

      //Audio Preprocessing
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_preprocessing_t);
      double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
      double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));

      applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);

      clock_gettime(CLOCK_MONOTONIC_RAW,&fft_run_start_t);
      Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      fftRunTime += (current_time_t.tv_sec - fft_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - fft_run_start_t.tv_nsec)/1000000.0;

      getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
      applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      audioPreProcessingTime += (current_time_t.tv_sec - start_audio_preprocessing_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_preprocessing_t.tv_nsec)/1000000.0;

      //Audio Transcription
      clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_transcription_t);
      int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);
      //double amplitude = amps[frequencyBin];
      double frequency = (double)frequencyBin * rate/runTimeInformation.sampleSize;
      currentNote = getMusicalNote(frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);

      duration = currentTime - lastTime;
      //if(duration > 0 && strcmp(currentNote, lastNote)!=0 && strcmp(currentNote, "")!=0){
      if(duration > (60.0*1000)/(runTimeInformation.beatsPerMinute*(RHYTHM_RESOLUTION/RHYTHM_DENOMINATOR)) && strcmp(currentNote, lastNote)!=0){
        double freq = oldBin * rate/runTimeInformation.sampleSize;
        char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
        char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

        if (strlen(currentExpression)>0) {
          MusicalDataPoint dP;
          initMusicalDataPoint(&dP, freq, duration, decibel);
          insertMusicalDataPoint(&capturedDataPoints, dP);
          //musicalExpression = append(musicalExpression, currentExpression);
          //musicalExpression = append(musicalExpression, " ");

          if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
            printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
          }
          lastTime = currentTime;
          lastNote = currentNote;
        }

        oldBin = frequencyBin;
        duration = 0;

        free(musicalNote);
        free(currentExpression);
      }
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      audioTranscriptionTime += (current_time_t.tv_sec - start_audio_transcription_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_transcription_t.tv_nsec)/1000000.0;
      free(actualoutreal);
      free(actualoutimag);

      clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);
  		currentTime = (current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0;
  		runTimeInformation.quit = currentTime > runTimeInformation.recordingTime*1000;
      __isRecording = runTimeInformation.quit;
      //-----
      clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
      runTime += (current_time_t.tv_sec - single_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - single_run_start_t.tv_nsec)/1000000.0;
      runs++;
  }
  runTimeInformation.isCapturingAudio = 0;
  pthread_join(metronomeThread,NULL);
  if(duration > 0){
    double freq = oldBin * rate/runTimeInformation.sampleSize;
    char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
    char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

    MusicalDataPoint dP;
    initMusicalDataPoint(&dP, freq, duration, decibel);
    insertMusicalDataPoint(&capturedDataPoints, dP);
    //musicalExpression = append(musicalExpression, currentExpression);
    //musicalExpression = append(musicalExpression, " ");
    if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
      printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
    }
    lastNote = currentNote;
    oldBin = 0;
    duration = 0;
    free(musicalNote);
    free(currentExpression);
  }
  clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    char *musicalExpression = (char *)calloc(32, sizeof(char));
    strcpy(musicalExpression,"");
    for(size_t i = 0; i < capturedDataPoints.pos; i++){
        char *musicalNote = getMusicalNote(capturedDataPoints.arr[i].frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);
        char *currentExpression = calculateNoteLength(musicalNote,capturedDataPoints.arr[i].duration,RHYTHM_RESOLUTION,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);
        musicalExpression = append(musicalExpression, currentExpression);
        musicalExpression = append(musicalExpression, " ");
        free(musicalNote);
        free(currentExpression);
    }
    GenerateNoteSheet(musicalExpression);
    free(musicalExpression);
  }
  if (runTimeInformation.melodyBenchmarking) {
    compareCapturedDataToOriginal(benchmarkingMelody, benchmarkingMode, benchmarkingFileName, &capturedDataPoints);
  }
  freeCapturedDataPoints(&capturedDataPoints);
  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief This function implements the post processing version of the Music Transcription Pipeline.
The function will process and transcribe data stored in a wav file. The process will be continued
till the end of the file is reached.
**/
void readWAVFile(char *wavFileName){
  struct pcm *pcm;
  char *pcm_name = wavFileName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  CapturedDataPoints capturedDataPoints;
  initCapturedDataPoints(&capturedDataPoints);

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  double currentTime = 0;
  runTimeInformation.quit = currentTime > runTimeInformation.recordingTime;

  //char *musicalExpression = "";
  double lastTime = currentTime;
	char *lastNote = "";
  char *currentNote = "";
	double duration = 0;
  float decibel = -60.0;
  int oldBin = 0;

  runs = 0;
  fftRunTime = 0;
  runTime = 0;
  audioCaptureTime = 0;
  audioPreProcessingTime = 0;
  audioTranscriptionTime = 0;

  while(!runTimeInformation.quit) {
    clock_gettime(CLOCK_MONOTONIC_RAW,&single_run_start_t);
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_capture_t);
    if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
      memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
    shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    audioCaptureTime += (current_time_t.tv_sec - start_audio_capture_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_capture_t.tv_nsec)/1000000.0;

    currentTime += 1000 * runTimeInformation.stepSize/rate;
    //Audio Preprocessing
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_preprocessing_t);
    double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
    double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));

    applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);

    clock_gettime(CLOCK_MONOTONIC_RAW,&fft_run_start_t);
    Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    fftRunTime += (current_time_t.tv_sec - fft_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - fft_run_start_t.tv_nsec)/1000000.0;

    getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
    applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    audioPreProcessingTime += (current_time_t.tv_sec - start_audio_preprocessing_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_preprocessing_t.tv_nsec)/1000000.0;

    //Audio Transcription
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_audio_transcription_t);
    int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);
    //double amplitude = amps[frequencyBin];
    double frequency = (double)frequencyBin * rate/runTimeInformation.sampleSize;
    currentNote = getMusicalNote(frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);

    duration = currentTime - lastTime;
    if(duration > (60.0*1000)/(runTimeInformation.beatsPerMinute*(RHYTHM_RESOLUTION/RHYTHM_DENOMINATOR)) && strcmp(currentNote, lastNote)!=0 && strcmp(currentNote, "") != 0){
      double freq = oldBin * rate/runTimeInformation.sampleSize;
      char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
      char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

      if (strlen(currentExpression)>0) {
        MusicalDataPoint dP;
        initMusicalDataPoint(&dP, freq, duration, decibel);
        insertMusicalDataPoint(&capturedDataPoints, dP);
        //musicalExpression = append(musicalExpression, currentExpression);
        //musicalExpression = append(musicalExpression, " ");

        if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
          printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
        }
        lastTime = currentTime;
        lastNote = currentNote;
      }
      oldBin = frequencyBin;
      duration = 0;
      free(musicalNote);
      free(currentExpression);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    audioTranscriptionTime += (current_time_t.tv_sec - start_audio_transcription_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - start_audio_transcription_t.tv_nsec)/1000000.0;

    free(actualoutreal);
    free(actualoutimag);

    runTimeInformation.quit = currentTime > runTimeInformation.recordingTime * 1000;
    //-----
    clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
    runTime += (current_time_t.tv_sec - single_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - single_run_start_t.tv_nsec)/1000000.0;
    runs++;
  }
  if(duration > 0){
    double freq = oldBin * rate/runTimeInformation.sampleSize;
    char *musicalNote = getMusicalNote(freq,runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents);
    char *currentExpression = calculateNoteLength(musicalNote,duration,runTimeInformation.pitchResolutionInCents,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);

    MusicalDataPoint dP;
    initMusicalDataPoint(&dP, freq, duration, decibel);
    insertMusicalDataPoint(&capturedDataPoints, dP);
    //musicalExpression = append(musicalExpression, currentExpression);
    //musicalExpression = append(musicalExpression, " ");
    if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
      printf("Current Time: %f - %f (%s) - Duration:%f\n",currentTime, freq, currentExpression, duration);
    }
    lastNote = currentNote;
    oldBin = 0;
    duration = 0;
    free(musicalNote);
    free(currentExpression);
  }
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    char *musicalExpression = (char *)calloc(32, sizeof(char));
    strcpy(musicalExpression, "");
    for(size_t i = 0; i < capturedDataPoints.pos; i++){
        char *musicalNote = getMusicalNote(capturedDataPoints.arr[i].frequency,runTimeInformation.tuningPitch,runTimeInformation.pitchResolutionInCents);
        char *currentExpression = calculateNoteLength(musicalNote,capturedDataPoints.arr[i].duration,RHYTHM_RESOLUTION,runTimeInformation.beatsPerMinute,RHYTHM_DENOMINATOR);
        musicalExpression = append(musicalExpression, currentExpression);
        musicalExpression = append(musicalExpression, " ");
        free(musicalNote);
        free(currentExpression);
    }
    GenerateNoteSheet(musicalExpression);
    free(musicalExpression);
  }
  if (runTimeInformation.melodyBenchmarking) {
    compareCapturedDataToOriginal(benchmarkingMelody, benchmarkingMode, benchmarkingFileName, &capturedDataPoints);
  }
  freeCapturedDataPoints(&capturedDataPoints);
  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief This function records audio data into wav file.
The audio data will be written into an audio file as long as the recording time
is not up.
@param soundCardName sound card name that alsa should interact with
@param wavFileName name of the wav file audio data should be stored in
**/
void writeWAVFile(char *soundCardName, char *wavFileName){
  struct pcm *pcm;
  struct pcm *wav;
  char *pcm_name = soundCardName;
  char *wav_name = wavFileName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  if (!open_pcm_write(&wav,wav_name,rate,channels,runTimeInformation.recordingTime)){
    return;
  }
  info_pcm(wav);

  pthread_t metronomeThread;
  //char *beatAudioFile = "../assets/MetroBar1.wav";
  if (!runTimeInformation.timeBenchmarking && !runTimeInformation.melodyBenchmarking) {
    double timePerBeat = 60.0*1000.0/(double)runTimeInformation.beatsPerMinute;
    double timeForSound = getDurationOfAudioFileInSeconds(BEAT_AUDIO_FILE) * 1000.0;
    double sleepTime = timePerBeat-timeForSound;
    char command[1024];
    sprintf(command,"play -q %s",BEAT_AUDIO_FILE);
    printf("Start Audio Capture:\n");
    for(int i = 1; i <= RHYTHM_NUMERATOR;i++){
        printf("%d\n",i);
        retError = system(command);
        if (retError == -1) {
          fail();
        }
        //system("play -q MetroBar1.wav"); //pass as argument
        msleep(sleepTime);
    }
  }
  runTimeInformation.isCapturingAudio = 1;
  pthread_create(&metronomeThread,NULL,metronome_entry_point,BEAT_AUDIO_FILE);


  if (runTimeInformation.melodyBenchmarking) {
    while (!runTimeInformation.isMidiFilePlaying) {
      msleep(5);
    }
  }

  struct timespec start_t, current_t;
  clock_gettime(CLOCK_MONOTONIC_RAW,&start_t);
	clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);
	//runTimeInformation.quit = !((current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0 < RECORDING_TIME*1000);

  short *buff = (short *)calloc(channels * runTimeInformation.sampleSize,sizeof(short));

  while(!runTimeInformation.quit) {
    if (!read_pcm(pcm, buff, runTimeInformation.sampleSize))
      memset(buff, 0, sizeof(short) * channels * runTimeInformation.sampleSize);

    write_pcm(wav,buff,runTimeInformation.sampleSize);

    clock_gettime(CLOCK_MONOTONIC_RAW,&current_t);
    runTimeInformation.quit = (current_t.tv_sec - start_t.tv_sec)*1000.0+ (current_t.tv_nsec - start_t.tv_nsec)/1000000.0 > runTimeInformation.recordingTime*1000;
  }
  runTimeInformation.isCapturingAudio = 0;
  pthread_join(metronomeThread,NULL);
  free(buff);
  close_pcm(pcm);
  close_pcm(wav);
  runTimeInformation.quit = 0;
}

/**
A seperate thread is opened to play a midi file, such that playing the midi file and audio capturing can be
achieved in parallel. The function uses timidity to play the midi file over the command line with the system
command.
@see https://wiki.ubuntuusers.de/TiMidity/
**/
void *playing_midi_file_entry(void *arg){
    runTimeInformation.isMidiFilePlaying = TRUE;
    char *midiFile = (char *)arg;
    char *playMidiCommand = (char *) calloc(strlen("timidity ") + strlen(midiFile) + sizeof(NULL),sizeof(char));
    strcpy(playMidiCommand,"timidity ");
    strcat(playMidiCommand,midiFile);
    while(!runTimeInformation.isCapturingAudio){
        msleep(5);
    }
    retError = system(playMidiCommand);
    if (retError == -1) {
      fail();
    }
    runTimeInformation.isMidiFilePlaying = FALSE;
    free(playMidiCommand);
    return NULL;
}

/**
@brief Thread used for melody and chord generation.
This function serves as the entry point for a thread and plays a certain amount of frequencies by passing the frequencies
to a python script.
@param arg: frequencies that are played by the python script.
**/
void *frequency_generator_entry_point(void *arg){
  double *frequency = (double *)arg;
  char *command = (char *) calloc(1024,sizeof(char));
  sprintf(command, "python3 FrequencyMaker.py %d %f",5,*frequency);
  retError = system(command);
  if (retError == -1) {
    fail();
  }
  free(command);
  runTimeInformation.quit = 1;
  return NULL;
}

/**
@brief This function performs the benchmarking for note detection.
The function iterates notewise through an octave. It starts at octave 0. When reaching
a new octave, the frequencies are doubled.
**/
void noteBenchmarking(char *soundCardName){
  struct pcm *pcm;
  char *pcm_name = soundCardName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  pthread_t frequencyGeneratorTool;
  FILE *fp;
  fp = fopen("../output/noteBenchmarking.csv","w");
  fprintf(fp, "note;octave;frequency;measuredFrequency;centDifference;isNoteDetected\n");
  fclose(fp);
  numBins = 1;
  int *bins = (int *) calloc(numBins,sizeof(int));
  double *testFrequencies = (double *) calloc(numBins, sizeof(double));
  for (int i = 0; i < numBins; i++) {
    bins[i] = 0;
  }

  __isRecording = 1;

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  int numNotesPerOctave = 12;
  char musicalNotes[][12]={
      "c",
      "cis",
      "d",
      "dis",
      "e",
      "f",
      "fis",
      "g",
      "gis",
      "a",
      "ais",
      "b"
  };
  double basicFrequencies[numNotesPerOctave];
  for (int i = 0; i < numNotesPerOctave; i++) {
    double freq = runTimeInformation.tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-4));
    basicFrequencies[i] = freq;
  }
  for (int octave = 0; octave < 9; octave++) {
    for (int halfStep = 0; halfStep < numNotesPerOctave; halfStep++) {
      int isCorrectDetected = 0;
      double measuredFrequency = 0.0;
      double testFrequency = basicFrequencies[halfStep];
      double minimumCentDifference = INFINITY;
      pthread_create(&frequencyGeneratorTool,NULL,frequency_generator_entry_point,&testFrequency);
      runTimeInformation.quit = 0;
      while(!runTimeInformation.quit){
          if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
            memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
          shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);

          double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
          double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));
          applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);
          Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);

          //audio preprocessing
          getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
          applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

          //audio processing
          getBins(bins,numBins,amps,runTimeInformation.sampleSize,rate,runTimeInformation.tuningPitch);
          //int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);

          for (int i = 0; i < numBins; i++) {
            //double amplitude = amps[bins[i]];
            double frequency = (double)bins[i] * rate/runTimeInformation.sampleSize;
            double centDifference = fabs(1200 * log(frequency/testFrequency)/log(2));
            if (centDifference < minimumCentDifference) {
              measuredFrequency = frequency;
              minimumCentDifference = centDifference;
              if (centDifference < runTimeInformation.pitchResolutionInCents) {
                isCorrectDetected = 1;
                runTimeInformation.quit = 1;
                measuredFrequency = frequency;
              }
            }
          }
          free(actualoutreal);
          free(actualoutimag);
          for (int i = 0; i < numBins; i++) {
            bins[i] = 0;
          }
          __isRecording = runTimeInformation.quit;
      }
      pthread_join(frequencyGeneratorTool,NULL);
      fp = fopen("../output/noteBenchmarking.csv","a+");
      fprintf(fp, "%s;%d;%f;%f;%f;%d\n",musicalNotes[halfStep],octave,testFrequency,measuredFrequency,minimumCentDifference,isCorrectDetected);
      fclose(fp);
      //printf("Note:%s - Octave:%d - Frequency:%fHz - Measured Frequency:%fHz - CentDifference:%fc - Note Detected:%d\n", musicalNotes[halfStep],octave,testFrequency,measuredFrequency,minimumCentDifference,isCorrectDetected);
    }
    for (int i = 0; i < numNotesPerOctave; i++) {
      basicFrequencies[i] *= 2.0;
    }
  }
  free(testFrequencies);
  free(bins);
  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief seperate structure for chord benchmarking.
To pass multiple frequencies to the thread tha calls the python script to play a chord.
**/
typedef struct chordBenchmarkingStructure{
  double *testFrequencies;
  int numNotes;
} chordBenchmarkingStructure;

/**
@brief This function calls a python script to play a chord.
The function receives a parameter that will be parsed to the chordBenchmarking structure.
**/
void *chord_generator_entry_point(void *arg){
  chordBenchmarkingStructure *myStruct = (chordBenchmarkingStructure *)arg;
  char *command = (char *) calloc(1024,sizeof(char));
  strcpy(command, "python3 FrequencyMaker.py 5");
  for (int i = 0; i < myStruct->numNotes; i++) {
    sprintf(command, "%s %f",command,myStruct->testFrequencies[i]);
  }
  //sprintf(command, "python3 FrequencyMaker.py %d %f",5,*frequency);
  retError = system(command);
  if (retError == -1) {
    fail();
  }
  free(command);
  runTimeInformation.quit = 1;
  return NULL;
}

/**
@brief Helper function to get frequency to a certain octave and half step index.
Starting from the basic frequencies of octave 0, the function calculates the frequencies
according to indices.
@param octave The octave of the note for which the frequency should be calculated.
@param halfStep The half step in the corresponding octave
**/
double getFrequencyFromIndex(int octave, int halfStep){
  int numNotesPerOctave = 12;
  double basicFrequencies[numNotesPerOctave];
  for (int i = 0; i < numNotesPerOctave; i++) {
    double freq = runTimeInformation.tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-4));
    basicFrequencies[i] = freq;
  }
  for (int i = 0; i < octave; i++) {
    for (int j = 0; j < numNotesPerOctave; j++) {
      basicFrequencies[j] *= 2.0;
    }
  }
  return basicFrequencies[halfStep];
}

/**
@brief This function implements the chord benchmarking tests.
The frequencies of the chord are created randomly. Afterwards they are played in a seperate
thread and measured by the sequential versoin of the pipeline.
**/
void chordBenchmarking(char *soundCardName){
  struct pcm *pcm;
  char *pcm_name = soundCardName;
  if (!open_pcm_read(&pcm, pcm_name))
    return;

  info_pcm(pcm);

  float rate = rate_pcm(pcm);
  int channels = channels_pcm(pcm);

  pthread_t frequencyGeneratorTool;
  FILE *fp;
  fp = fopen("../output/chordBenchmarking.csv","w");
  fprintf(fp, "noteID;note;octave;frequency;measuredFrequency;centDifference;isNoteDetected\n");
  fclose(fp);
  //numBins = 2;
  /*int *bins = (int *) calloc(numBins,sizeof(int));
  int *isCorrectDetected = (int *) calloc(numBins, sizeof(int));
  int *notes = (int *) calloc(numBins, sizeof(int));
  int *octaves = (int *) calloc(numBins, sizeof(int));
  double *testFrequencies = (double *) calloc(numBins, sizeof(double));
  double *measuredFrequencies = (double *) calloc(numBins, sizeof(double));
  double *minimumCentDifferences = (double *) calloc(numBins, sizeof(double));
  for (int i = 0; i < numBins; i++) {
    bins[i] = 0;
  }

  chordBenchmarkingStructure myStruct;
  myStruct.testFrequencies = testFrequencies;
  myStruct.numNotes = numBins;*/

  __isRecording = 1;

  short *buff = (short *)calloc(runTimeInformation.stepSize,sizeof(short));
  double *amps = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputReal = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));
  double *inputImag = (double *)calloc(runTimeInformation.sampleSize,sizeof(double));

  //int numNotesPerOctave = 12;
  char musicalNotes[][12]={
      "c",
      "cis",
      "d",
      "dis",
      "e",
      "f",
      "fis",
      "g",
      "gis",
      "a",
      "ais",
      "b"
  };
  /*double basicFrequencies[numNotesPerOctave];
  for (int i = 0; i < numNotesPerOctave; i++) {
    double freq = runTimeInformation.tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-3));
    basicFrequencies[i] = freq;
  }*/
  for (int numBins = 2; numBins <= 5; numBins++) {
    int *bins = (int *) calloc(numBins,sizeof(int));
    int *isCorrectDetected = (int *) calloc(numBins, sizeof(int));
    int *notes = (int *) calloc(numBins, sizeof(int));
    int *octaves = (int *) calloc(numBins, sizeof(int));
    double *testFrequencies = (double *) calloc(numBins, sizeof(double));
    double *measuredFrequencies = (double *) calloc(numBins, sizeof(double));
    double *minimumCentDifferences = (double *) calloc(numBins, sizeof(double));
    for (int i = 0; i < numBins; i++) {
      bins[i] = 0;
    }

    chordBenchmarkingStructure myStruct;
    myStruct.testFrequencies = testFrequencies;
    myStruct.numNotes = numBins;
    for (int try = 0; try < 15; try++) {
        int chordDetected = 0;
        for (int i = 0; i < numBins; i++) {
          testFrequencies[i] = 0;
          minimumCentDifferences[i] = INFINITY;
          measuredFrequencies[i] = 0;
          int octave = (rand() % (5-3 + 1)) + 3;
          int halfStep = rand() % 12;
          octaves[i] = octave;
          notes[i] = halfStep;
          testFrequencies[i] = getFrequencyFromIndex(octave,halfStep);
          isCorrectDetected[i] = 0;
        }
        for (int i = numBins; i > 1; --i) {
          for (int j = 0; j < i-1; ++j) {
            if (testFrequencies[j] > testFrequencies[j+1]) {
              double temp = testFrequencies[j];
              testFrequencies[j] = testFrequencies[j+1];
              testFrequencies[j+1] = temp;
              int tempOctave = octaves[j];
              octaves[j] = octaves[j+1];
              octaves[j+1] = tempOctave;
              int tempHalfStep = notes[j];
              notes[j] = notes[j+1];
              notes[j+1] = tempHalfStep;
            }
          }
        }
        pthread_create(&frequencyGeneratorTool,NULL,chord_generator_entry_point,&myStruct);
        runTimeInformation.quit = 0;
        while(!runTimeInformation.quit){
            chordDetected = 1;
            /*for (int i = 0; i < numBins; i++) {
              isCorrectDetected[i] = 0;
            }*/
            if (!read_pcm(pcm, buff, runTimeInformation.stepSize))
              memset(buff, 0, sizeof(short) * channels * runTimeInformation.stepSize);
            shiftWrite(inputReal, buff, runTimeInformation.sampleSize, runTimeInformation.stepSize);

            double *actualoutreal = copyArray(inputReal,runTimeInformation.sampleSize*sizeof(double));
            double *actualoutimag = copyArray(inputImag,runTimeInformation.sampleSize*sizeof(double));
            applyWindowingFunction(actualoutreal,runTimeInformation.sampleSize,runTimeInformation.windowingFunction);
            Fft_transform(actualoutreal,actualoutimag,runTimeInformation.sampleSize);

            //audio preprocessing
            getFrequencySpectrum(amps,actualoutreal,actualoutimag,runTimeInformation.sampleSize);
            applyFrequencyBandpass(amps,runTimeInformation.sampleSize,rate,LOW_FREQUENCY,HIGH_FREQUENCY);

            //audio processing
            getBins(bins,numBins,amps,runTimeInformation.sampleSize,rate,runTimeInformation.tuningPitch);
            //int frequencyBin = getFrequencyBin(amps,runTimeInformation.sampleSize);

            for (int i = numBins; i > 1; --i) {
              for (int j = 0; j < i-1; ++j) {
                if (bins[j] > bins[j+1]) {
                  double temp = bins[j];
                  bins[j] = bins[j+1];
                  bins[j+1] = temp;
                }
              }
            }

            for (int i = 0; i < numBins; i++) {
              //double amplitude = amps[bins[i]];a
              double minCentDiff = INFINITY;
              double bestFreq = 0;
              double frequency = 0;
              double centDifference = 0;
              for (int j = 0; j < numBins; j++) {
                frequency = (double)bins[j] * rate/runTimeInformation.sampleSize;
                centDifference = fabs(1200 * log(frequency/testFrequencies[i])/log(2));
                if (centDifference < minCentDiff) {
                  bestFreq = frequency;
                  minCentDiff = centDifference;
                }
              }

              frequency = bestFreq;
              centDifference = minCentDiff;

              if (centDifference < minimumCentDifferences[i]) {
                measuredFrequencies[i] = frequency;
                minimumCentDifferences[i] = centDifference;
              }
              /*if (strcmp(getMusicalNote(frequency, runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents), getMusicalNote(testFrequencies[i], runTimeInformation.tuningPitch, runTimeInformation.pitchResolutionInCents))==0) {
                isCorrectDetected[i] = 1;
                if (centDifference < minimumCentDifferences[i]) {
                  measuredFrequencies[i] = frequency;
                  minimumCentDifferences[i] = centDifference;
                }
              }*/
              /*if (centDifference < minimumCentDifferences[i]) {
                measuredFrequencies[i] = frequency;
                minimumCentDifferences[i] = centDifference;
                if (centDifference < runTimeInformation.pitchResolutionInCents) {
                  isCorrectDetected[i] = 1;
                  measuredFrequencies[i] = frequency;
                }
              }*/
            }

            for (int i = 0; i < numBins; i++) {
              if (minimumCentDifferences[i] < runTimeInformation.pitchResolutionInCents) {
                isCorrectDetected[i] = 1;
              }
              if (isCorrectDetected[i] == 0) {
                chordDetected = 0;
              }
            }
            if (chordDetected) {
              runTimeInformation.quit = 1;
            }
            free(actualoutreal);
            free(actualoutimag);
            for (int i = 0; i < numBins; i++) {
              bins[i] = 0;
            }
            __isRecording = runTimeInformation.quit;
        }
        pthread_join(frequencyGeneratorTool,NULL);
        fp = fopen("../output/chordBenchmarking.csv","a+");
        for (int i = 0; i < numBins; i++) {
          fprintf(fp, "%d;%s;%d;%f;%f;%f;%d\n",i+1, musicalNotes[notes[i]],octaves[i],testFrequencies[i],measuredFrequencies[i],minimumCentDifferences[i],isCorrectDetected[i]);
          //printf("%d. Note:%s - Octave:%d - Frequency:%fHz - Measured Frequency:%fHz - CentDifference:%fc - Note Detected:%d\n", i+1, musicalNotes[notes[i]],octaves[i],testFrequencies[i],measuredFrequencies[i],minimumCentDifferences[i],isCorrectDetected[i]);
        }
        fclose(fp);
    }
    free(notes);
    free(octaves);
    free(isCorrectDetected);
    free(measuredFrequencies);
    free(minimumCentDifferences);
    free(testFrequencies);
    free(bins);
  }


  free(buff);
  free(inputReal);
  free(inputImag);
  free(amps);
  close_pcm(pcm);
  runTimeInformation.quit = 0;
}

/**
@brief Helper function that creates a melody string randomly.
The function creates melodies in a certain range of octaves and of a certain amount of notes
in the melody. Instead of writing a test melody manually, a melody is created randomly for more robust tests.
The function is not able to create melodies with punctuated note lengths or uses ~.
**/
char *getRandomMelody(){
  srand ( time(NULL) );
  int numNotes = rand() % (10 + 1 - 3) + 3;
  int numNotesPerOctave = 12;
  char *musicalNotes[12]={
      "c",
      "cis",
      "d",
      "dis",
      "e",
      "f",
      "fis",
      "g",
      "gis",
      "a",
      "ais",
      "b"
  };
  int numOctaves = 3;
  char *octaves[3]={
    "\'",
    "\'\'",
    "\'\'\'"
  };
  int numNoteLengths = 4;
  /*char *noteLengths[4]={
    "1",
    "2",
    "4",
    "8"
  };*/
  char *melody = (char *)calloc(1024,sizeof(char));
  for (int i = 0; i < numNotes; i++) {
    int noteIndex = rand() % numNotesPerOctave;
    int octaveIndex = rand() % numOctaves;
    int noteLengthIndex = rand() % numNoteLengths;
    sprintf(melody, "%s %s%s%d",melody,musicalNotes[noteIndex],octaves[octaveIndex],(int)pow(2.0, (float)noteLengthIndex));
  }
  return melody;
}

/**
@brief This function implements the melody benchmarking.
There is the option to use all the different MIDI instruments for future use cases. For now,
the benchmark only uses a fairly simple instrument called "voice oohs". Melody Benchmarking is
tested for every variant of the pipeline.
**/
void melodyBenchmarking(char *soundCardName){
  pthread_t playMidiThread;
  FILE *fp;
  char *str = (char *) calloc(1024,sizeof(char));

  //FILE *instruments;
  char *instrumentStr =(char *) calloc(1024,sizeof(char));

  //konstruierte Beispiele

  char *melodyFile = (char *) calloc(1024,sizeof(char));
  char *systemCommand = (char *) calloc(1024,sizeof(char));
  char *melodyDirectoryName = "../assets/benchmarking/";
  //char *instrumentsNames = "../assets/instruments.txt";
  //instruments = fopen(instrumentsNames, "r");

  char *lilyPondFile = "../output/benchmarking.ly";
  char *midiFile = "../output/benchmarking.midi";
  char *csvFile = "../output/benchmarking.csv";
  benchmarkingFileName = csvFile;

  //sprintf(systemCommand, "cd ../output/ && lilypond --loglevel=NONE %s && timidity %s", lilyPondFile,midiFile);
  sprintf(systemCommand, "cd ../output/ && lilypond --loglevel=NONE %s", lilyPondFile);
  //sprintf(systemCommand, "cd ../output/ && lilypond --loglevel=NONE %s", lilyPondFile);

  char *rectangle = "rectangle";
  char *hann = "hann";
  char *hamming = "hamming";
  char *triangle = "triangle";
  char *blackman_harris = "blackman-harris";
  char *gauss = "gauss";
  char *kaiser = "kaiser";
  char *plank_taper = "plank-taper";
  char *tukey = "tukey";
  char *nuttall = "nuttall";
  char *flattop = "flattop";
  char *lanczos = "lanczos";

  int numWindowingFunctions = 12;
  char *windowFunctions[12]={
      rectangle,
      hann,
      hamming,
      triangle,
      gauss,
      kaiser,
      tukey,
      nuttall,
      flattop,
      lanczos,
      plank_taper,
      blackman_harris
  };

  fp = fopen(csvFile,"a+");
  fclose(fp);

  for (int var = 0; var < 5; var++) {
    char *melody = getRandomMelody();
    for (int i = 0; i < numWindowingFunctions; i++) {
      runTimeInformation.windowingFunction = windowFunctions[i];
      runTimeInformation.beatsPerMinute = 60;
      while (runTimeInformation.beatsPerMinute <= 120) {
        double testMelodyTotalDuration = getTotalDurationOfMelodyString(melody,runTimeInformation.beatsPerMinute);
        if(testMelodyTotalDuration > 0){
          benchmarkingMelody = melody;
          runTimeInformation.recordingTime = testMelodyTotalDuration/1000.0;
          instrumentStr = "voice oohs";
          runTimeInformation.instrument = instrumentStr;
          //runTimeInformation.instrument = instrumentStr;
          printf("melody: %s\n", melody);
          printf("melody duration: %f\n", testMelodyTotalDuration);
          GenerateBenchmarkingLilyPondFile(melody,lilyPondFile,runTimeInformation.beatsPerMinute,instrumentStr,runTimeInformation.title,runTimeInformation.composer);
          retError = system(systemCommand);
          if (retError == -1) {
            fail();
          }
          //pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
          //pthread_join(playMidiThread,NULL);

          //system("rm -rf ../output/benchmarking.midi ../output/benchmarking.ly");
          benchmarkingMode = "sequential";
          pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
          sequentialVersion(soundCardName);
          pthread_join(playMidiThread,NULL);
          while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
            msleep(10);
          }

          benchmarkingMode = "threaded";
          pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
          threadedRealTimeVersion(soundCardName);
          pthread_join(playMidiThread,NULL);
          while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
            msleep(10);
          }

          benchmarkingMode = "post";
          pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
          writeWAVFile(soundCardName, runTimeInformation.wavFileName);
          pthread_join(playMidiThread,NULL);
          while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
            msleep(10);
          }
          readWAVFile(runTimeInformation.wavFileName);
          //compareCapturedDataToOriginal(str,de->d_name,csvFile);

          //resetCapturedDataPoints(&capturedDataPoints);
        }
        runTimeInformation.beatsPerMinute += 30;
      }
    }
    free(melody);
  }

  //int numberOfFiles = 0;
  struct dirent *de;
  DIR *dr = opendir(melodyDirectoryName);
  if (dr == NULL)  // opendir returns NULL if couldn't open directory
  {
      printf("Could not open current directory" );
      return;
  }

  /*while (fgets(instrumentStr, 1024, instruments) != NULL && strcmp(instrumentStr,"")!=0){
    printf("%s\n", instrumentStr);
    size_t n = strlen(instrumentStr);
    if(n && instrumentStr[n-1]=='\n'){
      instrumentStr[n-1] = '\0';
    }*/
  while ((de = readdir(dr))!=NULL) {
    if(strcmp(de->d_name,".")!=0 && strcmp(de->d_name, "..")!=0){
      sprintf(melodyFile, "%s/%s", melodyDirectoryName,de->d_name);
      fp = fopen(melodyFile, "r");
      printf("file:%s\n", melodyFile);
      if (fp == NULL){
          printf("Could not open file %s",melodyFile);
          return;
      }
      while (fgets(str, 1024, fp) != NULL && strcmp(str,"")!=0){
          size_t n = strlen(str);
          if(n && str[n-1]=='\n'){
            str[n-1] = '\0';
          }
          //change tempo
          for (int i = 0; i < numWindowingFunctions; i++) {
            runTimeInformation.windowingFunction = windowFunctions[i];
            runTimeInformation.beatsPerMinute = 60;
            while (runTimeInformation.beatsPerMinute <= 120) {
              double testMelodyTotalDuration = getTotalDurationOfMelodyString(str,runTimeInformation.beatsPerMinute);
              if(testMelodyTotalDuration > 0){
                benchmarkingMelody = str;
                runTimeInformation.recordingTime = testMelodyTotalDuration/1000.0;
                instrumentStr = "voice oohs";
                runTimeInformation.instrument = instrumentStr;
                //runTimeInformation.instrument = instrumentStr;
                printf("melody: %s\n", str);
                printf("melody duration: %f\n", testMelodyTotalDuration);
                GenerateBenchmarkingLilyPondFile(str,lilyPondFile,runTimeInformation.beatsPerMinute,instrumentStr,runTimeInformation.title,runTimeInformation.composer);
                retError = system(systemCommand);
                if (retError == -1) {
                  fail();
                }
                //pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
                //pthread_join(playMidiThread,NULL);

                //system("rm -rf ../output/benchmarking.midi ../output/benchmarking.ly");
                benchmarkingMode = "sequential";
                pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
                sequentialVersion(soundCardName);
                pthread_join(playMidiThread,NULL);
                while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
                  msleep(10);
                }

                benchmarkingMode = "threaded";
                pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
                threadedRealTimeVersion(soundCardName);
                pthread_join(playMidiThread,NULL);
                while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
                  msleep(10);
                }

                benchmarkingMode = "post";
                pthread_create(&playMidiThread,NULL,playing_midi_file_entry,midiFile);
                writeWAVFile(soundCardName, runTimeInformation.wavFileName);
                pthread_join(playMidiThread,NULL);
                while (runTimeInformation.isCapturingAudio || runTimeInformation.isMidiFilePlaying) {
                  msleep(10);
                }
                readWAVFile(runTimeInformation.wavFileName);
                //compareCapturedDataToOriginal(str,de->d_name,csvFile);

                //resetCapturedDataPoints(&capturedDataPoints);
              }
              runTimeInformation.beatsPerMinute += 30;
            }
          }
      }
      fclose(fp);
      msleep(10);
    }
  }
  //}

  free(str);
  free(instrumentStr);
  free(melodyFile);
  free(systemCommand);
  closedir(dr);
  fclose(fp);
}

/**
@brief This function list all midi instruments that lilypond can use.
The instruments are listed in the text file '../assets/instruments.txt'
**/
void listInstruments(){
  char *instrumentFileLocation = "../assets/instruments.txt";
  FILE *file = fopen(instrumentFileLocation, "r");
  char line[256]; /* or other suitable maximum line size */
  int counter = 0;
  printf("%s\n\n", "Instruments:");
  while (fgets(line, sizeof line, file) != NULL) /* read a line */
  {
      line[strlen(line)-1] = '\0';
      printf("%d - %s\n", counter,line);
      counter++;
  }
  fclose(file);
}

/**
@brief This function sets the midi instrument for the pipeline.
The function sets the instrument according to line number in instruments text file (../assets/instruments.txt).
@param instrumentNumber: line number at which instrument is located in text file.
**/
void determineInstrument(int instrumentNumber){
  char *instrumentFileLocation = "../assets/instruments.txt";
  FILE *file = fopen(instrumentFileLocation, "r");
  char line[256];
  int counter = 0;
  while (fgets(line, sizeof line, file) != NULL)
  {
      line[strlen(line)-1] = '\0';
      if (counter == instrumentNumber) {
        strcpy(runTimeInformation.instrument, line);
        fclose(file);
        return;
      }
      counter++;
  }
  fclose(file);
}

/**
@brief This function initializes runtime information values.
**/
void initializeRunTimeConfiguration(){
  runTimeInformation.sampleSize = 4096;
  runTimeInformation.stepSize = 1024;
  runTimeInformation.buffSize = 128;
  //runTimeInformation.stepSize = 64;
  runTimeInformation.tuningPitch = 440.0;
  runTimeInformation.pitchResolutionInCents = 40.0;
  runTimeInformation.windowingFunction = "rectangle";
  runTimeInformation.instrument = (char *)calloc(256, sizeof(char));
  strcpy(runTimeInformation.instrument, "trumpet");
  runTimeInformation.composer = (char *)calloc(256, sizeof(char));
  strcpy(runTimeInformation.composer, "Composer");
  runTimeInformation.title = (char *)calloc(512,sizeof(char));
  strcpy(runTimeInformation.title, "Title");
  //runTimeInformation.composer = "Composer";
  //runTimeInformation.title = "Title";

  runTimeInformation.lowFrequency = 100.0;
  runTimeInformation.highFrequency = 10000.0;
  runTimeInformation.beatsPerMinute = 90;

  runTimeInformation.numBins = 1;
  runTimeInformation.path = "../output/";
  runTimeInformation.wavFileName = "../output/wavfile.wav";
  runTimeInformation.csvFileName = "../output/wavfile.csv";
  runTimeInformation.lilyPondFileName = "melody.ly";
  runTimeInformation.pdfFileName = "melody.pdf";
  runTimeInformation.midiFileName = "melody.midi";
  runTimeInformation.melodyFileName = "melody.wav";
}

/**
@brief Entry point for the program.
The function contains the control flow of the pipeline and gives the user the
possibility to choose between different execution models.
**/
int main(){
    initializeRunTimeConfiguration();
    char *wavFileName;
    char *csvFileName;
    //int __mode;

    csvFileName = CSV_FILE_NAME;
    wavFileName = WAV_FILE_NAME;
    pthread_t chordGeneratorTool;
    char userInput;

    printf("%s\n", "Welcome!");
    printf("%s\n", "Detecting System...");
    FILE *fp;
    char os_version[1035];

    fp = popen("sudo lshw -C system | grep \"product\" | cut -c14-", "r");
    if (fp == NULL) {
      printf("Failed to run command\n" );
      exit(1);
    }

    while (fgets(os_version, sizeof(os_version)-1, fp) != NULL) {
      printf("System detected: %s", os_version);
      char *raspberry_os = "Raspberry";
      runTimeInformation.raspi = 1;
      for (size_t i = 0; i < strlen(raspberry_os); i++) {
        if (os_version[i]!=raspberry_os[i]) {
          runTimeInformation.raspi = 0;
        }
      }
    }
    pclose(fp);
    if (runTimeInformation.raspi) {
      printf("%s\n", "Reminder: Raspberry is not able to generate a notesheet. That is why you need to have internet connection.");
    }
    printf("%s\n", "Detecting sound cards...");
    printf("%s\n", "Sound Cards");
    retError = system("cat /proc/asound/cards");
    if (retError == -1) {
      fail();
    }
    printf("%s", "Enter preferred sound card number: ");
    retError = scanf("%d", &runTimeInformation.soundCardNumber);
    if (retError == -1) {
      fail();
    }
    printf("%d\n", runTimeInformation.soundCardNumber);
    char *soundCardName = (char *) calloc(1024,sizeof(char));
    sprintf(soundCardName, "plughw:%d,0",runTimeInformation.soundCardNumber);
    printf("%s\n", "Options:");
    printf("\t 0 - %s\n", "Audio Spectrogram");
    printf("\t 1 - %s\n", "Record to WAV File");
    printf("\t 2 - %s\n", "Note and Chord Detection");
    printf("\t 3 - %s\n", "Melody Recognition (Real Time - Threaded Version)");
    printf("\t 4 - %s\n", "Melody Recognition (Real Time - Sequential Version)");
    printf("\t 5 - %s\n", "Melody Recognition (Post Processing)");
    printf("\t 6 - %s\n", "Note Benchmarking");
    printf("\t 7 - %s\n", "Chord Benchmarking");
    printf("\t 8 - %s\n", "Melody Benchmarking");
    printf("\t 9 - %s\n", "Performance Benchmarking");
    printf("%s", "Enter feature number: ");
    retError = scanf("%d", &runTimeInformation.mode);
    if (retError == -1) {
      fail();
    }
    int recTime = 0;
    int instrumentNumber = 0;
    //printf("%s\n", "Enter recording time:");
    //printf("%s\n", "Enter preferred beats per minute (Tempo):");
    switch (runTimeInformation.mode) {
      case 0:
        printf("%s\n", "Audio Spectrogram Mode");
        printf("%s", "Enter recording time:");
        //int recTime = 0;
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n\n", runTimeInformation.recordingTime);
        writeWAVFile(soundCardName, wavFileName);
        buildAudioSpectrogram(wavFileName,csvFileName);
        break;
      case 1:
        printf("%s\n", "Recording Mode");
        printf("%s", "Enter recording time:");
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n", runTimeInformation.recordingTime);
        writeWAVFile(soundCardName, wavFileName);
        break;
      case 2:
        printf("%s\n", "Note and Chord Detection Mode");
        runTimeInformation.sampleSize = 16384;
        runTimeInformation.stepSize = runTimeInformation.sampleSize;
        printf("%s", "Enter number of notes you want to identify:");
        retError = scanf(" %d", &numBins);
        if (retError == -1) {
          fail();
        }
        printf("%s", "Do you want to use chord generator tool?(y/n)");
        retError = scanf(" %c", &userInput);
        if (retError == -1) {
          fail();
        }
        if (userInput == 'y') {
          pthread_create(&chordGeneratorTool,NULL,chord_generator_tool_entry_point,&numBins);
        }
        //printf("Recording Time: %ds\n", runTimeInformation.recordingTime);
        chordDetector(soundCardName);
        pthread_join(chordGeneratorTool,NULL);
        break;
      case 3:
        printf("%s\n", "Melody Recognition (Real Time - Threaded Version) Mode");
        printf("%s", "Enter title of song:");
        listInstruments();
        printf("%s", "Enter preferred instrument:");
        retError = scanf("%d", &instrumentNumber);
        if (retError == -1) {
          fail();
        }
        determineInstrument(instrumentNumber);
        printf("Instrument '%s' was chosen!\n\n", runTimeInformation.instrument);
        printf("%s", "Enter preferred tempo (beats per minute):");
        retError = scanf("%d", &runTimeInformation.beatsPerMinute);
        if (retError == -1) {
          fail();
        }
        printf("Tempo: %d bpm\n\n", runTimeInformation.beatsPerMinute);
        printf("%s", "Enter recording time:");
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n\n", runTimeInformation.recordingTime);
        printf("%s", "Do you want to use metronome?(y/n)");
        retError = scanf(" %c", &userInput);
        if (retError == -1) {
          fail();
        }
        if (userInput == 'y') {
          runTimeInformation.headPhones = 1;
          printf("%s\n\n", "Remember to use headphones.");
        }
        threadedRealTimeVersion(soundCardName);
        break;
      case 4:
        printf("%s\n", "Melody Recognition (Real Time - Sequential Version) Mode");
        listInstruments();
        printf("%s", "Enter preferred instrument:");
        retError = scanf("%d", &instrumentNumber);
        if (retError == -1) {
          fail();
        }
        determineInstrument(instrumentNumber);
        printf("Instrument '%s' was chosen!\n\n", runTimeInformation.instrument);
        printf("%s", "Enter preferred tempo (beats per minute):");
        retError = scanf("%d", &runTimeInformation.beatsPerMinute);
        if (retError == -1) {
          fail();
        }
        printf("Tempo: %d bpm\n\n", runTimeInformation.beatsPerMinute);
        printf("%s", "Enter recording time:");
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n\n", runTimeInformation.recordingTime);
        printf("%s", "Do you want to use metronome?(y/n)");
        retError = scanf(" %c", &userInput);
        if (retError == -1) {
          fail();
        }
        if (userInput == 'y') {
          runTimeInformation.headPhones = 1;
          printf("%s\n\n", "Remember to use headphones.");
        }
        sequentialVersion(soundCardName);
        break;
      case 5:
        printf("%s\n", "Melody Recognition (Post Processing)");
        listInstruments();
        printf("%s", "Enter preferred instrument:");
        retError = scanf("%d", &instrumentNumber);
        if (retError == -1) {
          fail();
        }
        determineInstrument(instrumentNumber);
        printf("Instrument '%s' was chosen!\n\n", runTimeInformation.instrument);
        printf("%s", "Enter preferred tempo (beats per minute):");
        retError = scanf("%d", &runTimeInformation.beatsPerMinute);
        if (retError == -1) {
          fail();
        }
        printf("Tempo: %d bpm\n\n", runTimeInformation.beatsPerMinute);
        printf("%s", "Enter recording time:");
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n\n", runTimeInformation.recordingTime);
        printf("%s", "Do you want to use metronome?(y/n)");
        retError = scanf(" %c", &userInput);
        if (retError == -1) {
          fail();
        }
        if (userInput == 'y') {
          runTimeInformation.headPhones = 1;
          printf("%s\n\n", "Remember to use headphones.");
        }
        writeWAVFile(soundCardName, wavFileName);
        readWAVFile(wavFileName);
        break;
      case 6:
        printf("%s\n", "Note Benchmarking Mode");
        runTimeInformation.sampleSize = 8192;
        runTimeInformation.stepSize = runTimeInformation.sampleSize;
        runTimeInformation.windowingFunction = "gauss";
        srand ( time(NULL) );
        noteBenchmarking(soundCardName);
        break;
      case 7:
        printf("%s\n", "Chord Benchmarking Mode");
        runTimeInformation.sampleSize = 8192;
        runTimeInformation.stepSize = runTimeInformation.sampleSize;
        runTimeInformation.windowingFunction = "gauss";
        srand ( time(NULL) );
        chordBenchmarking(soundCardName);
        break;
      case 8:
        printf("%s\n", "Melody Benchmarking Mode");
        runTimeInformation.melodyBenchmarking = 1;
        melodyBenchmarking(soundCardName);
        break;
      case 9:
        runTimeInformation.timeBenchmarking = 1;
        runTimeInformation.headPhones = 0;
        printf("%s\n", "Performance Benchmarking Mode");
        printf("%s", "Enter recording time:");
        retError = scanf("%d", &recTime);
        if (retError == -1) {
          fail();
        }
        runTimeInformation.recordingTime = (float)recTime;
        printf("Recording Time: %fs\n", runTimeInformation.recordingTime);

        char *fileName = (char *)calloc(100, sizeof(char));
        sprintf(fileName, "../output/%s.csv","timeBenchmarking");
        FILE *temp_fp;
        temp_fp = fopen(fileName, "w");

        fprintf(temp_fp, "version;stepSize;sampleSize;duration;iterations;singleRunTime;audioCaptureTime;fftTime;audioPreProcessingTime;audioTranscriptionTime\n");
        runTimeInformation.sampleSize = 128;
        while (runTimeInformation.sampleSize <= 8192) {
          runTimeInformation.stepSize = 16;
          runTimeInformation.buffSize = runTimeInformation.stepSize;
          while (runTimeInformation.stepSize <= runTimeInformation.sampleSize) {
            printf("%d - %d\n", runTimeInformation.stepSize, runTimeInformation.sampleSize);
            printf("Recording Time: %fs\n", runTimeInformation.recordingTime);
            printf("%s\n\n", "############################################");

            printf("%s\n", "Sequential Version:");
            clock_gettime(CLOCK_MONOTONIC_RAW,&whole_run_start_t);
            sequentialVersion(soundCardName);
            clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
            fprintf(temp_fp, "%s;%d;%d;%f;%d;%f;%f;%f;%f;%f\n","sequential",runTimeInformation.stepSize,runTimeInformation.sampleSize,(current_time_t.tv_sec - whole_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - whole_run_start_t.tv_nsec)/1000000.0,runs,runTime/runs,audioCaptureTime/runs,fftRunTime/runs,audioPreProcessingTime/runs,audioTranscriptionTime/runs);
            printf("%s\n\n", "############################################");

            printf("%s\n", "Threaded Version:");
            clock_gettime(CLOCK_MONOTONIC_RAW,&whole_run_start_t);
            threadedRealTimeVersion(soundCardName);
            clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
            fprintf(temp_fp, "%s;%d;%d;%f;%d;%f;%f;%f;%f;%f\n","threaded",runTimeInformation.stepSize,runTimeInformation.sampleSize,(current_time_t.tv_sec - whole_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - whole_run_start_t.tv_nsec)/1000000.0,runs,runTime/runs,audioCaptureTime/runs,fftRunTime/runs,audioPreProcessingTime/runs,audioTranscriptionTime/runs);
            printf("%s\n\n", "############################################");

            printf("%s\n", "Post Processing Version:");
            clock_gettime(CLOCK_MONOTONIC_RAW,&whole_run_start_t);
            writeWAVFile(soundCardName, wavFileName);
            readWAVFile(wavFileName);
            clock_gettime(CLOCK_MONOTONIC_RAW,&current_time_t);
            fprintf(temp_fp, "%s;%d;%d;%f;%d;%f;%f;%f;%f;%f\n","post",runTimeInformation.stepSize,runTimeInformation.sampleSize,(current_time_t.tv_sec - whole_run_start_t.tv_sec)*1000.0+ (current_time_t.tv_nsec - whole_run_start_t.tv_nsec)/1000000.0,runs,runTime/runs,audioCaptureTime/runs,fftRunTime/runs,audioPreProcessingTime/runs,audioTranscriptionTime/runs);
            printf("%s\n\n", "############################################");
            runTimeInformation.stepSize *= 2;
          }
          runTimeInformation.sampleSize *= 2;
        }
        fclose(temp_fp);
        runTimeInformation.timeBenchmarking = 0;
        break;
      default:
        printf("%s\n", "Mode does not exist!");
        break;
    }
    printf("%s\n", "Application closing.");
    free(runTimeInformation.title);
    free(runTimeInformation.composer);
    free(runTimeInformation.instrument);
    free(soundCardName);
    return 0;
}
