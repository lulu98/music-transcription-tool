/**
@file AudioTranscription.c
@author Lukas Graber
@date 30 May 2019
@brief Implementation of the functions to implement audio transcription.
**/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "../include/ApplicationMacros.h"
#include "../include/AudioTranscription.h"
#include "../include/HelperFunctions.h"
#include "../include/Structures.h"

/**
The function starts out from with the frequencies of the lowest octave. The frequencies of
the corresponding notes one octave higher are related with the factor 2. Afterwards the musical
note within the octave is searched and together with the octave, the actual frequency of the note
can be calculated.
**/
double getFrequency(char *musicalNote, int octave, double tuningPitch){
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
    double freq = tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-3));
    basicFrequencies[i] = freq;
  }
  int positionInOctave = 0;
  for (int i = 0; i < numNotesPerOctave; i++) {
    if (strcmp(musicalNote, musicalNotes[i])==0) {
      positionInOctave = i;
    }
  }
  if (positionInOctave == numNotesPerOctave) {
    printf("Musical Note does not exist in octave.");
    exit(0);
  }
  double frequency = basicFrequencies[positionInOctave];
  frequency = frequency * pow(2.0,(double)(octave-1));
  return frequency;
  //find frequency to musical note and octave
}

/**
The function checks for every possible starting position whether the following characters in sub match
with the characters in s. If so, then sub is a substring of s.
**/
int isSubstring(char sub[], char s[]) {
   //int c = 0;
   int length = strlen(s);
   int subStringLength = strlen(sub);
   if(length <= 0 || subStringLength <= 0 || subStringLength > length){
        return FALSE;
   }

   int subStringFound = TRUE;
   for(int startPos = 0; startPos <= length - subStringLength;startPos++){
        for(int offset = 0; offset < subStringLength; offset++){
            subStringFound = subStringFound && (s[startPos+offset]==sub[offset]);
        }
        if(subStringFound){
            return TRUE;
        } else{
            subStringFound=TRUE;
        }
    }
    return FALSE;
}

/**
The function generates a lilypond file with the melody string passed to it. There are boilerplate headers and footers used to create
a valid lilypond file that can later be used to generate a pdf notesheet and a midi file.
@see http://lilypond.org/doc/v2.19/Documentation/notation/file-structure
**/
void GenerateLilyPondFile(char *melodyStringRepresentation,char *path){
    char *header = (char *)calloc(1024,sizeof(char));
    sprintf(header,"\\version \"2.18.2\" \\header { title=\"%s\" subtitle=\"%s\" composer=\"%s\" } \\score { \\new Staff \\with { instrumentName = \"%s\" } { \\set Staff.midiInstrument = #\"%s\" \\absolute { \\clef %s \\key %s \\%s \\time %d/%d \\tempo \"%s\" %d = %d ", TITLE, SUBTITLE, COMPOSER, INSTRUMENT,INSTRUMENT,CLEF,KEY,SCALE_TYPE,RHYTHM_NUMERATOR,RHYTHM_DENOMINATOR,TEMPO_DESCRIPTION,RHYTHM_DENOMINATOR,BEATS_PER_MINUTE);
    char *tail ="} } \\layout {} \\midi {} }";

    char *lilyPondStringRepresentation = (char *)calloc((strlen(header) + strlen(melodyStringRepresentation) + strlen(tail)+1),sizeof(char));

    if(lilyPondStringRepresentation == NULL){
        printf("not able to allocate memory.\n");
        return;
    }

    strcpy(lilyPondStringRepresentation,header);
    strcat(lilyPondStringRepresentation,melodyStringRepresentation);
    strcat(lilyPondStringRepresentation,tail);

    FILE *file = fopen(path, "w");
    int results = fputs(lilyPondStringRepresentation, file);
    if (results == EOF) {
        printf("Failed to write to Lilypond file.\n");
    }
    fclose(file);

    free(lilyPondStringRepresentation);
    printf("Lilypond File created.\n");
}

