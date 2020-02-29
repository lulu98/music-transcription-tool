/**
 @file Structures.h

 All the necessary structures of the program are gathered in this file. Other c files can just include this header file and have
 access to all the data structures.

 @author Lukas Graber
 @date 30 May 2019
 @brief File containing all the structures used in the code.
 **/
#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

#include "./ApplicationMacros.h"
/**
@brief Structure for musical data point.
One of these audio data points will encode a musical note.
**/
struct MusicalDataPoint{
    double frequency;   ///< frequency of the musical note
    double duration;  ///< duration of the musical note
    double amplitude;   ///< amplitude of the captured audio data
};
typedef struct MusicalDataPoint MusicalDataPoint; ///< use the data structure without the keyword struct

/**
@brief This function initializes a musical data point.
@param dP pointer to musical data point structure
@param frequency the frequency of the recognised note
@param duration the duration of the recognised note
@param amplitude the amplitude of the recognised note
**/
void initMusicalDataPoint(MusicalDataPoint *dP,double frequency,double duration, double amplitude);

/**
@brief This function creates a musical data point structure and returns it.
@param dP pointer to musical data point structure
@param frequency the frequency of the recognised note
@param duration the duration of the recognised note
@param amplitude the amplitude of the recognised note
**/
//MusicalDataPoint *getMusicalDataPoint(double frequency, double duration, double amplitude);

/**
@brief This function prints the content of a musical data point.
@param dP the musical note stored in a musical data point structure
**/
void printMusicalDataPoint(MusicalDataPoint *dP);

/**
@brief This structure contains valuable information of the recognised musical notes.
This information will later be used to construct the melody string in lilypond format.
**/
struct CapturedDataPoints{
    MusicalDataPoint *arr; ///< All the notes recognised defined by their important attributes like frequency,duration and amplitude.
    size_t pos;   ///< The position where to put the next musical data point.
    size_t size; ///< The current size of the array.
};
typedef struct CapturedDataPoints CapturedDataPoints; ///< use the data structure without the keyword struct

/**
@brief This function intializes the structure that stores all the musical data points during melody detection.
@param dPs pointer to the real data structure
**/
void initCapturedDataPoints(CapturedDataPoints *dPs);

/**
@brief This function resets the properties of the data structure.
@param dPs pointer to the real data structure
**/
void resetCapturedDataPoints(CapturedDataPoints *dPs);

/**
@brief This function inserts a musical data point / information for musical note into the structure holding the melody.
@param dPs pointer to the real data structure
@param dP data point containing information to one musical note
**/
void insertMusicalDataPoint(CapturedDataPoints *dPs,MusicalDataPoint dP);

/**
@brief This function frees memory space occupied by the data structure.
@param dPs pointer to the data structure.
**/
void freeCapturedDataPoints(CapturedDataPoints *dPs);

/**
@brief Structure that stores raw audio data and the time when it was captured.
**/
struct AudioCapturePoint{
    short *arr; ///< the actual audio data to be cached
    double captureTime;       ///< the time when the audio data was captured - can be used for rhythm functionality
    size_t pos; ///< position where to insert element next
    size_t size;  ///< size of the data structure
};
typedef struct AudioCapturePoint AudioCapturePoint;///< use the data structure without the keyword struct
/**
@brief This function intializes the data structure.
@param cP one audio capture point of the real time audio capture process.
@param size the size that the array should have
**/
//void initAudioCapturePoint(AudioCapturePoint *cP,size_t size);
AudioCapturePoint *initAudioCapturePoint(size_t size);
/**
@brief This function inserts one audio byte into the data structure.
@param cP pointer to the data structure
@param dP data point of raw audio data
**/
void insertAudioCapturePoint(AudioCapturePoint *cP,short dP);

/**
@brief This function frees memory space taken by an AudioCapturePoint object
@param cP pointer to the data structure
**/
void freeAudioCapturePoint(AudioCapturePoint *cP);


/**
@brief defines one node of the queue
**/
struct queue_node{
  struct queue_node *next; ///< points to the following node in the queue
  AudioCapturePoint *data; ///< data stored in this node
};

/**
@brief Queue that stores captured raw audio data together with their capture time.
This data structure will be shared by the audio capture, as well as by the audio processing thread. Access to
its property is handled over mutexes.
**/
struct AudioDataQueue{
    struct queue_node *front; ///< points to the first node of the queue
    struct queue_node *back; ///< points to the last node of the queue
    size_t size; ///< how many elements there are in the queue
};
typedef struct AudioDataQueue AudioDataQueue;///< use the data structure without the keyword struct

