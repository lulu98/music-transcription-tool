/**
@file HelperFunctions.h
@author Lukas Graber
@date 30 May 2019
@brief Some helper functions.
**/

#ifndef HELPERFUNCTIONS_H_INCLUDED
#define HELPERFUNCTIONS_H_INCLUDED

/**
@brief This function stalls the execution of the program for d milliseconds.
@param d time to sleep in milliseconds
**/
void msleep(int d);

/**
@brief This function deep copies an array.
@param src the memory location from where to copy
@param n the size of the structure that should be copied
@return arr the copied array
**/
void *copyArray(const void *src, size_t n);

void shiftWrite(double *arr,short *buff,int arrSize, int buffSize);

/**
@brief This function returns the concatenation of two strings.
@param stringA the first string
@param stringB the second string
@return concat the concatenated string
**/
char *concat(char *stringA,char *stringB);

/**
@brief This function returns the concatenation of two strings.
@param stringA the first string
@param stringB the second string
@return concat the concatenated string
**/
char *append(char *stringA,char *stringB);


void transferFiles();

#endif // HELPERFUNCTIONS_H_INCLUDED
