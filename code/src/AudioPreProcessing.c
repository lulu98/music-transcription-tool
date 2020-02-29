/**
@file AudioPreProcessing.c
The file mainly contains functions to implement windowing funcitonality.
@author Lukas Graber
@date 30 May 2019
@brief Implementation of the functions to implement audio preprocessing functionality.
@see https://de.wikipedia.org/wiki/Fensterfunktion
@see https://en.wikipedia.org/wiki/Window_function
**/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../include/ApplicationMacros.h"
#include "../include/AudioPreProcessing.h"
#include "../include/HelperFunctions.h"

/**
The function will calculate the power level/ amplitude of the fft data in each bin. It puts the data in logarithmic scale back to the array.
**/
void getFrequencySpectrum(double *amps, double *real, double *imag, int sampleSize){
    for(int i = 0; i < sampleSize; i++){
        double magnitude = sqrt(real[i]*real[i] + imag[i]*imag[i]);
        double amplitude = 10 * log10(magnitude);
        amps[i] = amplitude;
    }
}

/**
**/
double getGaussianNumber(double x,int N){
    double sigma = 0.14;
    double base = M_E;
    double exponent = -pow(((x-N/2)/(2*(N+1)*sigma)),2);
    return pow(base,exponent);
}

/**
**/
double getPositivePlankNumber(double epsilon, double n, int N){
    return 2*epsilon * ((1/(1+(2*n/(N-1))))+(1/(1-2*epsilon + (2*n/(N-1)))));
}

/**
**/
double getNegativePlankNumber(double epsilon, double n, int N){
    return 2*epsilon * ((1/(1-(2*n/(N-1))))+(1/(1-2*epsilon - (2*n/(N-1)))));
}

/**
**/
int faculty(int x){
    if(x>1){
        return x*faculty(x-1);
    }else{
        return 1;
    }
}

/**
**/
double besselFunctionFirstKind(int order,double x){
    int numIterations = 1000;
    double sum = 0;
    double numerator = 0;
    double denominator = 0;
    for(int k = 0; k < numIterations; k++){
        numerator = pow((0.25*pow(x,2.0)),k);
        denominator = faculty(k)*faculty(order+k);
        sum = sum + (numerator/denominator);
    }
    return pow(0.5*x,order);
}

/**
**/
double getChebyshevPolynomial(double x, double n){
    double polynomial = 0;
    if(-1 <= x && x <= 1){
        polynomial = cos(n*acos(x));
    }else if(x >= 1){
        polynomial = cosh(n*acosh(x));
    }else if(x <= -1){
        polynomial = pow(-1,n)*cosh(n*acosh(-x));
    }
    return polynomial;
}

/**
**/
double getDolphChebyshevNumber(double alpha, double beta, double n, int N, int k){
    return getChebyshevPolynomial(beta*cos(M_PI*k/(N+1)),n)/powf(10.0, alpha);
}

/**
**/
double gegenBauerPolynomial(int n,double alpha,double x){
    if(n == 0){
        return 1;
    }else if(n == 1){
        return 2*alpha*x;
    }else{
        return (1/n) * (2*x*(n+alpha-1)*gegenBauerPolynomial(n-1,alpha,x)-(n+2*alpha-2)*gegenBauerPolynomial(n-2,alpha,x));
    }
}

