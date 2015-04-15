#include <fileio.h>
#include <conio.h>
int main() {
	int fd = Open("/x/blah", O_CREATE | O_WRITE);
	Print("Fd: %d\n", fd);
	char buf[] = "Aman Gour yoloswag";
	Print("Written %d bytes\n", Write(fd, buf, 19));
	Close(fd);
	return 0;
}
