/**
@file MusicalDataPoint.c
@author Lukas Graber
@date 30 May 2019
@brief Implementiation of functions to deal with musical notes / musical data point.
**/
#include <stdio.h>
#include <stdlib.h>

#include "../include/Structures.h"

/**
All the properties of the data structure are set to their initial values.
**/
void initMusicalDataPoint(MusicalDataPoint *dP,double frequency,double duration, double amplitude){
  dP->frequency = frequency;
  dP->duration = duration;
  dP->amplitude = amplitude;
}

/**
This function can be used if the programmer directly wants to create and initialize the data structure.
**/
/*MusicalDataPoint *getMusicalDataPoint(double frequency, double duration, double amplitude){
  MusicalDataPoint *dP;
  dP->frequency = frequency;
  dP->duration = duration;
  dP->amplitude = amplitude;
  return dP;
}*/

/**
The function prints all the properties (frequency,duration,amplitude) of a musical note to the command line.
This information will later be used to identify one musical note. The frequency can help to identify the pitch,
the duration will help to calculate the note length and the amplitude may be helpful to get a feeling for the
volume of the musical note.
**/
void printMusicalDataPoint(MusicalDataPoint *dP){
  printf("frequency: %f\n", dP->frequency);
  printf("duration: %f\n", dP->duration);
  printf("amplitude: %f\n", dP->amplitude);
}
