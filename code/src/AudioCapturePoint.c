/**
@file AudioCapturePoint.c
@author Lukas Graber
@date 30 May 2019
@brief Implementiation of functions to cope with a audio raw data when capturing in real time.
**/
#include <stdio.h>
#include <stdlib.h>

#include "../include/Structures.h"

/**
All the characteristics of the structure are initialized.
**/
AudioCapturePoint *initAudioCapturePoint(size_t size){
  AudioCapturePoint *cP = malloc(sizeof(AudioCapturePoint));
  cP->size = size;
  cP->arr = malloc(cP->size * sizeof(short));
  cP->pos = 0;
  cP->captureTime = 0;
  return cP;
}

/**
The function only inserts to a certain number of elements or it has rather just a certain amount of
space reserved and is not designed to be growing.
**/
void insertAudioCapturePoint(AudioCapturePoint *cP,short dP){
  if (cP->pos == cP->size) {
    printf("%s\n", "Array is full!");
  }else{
    cP->arr[cP->pos++] = dP;
  }
}

void freeAudioCapturePoint(AudioCapturePoint *cP){
  free(cP->arr);
  free(cP);
}