/**
@brief This function initializes the queue data structure.
@param queue points to the queue
**/
void initAudioDataQueue(AudioDataQueue *queue);

/**
@brief This function returns an empty queue structure.
@return queue the created queue data structure
**/
AudioDataQueue *queue_new(void);

/**
@brief This function frees memory space occupied by the queue.
@param queue points to queue
@return isSuccess message whether succeeded or not
**/
int freeAudioDataQueue(AudioDataQueue *queue);

/**
@brief This function frees memory space occupied by the queue.
@param queue points to queue
@return isSuccess message whether succeeded or not
**/
int queue_destroy(AudioDataQueue *queue);

/**
@brief This function checks whether the queue is empty.
@param queue points to queue
@return isSuccess message whether succeeded or not
**/
int queue_empty(AudioDataQueue *queue);

/**
@brief This function removes the element that was inserted into the queue at first.
@param queue points to the queue
@return audioCapturePoint the element stored at the front of queue
**/
AudioCapturePoint *queue_dequeue(AudioDataQueue *queue);

/**
@brief This function insert an element into the queue.
@param queue points to the queue.
@param data element to be stored.
@return isSuccess message whether succeeded or not
**/
int queue_enqueue(AudioDataQueue *queue, AudioCapturePoint *data);

/**
@brief WAVE file header format
**/
struct HEADER {
	unsigned char riff[4];						///< RIFF string
	unsigned int overall_size	;				///< overall size of file in bytes
	unsigned char wave[4];						///< WAVE string
	unsigned char fmt_chunk_marker[4];			///< fmt string with trailing null char
	unsigned int length_of_fmt;					///< length of the format data
	unsigned int format_type;					///< format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	unsigned int channels;						///< no.of channels
	unsigned int sample_rate;					///< sampling rate (blocks per second)
	unsigned int byterate;						///< SampleRate * NumChannels * BitsPerSample/8
	unsigned int block_align;					///< NumChannels * BitsPerSample/8
	unsigned int bits_per_sample;				///< bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char data_chunk_header [4];		///< DATA string or FLLR string
	unsigned int data_size;						///< NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};
typedef struct HEADER HEADER;///< use the data structure without the keyword struct

struct SongConfiguration {
    char *path;
    char *fileName;
    char *title;
    char *subtitle;
    char *composer;
    char *clef;
    char *key;
    char *scaleType;
    char *tempoDescription;
    char *instrument;
    int rhythmNumerator;
    int rhythmDenominator;
    int beatsPerMinute;
};
typedef struct SongConfiguration SongConfiguration;
void initSongConfiguration(SongConfiguration *sc);

struct AudioPreProcessingConfiguration {
    char *windowingFunction;
    double lowFrequency;
    double highFrequency;
};
typedef struct AudioPreProcessingConfiguration AudioPreProcessingConfiguration;
void initAudioPreProcessingConfiguration(AudioPreProcessingConfiguration *appc);

struct AudioInterfaceConfiguration {
    int sampleSize;
    int stepSize;
    int soundCardNumber;
    int recordingTime;
    int channels;
    double tuningPitch;
    double pitchResolutionInCents;
    double rate;
};
typedef struct AudioInterfaceConfiguration AudioInterfaceConfiguration;
void initAudioInterfaceConfiguration(AudioInterfaceConfiguration *aic);

/**
@brief runtime information structure
This structure holds important control and data structures used during runtime.
**/
struct RunTimeInformation{
  double recordingTime;
  int mode;
  int channels;
  int soundCardNumber;
  int stepSize;
  int sampleSize;
  int buffSize;
  int numBins;
  char *windowingFunction;
  int beatsPerMinute;

  int quit;
  int raspi;
  int noOutput;
  int melodyBenchmarking;
  int headPhones;

  int isAudioInterfaceReady;
  int isRecording;
  int isMidiFilePlaying;
  int isCapturingAudio;
  int timeBenchmarking;

  double rate;
  double tuningPitch;
  double pitchResolutionInCents;
  double lowFrequency;
  double highFrequency;

  char *path;
  char *wavFileName;
  char *csvFileName;
  char *lilyPondFileName;
  char *pdfFileName;
  char *midiFileName;
  char *melodyFileName;

  char *instrument;
  char *composer;
  char *title;
};
typedef struct RunTimeInformation RunTimeInformation;
#endif // STRUCTURES_H_INCLUDED
