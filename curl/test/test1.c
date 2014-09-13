#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


void childBatchMatchTimerHandler(int signalNo)
{
	printf("%d\n", signalNo);

}

int main(int argc, char **argv)
{
        struct itimerval timer, *ovalue = NULL;
	timer.it_value.tv_sec = 3;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 10;
	timer.it_interval.tv_usec = 0;

	signal(SIGALRM, childBatchMatchTimerHandler);
	setitimer(ITIMER_REAL, &timer, NULL);

	while(1);	
	return 0;
	
}