/**
The only difference to the GenerateLilyPondFile is that the calling function can specify
the tempo as well as the instrument name.
**/
void GenerateBenchmarkingLilyPondFile(char *melodyStringRepresentation,char *path, int bpm, char *instrument, char *title, char *composer){
    char *header = (char *)calloc(1024, sizeof(char));
    sprintf(header,"\\version \"2.18.2\" \\header { title=\"%s\" subtitle=\"%s\" composer=\"%s\" } \\score { \\new Staff \\with { instrumentName = \"%s\" } { \\set Staff.midiInstrument = #\"%s\" \\absolute { \\clef %s \\key %s \\%s \\time %d/%d \\tempo \"%s\" %d = %d ", title, SUBTITLE, composer, instrument,instrument,CLEF,KEY,SCALE_TYPE,RHYTHM_NUMERATOR,RHYTHM_DENOMINATOR,TEMPO_DESCRIPTION,RHYTHM_DENOMINATOR,bpm);
    char *tail ="} } \\layout {} \\midi {} }";

    //char *lilyPondStringRepresentation = malloc(strlen(header) + strlen(melodyStringRepresentation) + strlen(tail));
    char *lilyPondStringRepresentation = (char *)calloc((strlen(header) + strlen(melodyStringRepresentation) + strlen(tail) + 1),sizeof(char));

    if(lilyPondStringRepresentation == NULL){
        printf("not able to allocate memory.\n");
        return;
    }

    strcpy(lilyPondStringRepresentation,header);
    strcat(lilyPondStringRepresentation,melodyStringRepresentation);
    strcat(lilyPondStringRepresentation,tail);

    FILE *file = fopen(path, "w+");
    int results = fputs(lilyPondStringRepresentation, file);
    if (results == EOF) {
        printf("Failed to write to Lilypond file.\n");
    }
    fclose(file);
    free(lilyPondStringRepresentation);
    printf("Lilypond File created.\n");
}

/**
The function checks whether the length of the note is bigger than a 32th part of a whole measure. Furthermore, it
considers punctuated notation.
**/
double getNoteLength(char *musicalExpression,int beatsPerMinute){
    int isEmptyString = TRUE;
    for(int i = 0; (unsigned)i < strlen(musicalExpression);i++){
      if(musicalExpression[i] != ' '){
        isEmptyString = FALSE;
      }
    }
    if(isEmptyString){
      return 0;
    }
    int ind = 0; //describes the position at which the first dot is to be found
    if(isSubstring(".",musicalExpression)){
        while((unsigned)ind < strlen(musicalExpression) && musicalExpression[ind] != '.'){
            ind++;
        }
    }
    int temp = strtol(musicalExpression,NULL,10);
    double noteLength = 0;
    double amountBeats = 0;
    double timePerBeat = 60.0 * 1000.0 / (double)beatsPerMinute;
    if(temp == 1 || temp == 2 || temp == 4 || temp == 8 || temp == 16 || temp == 32){
        amountBeats = (double)RHYTHM_DENOMINATOR/(double)temp;
        noteLength = noteLength + amountBeats * timePerBeat;
    }else{
        printf("Note length is too small: %d!\n",temp);
        exit(0);
    }
    int i = 0;
    while((unsigned)i < (strlen(musicalExpression)-ind) && musicalExpression[ind + i] == '.'){
        noteLength = noteLength + (amountBeats * timePerBeat) / powl(2.0,(double)(i+1));
        i++;
    }
    /*if(musicalExpression[ind] == '.'){
        noteLength = noteLength + (amountBeats * timePerBeat) / powl(2.0,(double)(strlen(musicalExpression)-ind));
    }*/

    return noteLength;
}

/**
This function is used to adjust the real note duration towards a more manageable length for processing. It calculates the
next higher and lower duration and returns the one of lower distance from the actualNoteDuration.
**/
double getNearestNoteDuration(double actualNoteDuration,int resolution, int beatsPerMinute,int rhythmDenominator){
    double timePerBeat = 60.0 * 1000.0 / (double)beatsPerMinute; //time for one beat
    double resolutionTime = timePerBeat / (resolution/rhythmDenominator); //sechzehntel auflösung
    double nextLowerNoteDuration = (double)((int)(actualNoteDuration/resolutionTime)) * resolutionTime;
    double nextUpperNoteDuration = (double)((int)(actualNoteDuration/resolutionTime) + 1) * resolutionTime;
    //printf("lower:%f\n",nextLowerNoteDuration);
    //printf("upper:%f\n",nextUpperNoteDuration);
    double middle = nextLowerNoteDuration + (nextUpperNoteDuration-nextLowerNoteDuration)/2;
    if(actualNoteDuration < middle){
        return nextLowerNoteDuration;
    }else{
        return nextUpperNoteDuration;
    }
}

