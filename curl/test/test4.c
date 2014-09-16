#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	char *arr[] = {"lijianwei", "weiyanping", "test"};
	printf("%d", sizeof(arr)/sizeof(arr[0]));

	return 0;
}

