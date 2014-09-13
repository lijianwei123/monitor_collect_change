#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	int fd = open("test.txt", O_RDWR|O_CREAT, 00744);
	close(fd);
	return 0;
}

