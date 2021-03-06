/**
@file AudioTranscription.h
@author Lukas Graber
@date 30 May 2019
@brief Fuctions to achieve audio transcription out of preprocessed fft data.
**/

#ifndef AUDIOTRANSCRIPTION_H_INCLUDED
#define AUDIOTRANSCRIPTION_H_INCLUDED

#include "../include/Structures.h"

/**
@brief This function is possible to find the bins with the numBins maximal frequencies.
@param bins array to save the indexes of the calculated bins
@param numBins number of bins for which the indexes should be find out
@param out array which holds the power levels of the different bins
@param N sample size
@param Fs sample rate
@param tuningPitch the reference frequency used (generally a4->440Hz)
**/
void getBins(int *bins, int numBins, double *out, int N, double Fs, double tuningPitch);

/**
@brief This function returns the corresponding frequency to a musical note.
@param musicalNote note in musical notation according to lilypond format
@param octave specifies the octave of the musical tone
@param tuningPitch the reference tone (generally a4->440Hz)
**/
double getFrequency(char *musicalNote, int octave, double tuningPitch);

/**
@brief This function checks whether a certain string is a substring of another string.
@param sub the substring char array / string
@param s the char array / string to test against
@return isSubstring true if sub is substring of s, false otherwise
**/
int isSubstring(char sub[],char s[]);

/**
@brief This function calculates the nearest note duration to match a the lowest duration to distinguish by the application.
@param actualNoteDuration the duration that the audio processing set for the current note
@return nearestNoteDuration an adjusted duration for more precise calculation of note length
**/
double getNearestNoteDuration(double actualNoteDuration, int resolution, int beatsPerMinute,int rhythmDenominator);

/**
@brief This function generates a lilypond file containing a specified string as melody.
@param melodyStringRepresentation the string containing the melody which was recognized during the run of the pipeline
@param path File location to store lilypond file generated by this function
**/
void GenerateLilyPondFile(char *melodyStringRepresentation,char *path);

/**
@brief This function is a more flexible implementation to generate a lilypond file.
@param melodyStringRepresentation the string containing the melody which was recognized during the run of the pipeline
@param path File location to store lilypond file generated by this function
@param bpm specifies tempo as beats per minute used for the recording
@param instrument instrument used for midi file and notesheet generation
**/
void GenerateBenchmarkingLilyPondFile(char *melodyStringRepresentation,char *path, int bpm, char *instrument, char *title, char *composer);

/**
@brief This function calculates the duration of one note passed in lilypond format.
@param musicalExpression the string containing information about one note in lilypond format
@return noteLength the calculated note length / duration for the melody string in lilypond format
**/
double getNoteLength(char *musicalExpression, int beatsPerMinute);

/**
@brief This function generates the musical note combined with its length in lilypond format.
@param musicalNote the musical note for which a string should be constructed that contains the note together with the duration
@param timeDifference the duration of the current note
@param beatsPerMinute beats per minute to calculate the basic measure
@return musicalExpression string containing the muscialNote together with note duration in lilypond format
**/
char *calculateNoteLength(char *musicalNote, double duration, int resolution, int beatsPerMinute, int rhythmDenominator);

/**
@brief This function calculates the total time of the melodyString passed to it.
@param testMelody the string containing a melody in lilypond format.
@return timeForMelody the duration of melody in milliseconds
**/
double getTotalDurationOfMelodyString(char *testMelody, int beatsPerMinute);

/**
@brief This function calculates the duration of an audio file in seconds.
@param audioFile File location where audio file is located
@return durationOfAudioFile duration of audio file in seconds
**/
double getDurationOfAudioFileInSeconds(char *audioFile);

/**
@brief This function transforms a frequency into a musical note.
@param frequency the frequency which was extracted in earlier stages of the pipeline
@param tuningPitch specifies the reference tone (generally a4->440Hz)
@param pitchResolution defines margin for note detection, specified in cent
@return musicalNote the calculated musical note in lilypond format
**/
char *getMusicalNote(double frequency, double tuningPitch, double pitchResolution);
//char *getMusicalNote(double frequency, double tuningPitch);

/**
@brief This function calculates the frequency to a musical expression.
@param musicalExpression string containing a musical note in lilypond format
@return frequency the calculated frequency
**/
double getFrequencyToMusicalNote(char *musicalExpression, double tuningPitch);

#endif // AUDIOTRANSCRIPTION_H_INCLUDED