/**
The function uses the beatsPerMinute to identify the basic measure for the current song. It then calculates how many of these
basic measure are represented by the musicalNote through the duration of the note. For the integer part of the calculated amount of beats,
the function will just draw as many basic measures and sticking them together with ~ character. For the rest of the not integer number
of beats, the function will then append as many smaller measures till the number of beats is small enough to be discarded.
**/
char *calculateNoteLength(char *musicalNote, double duration, int resolution, int beatsPerMinute, int rhythmDenominator){
    char *musicalExpression = (char *)calloc(1024,sizeof(char));
    if(strcmp(musicalNote, "")==0){
        strcpy(musicalExpression, "");
        return musicalExpression;
    }
    int currentStringSize = 0;
    double timePerBeat = 60.0 * 1000.0 / (double)beatsPerMinute; //time for one beat
    duration = getNearestNoteDuration(duration,resolution,beatsPerMinute,rhythmDenominator);
    double beats = duration / timePerBeat; //amount of beats
    //int beatsPerMeasure = RHYTHM_NUMERATOR;

    char *thirtytwoMeasure = (char *) calloc((strlen(musicalNote) + strlen("32") + 1),sizeof(char));
    sprintf(thirtytwoMeasure,"%s%d",musicalNote,32);
    char *sixteenthMeasure = (char *) calloc((strlen(musicalNote) + strlen("16") + 1),sizeof(char));
    sprintf(sixteenthMeasure,"%s%d",musicalNote,16);
    char *eighthMeasure = (char *)calloc((strlen(musicalNote) + strlen("8") + 1),sizeof(char));
    sprintf(eighthMeasure,"%s%d",musicalNote,8);
    char *quarterMeasure = (char *)calloc((strlen(musicalNote) + strlen("4") + 1),sizeof(char));
    sprintf(quarterMeasure,"%s%d",musicalNote,4);
    char *halfMeasure = (char *)calloc((strlen(musicalNote) + strlen("2") + 1),sizeof(char));
    sprintf(halfMeasure,"%s%d",musicalNote,2);
    char *fullMeasure = (char *)calloc((strlen(musicalNote) + strlen("1") + 1),sizeof(char));
    sprintf(fullMeasure,"%s%d",musicalNote,1);

    char *baseMeasure;
    if(RHYTHM_DENOMINATOR == 2){
        baseMeasure = halfMeasure;
    }else if(RHYTHM_DENOMINATOR == 4){
        baseMeasure = quarterMeasure;
    }else if(RHYTHM_DENOMINATOR == 8){
        baseMeasure = eighthMeasure;
    }
    int integerAmount = (int)floor(beats);
    double additionalAmount = beats - integerAmount;                //fractions of a beat

    for(int i = 0; i < integerAmount; i++){
        if(currentStringSize == 0){
            strcpy(musicalExpression, baseMeasure);
            //musicalExpression = baseMeasure;
            currentStringSize = strlen(musicalExpression);
        }else{
            currentStringSize = strlen(musicalExpression) + strlen("~ ") + strlen(baseMeasure) + 1;
            /*char *temp = (char *) calloc(currentStringSize,sizeof(char));
            strcpy(temp,musicalExpression);
            if (strcmp(musicalNote, "r")!=0) {
              strcat(temp, "~");
            }
            strcat(temp," ");
            strcat(temp,baseMeasure);
            musicalExpression = temp;*/
            if (strcmp(musicalNote, "r")!=0) {
              musicalExpression = append(musicalExpression, "~");
            }
            musicalExpression = append(musicalExpression, " ");
            musicalExpression = append(musicalExpression, baseMeasure);
        }
    }
    int counter = 0;
    double amountOfBeat = additionalAmount;
    if(integerAmount > 0){
        counter = 1;
    }
    while(amountOfBeat >= 0.125){
        if(counter != 0){
            currentStringSize = strlen(musicalExpression) + strlen("~ ") + 1;
            //char *temp = (char *) calloc(currentStringSize,sizeof(char));
            //strcpy(temp,musicalExpression);
            if (strcmp(musicalNote, "r")!=0) {
              musicalExpression = append(musicalExpression, "~");
              //strcat(temp, "~");
            }
            musicalExpression = append(musicalExpression, " ");
            //strcat(temp," ");
            //musicalExpression = temp;
            counter = 0;
        }
        if(amountOfBeat >= 0.5){
            currentStringSize = strlen(musicalExpression) + strlen(eighthMeasure) + 1;
            //char *temp = (char *)calloc(currentStringSize,sizeof(char));
            //strcpy(temp,musicalExpression);
            //strcat(temp,eighthMeasure);
            //musicalExpression = temp;
            musicalExpression = append(musicalExpression, eighthMeasure);
            amountOfBeat = amountOfBeat - 0.5;
            counter = 1;
        }else if(amountOfBeat >= 0.25){
            /*currentStringSize = strlen(musicalExpression) + strlen(sixteenthMeasure) + 1;
            char *temp =(char *) calloc(currentStringSize,sizeof(char));
            strcpy(temp,musicalExpression);
            strcat(temp,sixteenthMeasure);
            musicalExpression = temp;*/
            musicalExpression = append(musicalExpression, sixteenthMeasure);
            amountOfBeat = amountOfBeat- 0.25;
            counter = 1;
        }else if(amountOfBeat >= 0.125){
            /*currentStringSize = strlen(musicalExpression) + strlen(thirtytwoMeasure) + 1;
            char *temp = (char *)calloc(currentStringSize,sizeof(char));
            strcpy(temp,musicalExpression);
            strcat(temp,thirtytwoMeasure);
            musicalExpression = temp;*/
            musicalExpression = append(musicalExpression, thirtytwoMeasure);
            //musicalExpression =(char *)realloc(musicalExpression,currentStringSize);
            //strcat(musicalExpression,thirtytwoMeasure);
            amountOfBeat = amountOfBeat- 0.125;
            counter = 1;
        }else{
            counter = 0;
        }
    }
    free(thirtytwoMeasure);
    free(sixteenthMeasure);
    free(eighthMeasure);
    free(quarterMeasure);
    free(halfMeasure);
    free(fullMeasure);
    return musicalExpression;
}

