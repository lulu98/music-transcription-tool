/**
@file ApplicationMacros.h
This file contains control flags and information that should be accessed from every file at any time.
@author Lukas Graber
@date 30 May 2019
@brief File containing control information.
 **/

#ifndef APPLICATIONMACROS_H_INCLUDED
#define APPLICATIONMACROS_H_INCLUDED

#define TRUE 1      ///< defines the value for TRUE
#define FALSE 0     ///< define the value for FALSE

//control features configuration
#define RECORDING_TIME 15              ///<defines the time period that audio should be captured
#define POSTPROCESSING TRUE            ///<defines if it is a real time analysis or not -> maybe not necessary
#define BENCHMARKING TRUE              ///<defines if we do benchmark testing
#define WITH_HEADPHONES FALSE         ///< defines whether to use headphones or not

//audio interface configuration
#define SAMPLE_SIZE 4096        ///< defines the sample size to be captured before running fft over it
#define SAMPLE_RATE 44100       ///< defines the sample rate that the microphone captures data
#define BUFFER_SIZE 128         ///< defines the buffer size for the real time audio capture
#define NUM_CHANNELS 2          ///< defines the number of channels of the microphone
#define SOUND_CARD_NUMBER 0     ///< defines the sound card number\n command: cat /proc/asound/cards

//audio preprocessing
#define WINDOWING_FUNCTION "Rectangle"  ///< name for fitting function
#define LOW_FREQUENCY 100.0               ///< lowest frequency for bandpassing
#define HIGH_FREQUENCY 10000.0            ///< highest frequency for bandpassing

//audio transcription configuration
#define TUNING_PITCH 440.0                ///< the reference pitch a4
#define PITCH_RESOLUTION 40.0             ///< pitch difference in cents to be allowed to be seen as the same note
#define LOUDNESS_THRESHOLD_FACTOR 0.99  ///< for chord detection, loudness difference logarithmically
#define RHYTHM_RESOLUTION 16            ///< the resolution that should be taken for the smallest distinguishable note length

//lilypond configuration
#define TITLE "Title"                   ///< title of the song that is recorded
#define SUBTITLE "Subtitle"             ///< subtitle of the song that is recorded
#define COMPOSER "Composer"             ///< composer of the song
//#define INSTRUMENT "voice oohs"         ///< instrument that should be used in midi file and used for notesheet\n more information: http://lilypond.org/doc/v2.19/Documentation/notation/midi-instruments
#define INSTRUMENT "trumpet"         ///< instrument that should be used in midi file and used for notesheet\n more information: http://lilypond.org/doc/v2.19/Documentation/notation/midi-instruments
#define CLEF "treble"                   ///< clef for the notesheet\n options: treble,alto,tenor,bass\n more information: http://lilypond.org/doc/v2.19/Documentation/notation/clef-styles
#define KEY "c"                         ///< key for the notesheet
#define SCALE_TYPE "major"              ///< scale type for key signature\n options: major, minor
#define RHYTHM_NUMERATOR 4              ///< together with the rhyhtm denominator the rhythm is build
#define RHYTHM_DENOMINATOR 4            ///< together with the rhythm numerator the rhythm is build
#define TEMPO_DESCRIPTION "Allegro"     ///< belongs to tempo description on notesheet\n options: http://lilypond.org/doc/v2.18/Documentation/notation/displaying-rhythms
#define BEATS_PER_MINUTE 90             ///< tempo description, is used to actually calculate pace of metronome

#endif // APPLICATIONMACROS_H_INCLUDED
