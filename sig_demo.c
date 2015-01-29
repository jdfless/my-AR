/*
Jonathan Flessner
flessnej@onid.oregonstate.edu
CS311-400
Homework 4.2
28 April 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

//signal handling functions
static void sigOneHandler(int sig)
{
	printf("SIGUSR1 has been caught\n");
}

static void sigTwoHandler(int sig)
{
	printf("SIGUSR2 has been caught\n");
}

static void sigIntHandler(int sig)
{
	printf("SIGINT has been caught, terminating the program\n");
	exit(EXIT_SUCCESS);
}

int main()
{
	//set up sigaction
	struct sigaction s1;
	struct sigaction s2;
	struct sigaction si;

	s1.sa_handler = sigOneHandler;
	s2.sa_handler = sigTwoHandler;
	si.sa_handler = sigIntHandler;
	s1.sa_flags = 0;
	s2.sa_flags = 0;
	si.sa_flags = 0;
	
	//set signal handling
	sigaction(SIGUSR1, &s1, NULL);
	sigaction(SIGUSR2, &s2, NULL);
	sigaction(SIGINT, &si, NULL);

	//send signals from current process
	kill(getpid(), SIGUSR1);
	kill(getpid(), SIGUSR2);
	kill(getpid(), SIGINT); 

	return 0;
}