/**
@file FFT.h
Sources are from: https://www.nayuki.io/page/free-small-fft-in-multiple-languages
@author Lukas Graber
@date 30 May 2019
@brief Functions to do fft.
**/
#ifndef FFT_H_INCLUDED
#define FFT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

bool Fft_transform(double real[], double imag[], size_t n);
bool Fft_inverseTransform(double real[], double imag[], size_t n);
bool Fft_transformRadix2(double real[], double imag[], size_t n);
bool Fft_transformBluestein(double real[], double imag[], size_t n);
bool Fft_convolveReal(const double x[], const double y[], double out[], size_t n);
bool Fft_convolveComplex(
		const double xreal[], const double ximag[],
		const double yreal[], const double yimag[],
		double outreal[], double outimag[], size_t n);

size_t reverse_bits(size_t x, int n);
void *memdup(const void *src, size_t n);


#endif // FFT_H_INCLUDED