/**
The function parses a melody string and generates an array containing the note length of each identified note.
The array will be looped over and the note lengths will be summed up. The calculated note length will be returned.
**/
double getTotalDurationOfMelodyString(char *testMelody, int beatsPerMinute){
    int base = 0;
    int offset = 0;
    int length = 1024;
    //double frequencies[length];
    double noteLength[length];
    int ind = 0;
    //char *melodyExpression = malloc(1024);
    //char *noteLengthExpression = malloc(1024);
    while((unsigned)(base + offset) < strlen(testMelody)){
        while((unsigned)(base + offset) < strlen(testMelody) && testMelody[base+offset] != ' '){
            offset++;
        }
        int temp = 0;
        //char *melodyExpression = (char *) calloc(1024,sizeof(char));
        char *noteLengthExpression = (char *) calloc(1024,sizeof(char));
        while(temp < offset && (testMelody[base+temp] != '1' && testMelody[base+temp] != '2' && testMelody[base+temp] != '3' && testMelody[base+temp] != '4' && testMelody[base+temp] != '8')){
            //melodyExpression[temp] = testMelody[base+temp];
            temp++;
        }
        int k = 0;
        while(temp < offset){
            noteLengthExpression[k] = testMelody[base + temp];
            temp++;
            k++;
        }
        //frequencies[ind] = getFrequencyToMusicalNote(melodyExpression,tuningPitch);
        noteLength[ind] = getNoteLength(noteLengthExpression,beatsPerMinute);
        ind++;
        base = base + offset;
        offset = 0;
        base++;
        free(noteLengthExpression);
        //strcpy(melodyExpression, "");
        //strcpy(noteLengthExpression, "");
    }
    double totalLength = 0;
    for(int i = 0; i < ind;i++){
        totalLength = totalLength + noteLength[i];
    }
    return totalLength;
}

/**
The function uses the sox command to get the duration of the whole audio file.
@see https://wiki.ubuntuusers.de/SoX/
**/
double getDurationOfAudioFileInSeconds(char *audioFile){
    FILE *tempFile;
    char path[1035];
    char *command = (char *) calloc(1024,sizeof(char));
    sprintf(command,"sox --info -D %s",audioFile);
    tempFile = popen(command, "r");
    if(tempFile == NULL) {
        printf("cleaning up...\n");
        exit(0);
    }
    double timeForSound = 0;
    while(fgets(path, sizeof(path)-1, tempFile) != NULL){
        timeForSound = strtod(path,NULL);
    }
    free(command);
    return timeForSound;
}

