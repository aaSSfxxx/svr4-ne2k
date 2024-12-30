#include <unistd.h>
#include <fcntl.h>

main() {
	int fd, fd2;

	fd = open("/dev/ne0", O_RDWR);
	if(fd < 0) {
		perror("Error opening device");
		return 1;
	}
	printf("My fd is %d\n", fd);
	fd2 = open("/dev/ne0", O_RDWR);
	if(fd2 < 0) {
		perror("Error opening device");
		return 1;
	}
	printf("My fd is %d\n", fd2);
}
