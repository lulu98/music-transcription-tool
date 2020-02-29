/**
@file CapturedDataPoints.c
@author Lukas Graber
@date 30 May 2019
@brief Implementiation of functions to store information of recognised musical notes for later use.
**/
#include <stdio.h>
#include <stdlib.h>

#include "../include/Structures.h"

#define INIT_SIZE 1024 ///< the initial size of the array of the data structure

/**
All the properties of the data structure are set to their initial values.
dPs the data structure to be intialised
**/
void initCapturedDataPoints(CapturedDataPoints *dPs){
  dPs->size = INIT_SIZE;
  dPs->arr = (MusicalDataPoint*)malloc(dPs->size * sizeof(MusicalDataPoint));
  dPs->pos = 0;
}

/**
Resets all the entries of the array to 0 and and sets current position to 0.
**/
void resetCapturedDataPoints(CapturedDataPoints *dPs){
  //memset(dPs->arr, 0, sizeof(dPs->arr));
  dPs->pos = 0;
}

/**
Inserts a musical data point into the data structure. If the array is too full, then it its size is doubled.
**/
void insertMusicalDataPoint(CapturedDataPoints *dPs,MusicalDataPoint dP){
  if (dPs->pos == dPs->size) {
    dPs->size *= 2;
    dPs->arr = (MusicalDataPoint*)realloc(dPs->arr, dPs->size * sizeof(MusicalDataPoint));
  }
  dPs->arr[dPs->pos++] = dP;
}

/**
The function frees all the properties of the data structure occupying memory space or sets them to NULL/0.
**/
void freeCapturedDataPoints(CapturedDataPoints *dPs) {
  free(dPs->arr);
  dPs->arr = NULL;
  dPs->pos = dPs->size = 0;
}
