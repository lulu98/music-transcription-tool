/**
@file HelperFunctions.c
@author Lukas Graber
@date 30 May 2019
@brief Implementation of helper functions.
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/HelperFunctions.h"

void msleep(int d){
    usleep(d*1000);
}

void *copyArray(const void *src, size_t n) {
  void *dest = malloc(n);
	if (n > 0 && dest != NULL)
		memcpy(dest, src, n);
	return dest;
}

void shiftWrite(double *arr,short *buff,int arrSize, int buffSize){
  memcpy(arr, arr+buffSize, arrSize*sizeof(double)-buffSize*sizeof(double));
  for (int i = 0; i < buffSize; i++) {
    arr[(arrSize-buffSize) + i] = (float)buff[i] / 32768.0f;
  }
}

char *concat(char *stringA,char *stringB){
  size_t newLength = strlen(stringA) + strlen(stringB) + 1;
  char *newString = malloc(newLength);
  if(newString == NULL){
    perror("Malloc failed!");
    exit(EXIT_FAILURE);
  }
  strcpy(newString,stringA);
  strcat(newString,stringB);
  return newString;
}

char *append(char *stringA,char *stringB){
  unsigned int newLength = strlen(stringA) + strlen(stringB) + 1;
  //char *newString = (char  *)calloc(newLength,sizeof(char));
  char *newString = (char *)realloc(stringA, newLength * sizeof(char));
  if(newString == NULL){
    perror("Malloc failed!");
    exit(EXIT_FAILURE);
  }
  strcat(newString, stringB);
  //strcpy(newString,stringA);
  //strcat(newString,stringB);
  return newString;
}
