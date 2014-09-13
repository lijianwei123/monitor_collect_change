#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

void handler(int no)
{
	printf("hanlder: %d\n", no);
	
}

int main(int argc, char **argv)
{
	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	pid_t pid;
	pid = fork();
	if (pid == 0) {
		//child
		sleep(20);
	} else if (pid > 0) {
		sleep(20);
	}
	return 0;
}
