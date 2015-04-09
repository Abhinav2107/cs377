#include <conio.h>
#include <fileio.h>
#include <process.h>
#include <string.h>
int main()
{
	Format("ide1","myfs");
	Mount("ide1","/x","myfs");
	return 0;
}