/**
@file AudioPreProcessing.h
@author Lukas Graber
@date 30 May 2019
@brief Functions that will do some preprocessing on the audio data.
@see https://de.wikipedia.org/wiki/Fensterfunktion
@see https://en.wikipedia.org/wiki/Window_function
**/
#ifndef AUDIOPREPROCESSING_H_INCLUDED
#define AUDIOPREPROCESSING_H_INCLUDED

/**
@brief This function calculates the faculty.
@param x the number for which the faculty should be built
@return faculty the faculty of x
**/
int faculty(int x);

/**
@brief This function translates fft data into an array containing amplitude values for each bin.
@param amps The array in which the different amplitudes for the bins will be stored.
@param real The real part of the fft data.
@param imag The imaginary part of the fft data.
@param sampleSize The size of the amps array.
**/
void getFrequencySpectrum(double *amps, double *real, double *imag, int sampleSize);

/**
@brief This functions calculates the gaussian number.
@param x current bin
@param N sample size
@return gaussianNumber the calculated gaussian number
**/
double getGaussianNumber(double x,int N);

/**
@brief This function return the positive plank number.
@param epsilon a specific coefficient
@param n current bin
@param N sample size
@return positivePlankNumber the negative plank number
**/
double getPositivePlankNumber(double epsilon, double n, int N);

/**
@brief This function return the negative plank number.
@param epsilon a specific coefficient
@param n current bin
@param N sample size
@return negativePlankNumber the negative plank number
**/
double getNegativePlankNumber(double epsilon, double n, int N);

/**
@brief This function calculates the specific order bessel function of first kind.
@param order the specific order of the bessel function, in this application the order = 0
@param x input for the bessel function
@return besselNumber the calculated number is a result of a bessel function of first kind of specific order
**/
double besselFunctionFirstKind(int order,double x);

/**
@brief This function calculates the n-th Chebyshev polynomial.
@param x input for the chebyshev polynomial
@param n defines the order of chebyshev polynomial
@return chebyshevPolynomial the result of the chebyshev polynomial function
**/
double getChebyshevPolynomial(double x, double n);

/**
@brief This function calculates the Dolph-Chebyshev Number for the Dolph-Chebyshev windowing function.
@param alpha some coefficient
@param beta some coefficient
@param n the current bin
@param N the sample size
@param k some index for the Dolph-Chebyshev functionality
@return dolphChebyshevNumber the result of the Dolph-Chebyshev function
**/
double getDolphChebyshevNumber(double alpha, double beta, double n, int N, int k);

/**
@brief This function calculates the GegenBauer polynomial for the ultraspherical windowing function.
@param n the current bin
@param alpha some coefficient
@param x some factor
@param gegenBauerPolynomial the result of the Gegen-Bauer polynomial
**/
double gegenBauerPolynomial(int n,double alpha,double x);

/**
@brief This function applies windowing on a specified audio data array.
@param audioData the actual array whose entries represent the audio data
@param sampleSize the sample size of the fft
@param windowingFunctionName is used to select between different windowing functions
**/
void applyWindowingFunction(double *audioData, int sampleSize,char *windowingFunctionName);

/**
@brief This function applies a certain frequency bandpass.
@param amps the array containing the audio data
@param sampleSize the sample size of the fft
@param sampleRate how many samples are taken in one second
@param lowFrequency lowest frequency to accept in analysis
@param highFrequency highest frequency to accept in analysis
**/
void applyFrequencyBandpass(double *amps, int sampleSize, int sampleRate, double lowFrequency, double highFrequency);

/**
@brief This function gets the frequency bin with the maximum amplitude.
@param amps the array containing the audio data
@param sampleSize the sample size of the fft
@return frequencyBin the calculated frequencyBin whose entry contains the maximal amplitude compared to all other bins in the array
**/
int getFrequencyBin(double *amps, int sampleSize);

#endif // AUDIOPREPROCESSING_H_INCLUDED