/*char *getMusicalNoteFromFrequency(double currentFrequency, double tuningPitch){
  double lowerFrequencyLimit = 100.0;
  double upperFrequencyLimit = 5000.0;
  if (currentFrequency < lowerFrequencyLimit || currentFrequency > upperFrequencyLimit) {
    return "r";
  }
  int numNotesPerOctave = 12;
  int numOctaves = 6;
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
  char pitches[][6] = {
      ",,",
      ",",
      "",
      "'",
      "''",
      "'''"
  };
  double basicFrequencies[numNotesPerOctave];
  for (int i = 0; i < numNotesPerOctave; i++) {
    double freq = tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-3));
    basicFrequencies[i] = freq;
  }
  int i = 0;
  int j = 0;
  double freq = INFINITY;
  double lastFrequency = 0;
  while (i < numOctaves && j < numNotesPerOctave && currentFrequency > freq) {
    if (j % numNotesPerOctave == 0) {
      j = 0;
      i++;
    }
    lastFrequency = freq;
    freq = basicFrequencies[j] * pow(2.0,i);
    j++;
  }
  double middle = (freq - lastFrequency)/2 + lastFrequency;
  if (currentFrequency <= middle) {
    if (j == 0) {
      j = numNotesPerOctave - 1;
      i--;
    }else{
      j--;
    }
  }
  char *musicalNote = calloc(strlen(musicalNotes[j]) + strlen(pitches[i]) + 1, sizeof(char));
  strcpy(musicalNote, musicalNotes[j]);
  strcat(musicalNotes, pitches[i]);
  return musicalNote;
}*/

/**
This function calculates index of bin with maximal power level.
**/
void getMaximalBin(int *bins, int numBins, float *out, int N){
  if (numBins <= 0) {
    exit(0);
  }
  int maxIndex = 0;
  float max_power = 0;
  for (size_t i = 0; i < (size_t) N; i++) {
    float power = out[i] * out[i];
    maxIndex = max_power < power ? i : (size_t) maxIndex;
    max_power = max_power < power ? power : max_power;
  }
  bins[0] = maxIndex;
}

/**
Basically iterates through the coefficient array out and whenever a bin is found with a
higher power level than one of the bins saved in bins, the array bins is updated. In the
end, the array bins will have the indexes of the bins with with numBins maximal power levels.
**/
void getBins(int *bins, int numBins, double *out, int N, double Fs, double tuningPitch){
  float power = 0;
  int numNotesPerOctave = 12;
  double basicFrequencies[numNotesPerOctave];
  for (int i = 0; i < numNotesPerOctave; i++) {
    double freq = tuningPitch * pow(pow(2.0,1.0/12.0),i-9) * pow(2.0,(double)(0-3));
    basicFrequencies[i] = freq;
  }
  int numOctaves = 6;
  for (int i = 0; i < numOctaves; i++) {
    for (int j = 0; j < numNotesPerOctave; j++) {
        double freq = basicFrequencies[j] * pow(2.0,i);
        int currentBin = (int)floor((freq * N)/(double)Fs);
        power = out[currentBin];
        //power = out[currentBin] * out[currentBin];
        //power = 10.0f * log10f(fmaxf(0.000001f, power));
        //printf("%f\n", power);

        int minBinIndex = 0;
        for (int k = 0; k < numBins; k++) {
          if (out[bins[k]] < out[bins[minBinIndex]]) {
            minBinIndex = k;
          }
        }
        bins[minBinIndex] = out[bins[minBinIndex]] < power ? currentBin : bins[minBinIndex];
    }
  }
}

/*void getBins(int *bins, int numBins, float *out, int N){
  float power = 0;
  for (size_t i = 0; i < N; i++) {
    //finde minimalen Index
    int minBinIndex = 0;
    for (size_t j = 0; j < numBins; j++) {
      if (out[bins[j]] < out[bins[minBinIndex]]) {
        minBinIndex = j;
      }
    }
    power = out[i] * out[i];
    bins[minBinIndex] = out[bins[minBinIndex]] < power ? i : bins[minBinIndex];
  }
}*/