/**
The windowingFunctionName selects a specific windowing function. This function will be run on every bin in the audioData array.
@see https://en.wikipedia.org/wiki/Window_function
**/
void applyWindowingFunction(double *audioData, int sampleSize, char *windowingFunctionName){
    double windowCoefficient = 1.0;
    int N = sampleSize;
    for(int n = 0; n < N; n++){
        if(strcmp(windowingFunctionName, "rectangle")==0){
            windowCoefficient = 1.0;
        }else if(strcmp(windowingFunctionName, "hann")==0){
            windowCoefficient = 0.5 - 0.5 * cos((2 * M_PI * n)/N);
        }else if(strcmp(windowingFunctionName, "hamming")==0){
            windowCoefficient = 0.54 - 0.46 * cos((2 * M_PI * n)/N);
        }else if(strcmp(windowingFunctionName, "triangle")==0 || strcmp(windowingFunctionName, "bartlett")==0 || strcmp(windowingFunctionName, "fejer")==0){ //B-Spline Window
            int L = N; //can also be N+1 or N+2
            windowCoefficient = 1 - fabs((n-L/2)/(L/2));
        }else if(strcmp(windowingFunctionName, "parzen")==0){
            int L = N+1;
            double cofactor = 0;
            if((0 <= fabs(n-L/2) && fabs(n-L/2) <= L/4)){
                cofactor = 1 - 6 * pow((n/(L/2)),2.0) * (1 - abs(n)/(L/2));
            }else if(L/4 < fabs(n-L/2) && fabs(n-L/2) <= L/2){
                cofactor = 2*pow((1-(abs(n)/(L/2))),3.0);
            }
            windowCoefficient = cofactor * (n-N/2);
        }else if(strcmp(windowingFunctionName, "welch")==0){
            windowCoefficient = 1- pow(((n-N/2)/(N/2)),2.0);
        }else if(strcmp(windowingFunctionName, "sine")==0){
            windowCoefficient = sin(M_PI * n/N);
        }else if(strcmp(windowingFunctionName, "blackman-original")==0){
            double a = 0.16;
            double a0 = (1-a)/2;
            double a1 = 1/2;
            double a2 = a/2;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "blackman-exact")==0){
            double a0 = 7938.0/18608.0;
            double a1 = 9240.0/18608.0;
            double a2 = 1430.0/18608.0;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "nuttall")==0){
            double a0 = 0.355768;
            double a1 = 0.487396;
            double a2 = 0.144232;
            double a3 = 0.012604;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N) - a3 * cos(6*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "blackman-nuttall")==0){
            double a0 = 0.3635819;
            double a1 = 0.4891775;
            double a2 = 0.1365995;
            double a3 = 0.0106411;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N) - a3 * cos(6*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "blackman-harris")==0){
            double a0 = 0.35875;
            double a1 = 0.48829;
            double a2 = 0.14128;
            double a3 = 0.01168;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N) - a3 * cos(6*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "flattop")==0){
            double a0 = 0.21557895;
            double a1 = 0.41663158;
            double a2 = 0.277263158;
            double a3 = 0.083578947;
            double a4 = 0.006947368;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N) - a3 * cos(6*M_PI*n/N) + a4 * cos(8*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "rife-vincent")==0){
            double a0 = 1.0;
            double a1 = 4.0/3.0;
            double a2 = 1.0/3.0;
            windowCoefficient = a0 - a1 * cos(2*M_PI*n/N) + a2 * cos(4*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "gauss")==0){
            double sigma = 0.5;
            double exponent = -0.5 * pow(((n-N/2)/(sigma*N/2)),2.0);
            windowCoefficient = pow(M_E,exponent);
        }else if(strcmp(windowingFunctionName, "gauss-confined")==0){
            windowCoefficient = getGaussianNumber(n,N) - (getGaussianNumber(-0.5,N)*(getGaussianNumber(n+N+1,N) + getGaussianNumber(n-N-1,N)))/(getGaussianNumber(0.5+N,N) + getGaussianNumber(-1.5-N,N));
        }else if(strcmp(windowingFunctionName, "tukey")==0){
            double alpha = 0.5; //can be chosen freely
            if(0 <= n && n < (alpha*N/2)){
                windowCoefficient = 0.5 * (1+cos(M_PI * ((2*n/(alpha*N))-1)));
            }else if((alpha*N/2) <= alpha && n <= (N*(1-alpha/2))){
                windowCoefficient = 1.0;
            }else if((N*(1-alpha/2)) < n && n <= N){
                windowCoefficient = 0.5 * (1+cos(M_PI * ((2*n/(alpha*N))-(2/alpha)+1)));
            }
        }else if(strcmp(windowingFunctionName, "plank-taper")==0){
            double epsilon = 0.1;
            if(0 <= n && n <= (epsilon * N)){
                windowCoefficient = 1/(getPositivePlankNumber(epsilon,n,N));
            }else if((epsilon*N) < n && n < ((1-epsilon)*N)){
                windowCoefficient = 1.0;
            }else if(((1-epsilon)*N) < n && n <= N){
                windowCoefficient = 1/(getNegativePlankNumber(epsilon,n,N)+1);
            }else{
                windowCoefficient = 0;
            }
        }else if(strcmp(windowingFunctionName, "kaiser")==0){
            double alpha = 3.0;
            double numerator = besselFunctionFirstKind(0,M_PI * alpha *sqrt(1-pow(2*n/N-1,2.0)));
            double denominator = besselFunctionFirstKind(0,M_PI*alpha);
            windowCoefficient = numerator/denominator;
        }else if(strcmp(windowingFunctionName, "dolph-chebyshev")==0){
            double alpha = 5.0;
            double beta = cosh((1/N)* acosh(pow(10,alpha)));
            double sum = 0;
            for(int k = 0; k < N; k++){
                sum = sum + getDolphChebyshevNumber(alpha,beta,n,N,k) * pow(M_E,2*M_PI*k*(n-N/2)/(N+1));
            }
            double cofactor = 1/(N+1)* sum;
            windowCoefficient = cofactor * (n-N/2);
        }else if(strcmp(windowingFunctionName, "ultraspherical")==0){
            double mu = -0.5;
            double x0 = 1.0;
            double sum = 0;
            for(int k = 1; k<=N/2;k++){
                sum = sum + gegenBauerPolynomial(N,mu,x0*cos(k*M_PI/(N+1))) * cos(2*n*M_PI*k/(N+1));
            }
            windowCoefficient = 1/(N+1) * (gegenBauerPolynomial(N,mu,x0)+sum);
        }else if(strcmp(windowingFunctionName, "saramaeki")==0){
            double mu = 1.0;
            double x0 = 1.0;
            double sum = 0;
            for(int k = 1; k<=N/2;k++){
                sum = sum + gegenBauerPolynomial(N,mu,x0*cos(k*M_PI/(N+1))) * cos(2*n*M_PI*k/(N+1));
            }
            windowCoefficient = 1/(N+1) * (gegenBauerPolynomial(N,mu,x0)+sum);
        }else if(strcmp(windowingFunctionName, "exponential")==0 || strcmp(windowingFunctionName, "poisson")==0){
            double decayOfdB = 1;
            double tau = (N/2) * (8.69/decayOfdB);
            windowCoefficient = pow(M_E,-fabs(n-N/2)*(1/tau));
        }else if(strcmp(windowingFunctionName, "bartlett-hann")==0){
            double a0 = 0.62;
            double a1 = 0.48;
            double a2 = 0.38;
            windowCoefficient = a0 - a1 * fabs(n/N-0.5) - a2 * cos(2*M_PI*n/N);
        }else if(strcmp(windowingFunctionName, "hann-poisson")==0){
            double alpha = 2.0;
            windowCoefficient = 0.5 * (1-cos(2*M_PI*n/N)) * pow(M_E,(-alpha * fabs(N-2*n))/N);
        }else if(strcmp(windowingFunctionName, "lanczos")==0){
            windowCoefficient = sin(M_PI * ((2*n/N)-1))/(M_PI*((2*n/N)-1));
        }else{
            windowCoefficient = 1.0;
        }
        audioData[n] = audioData[n] * windowCoefficient;
    }
}

/**
In the ApplicatinoMacros.h file, you can specify the low and high frequency. The idea is that you can narrow down
the frequency region for which the human ear is perceptible.
**/
void applyFrequencyBandpass(double *amps, int sampleSize, int sampleRate, double lowFrequency, double highFrequency){
    int minBin = (int)(lowFrequency * sampleSize / sampleRate);
    int maxBin = (int)(highFrequency * sampleSize / sampleRate);
    for(int i = 0; i < minBin;i++){
        amps[i] = 0;
    }
    for(int i = maxBin; i < sampleSize; i++){
        amps[i] = 0;
    }
}

/**
The function iterates over the amps array and keeps track which bin has the highest amplitude. At the end
the index of this bin will be returned.
**/
int getFrequencyBin(double *amps, int sampleSize){
    double maxAmp = 0;
    int maxInd = 0;
    for(int i = 0; i < sampleSize; i++){
        double amplitude = amps[i];
        if(amplitude > maxAmp){
            maxAmp = amplitude;
            maxInd = i;
        }
    }
    return maxInd;
}
