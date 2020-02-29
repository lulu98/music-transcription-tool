/**
@file HeadphoneFunctions.h
There are two options to check for headphones. Either use the interface of acpi_listen or use the amixer command
to get the numid of headphone jack interface. The last option is currently used, but will have big limitations for
other devices, because the num id of headphone jack will most certainly be different.
@author Lukas Graber
@date 30 May 2019
@brief Functions to deal with headphones.
**/
#ifndef HEADPHONEFUNCTIONS_H_INCLUDED
#define HEADPHONEFUNCTIONS_H_INCLUDED

/**
@brief This function checks whether something is plugged in or out of headphone jack
**/
void headphoneJackTest();

/**
@brief This functions checks whether something is plugged into the headphone Jack.
@return true if headphone is connected over headphone jack, false otherwise
**/
int headPhoneConnected();

/**
@brief This function halts further execution of program till headphones are plugged into headhpone jack.
**/
void waitForHeadPhones();

#endif // HEADPHONEFUNCTIONS_H_INCLUDED