/**
The function calculates the musical note in lilypond format from a frequency. It looks through all the possible frequencies
below and above the tuning pitch. If the passed frequency lies in an acceptible range from the calculated frequency, then a note is
found and an index will be calculated. This index can then be used to reference the musical note in the musicalNotes array and to
calculate the footer of the string to identify its octave.
@see https://pages.mtu.edu/~suits/NoteFreqCalcs.html
**/
char *getMusicalNote(double frequency, double tuningPitch, double pitchResolution){
    double lowerFrequencyLimit = 100.0;
    double upperFrequencyLimit = 5000.0;
    char *musicalNote = (char *) calloc(32,sizeof(char));
    if (frequency < lowerFrequencyLimit || frequency > upperFrequencyLimit) {
      strcpy(musicalNote, "");
      return musicalNote;
    }//array from a to next a
    char musicalNotes[][13]={
        "a",
        "ais",
        "b",
        "c",
        "cis",
        "d",
        "dis",
        "e",
        "f",
        "fis",
        "g",
        "gis",
        "a"
    };
    //int frequencyFound = 0;
    //fall für runter
    if(frequency <= tuningPitch){
        for (int octave = 0;octave < 5;octave++){
            for(int note = 0; note < 12;note++){
                double currentFrequency = tuningPitch * powl(powl(2,1./12),(octave * 12 + note) * (-1));
                double centDifference = 1200 * log(frequency/currentFrequency)/log(2);
                if (centDifference > (-1) * pitchResolution && centDifference <= pitchResolution){
                    //frequencyFound = 1;
                    strcpy(musicalNote,musicalNotes[sizeof(musicalNotes)/sizeof(musicalNotes[0]) - note - 1]);
                    if(octave == 0){
                        if(note <= 9){
                        //zeichne octave viele Kommas
                            strcat(musicalNote,"'");
                        }
                    } else{
                        if(note <= 9){
                            //zeichne octave viele Kommas
                            for(int i = 0; i < octave-1; i++){
                                strcat(musicalNote,",");
                            }
                        } else{
                            //zeichne octave +1 viele Kommas
                            for(int i = 0; i < octave;i++){
                                strcat(musicalNote,",");
                            }
                        }
                    }
                    return musicalNote;
                }
            }
        }
    }else {
        for (int octave = 0;octave < 5;octave++){
            for(int note = 0; note < 12;note++){
                double currentFrequency = tuningPitch * powl(powl(2,1./12),octave * 12 + note);
                double centDifference = 1200 * log(frequency/currentFrequency)/log(2);
                if (centDifference > (-1) * pitchResolution && centDifference <= pitchResolution){
                    //frequencyFound = 1;
                    //char *musicalNote = (char *) calloc(32,sizeof(char));
                    strcpy(musicalNote,musicalNotes[note]);
                    if(note <= 2){
                        //zeichne octave viele '
                        for(int i = 0; i < octave+1; i++){
                            strcat(musicalNote,"'");
                        }
                    } else{
                        //zeichne octave +1 viele '
                        for(int i = 0;i < octave+2; i++){
                            strcat(musicalNote,"'");
                        }
                    }
                    return musicalNote;
                }
            }
        }
    }
    strcpy(musicalNote, "");
    return musicalNote;
}

/**
The function calculates a frequency from a musicalExpression. It checks which musical note the musicalExpression
identifies. Later the octave will be calculated. Out of this data the frequency is calculated.
@see https://pages.mtu.edu/~suits/NoteFreqCalcs.html
**/
double getFrequencyToMusicalNote(char *musicalExpression,double tuningPitch){
    for(size_t k = 0; k < strlen(musicalExpression);k++){
        if(musicalExpression[k] == ' '){
            return 1.0;
        }
    }
    char musicalNotes[][12]={
        "a",
        "ais",
        "b",
        "c",
        "cis",
        "d",
        "dis",
        "e",
        "f",
        "fis",
        "g",
        "gis"
    };
    char *musicalNote = "";
    int i = -1;
    for(int j = 0; j < 12; j++){
        if(isSubstring(musicalNotes[j],musicalExpression)){
            musicalNote = musicalNotes[j];
            i = j;
        }
    }
    //nach oben
    double frequency = 0;
    if(i <= 2){
        frequency = tuningPitch * powl(powl(2,1./12),i);
    }else{
        frequency = tuningPitch * powl(powl(2,1./12),(12-i) * (-1));
    }
    //bestimme so viel Oktaven wie unter oder über a4
    int lengthOfTail = strlen(musicalExpression) - strlen(musicalNote);
    if(isSubstring("''",musicalExpression)){
        frequency = frequency * powl(2.0,(double)lengthOfTail-1);
    } else if(lengthOfTail == 0){
        frequency = frequency / 2.0;
    } else if(isSubstring(",",musicalExpression)){
        frequency = frequency * powl(2.0,(double)(lengthOfTail+1)*(-1));
    }
    return frequency;
}
