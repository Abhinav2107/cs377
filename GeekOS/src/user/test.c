#include <conio.h>
#include <fileio.h>
#include <process.h>
#include <string.h>
int main()
{
	int fd = Open("/x/hello", O_READ);
	Print("Fd: %d\n", fd);
	Close(fd);	
	return 0;
}
