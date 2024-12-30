#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <poll.h>
#include <string.h>

main() {
	int fd, i, ret, flags;
	struct pollfd s_poll;
	struct strbuf ctrl, data;
	char databuf[2048], ctrlbuf[128];

	fd = open("/dev/ne0", O_RDWR);
	if(fd < 0) {
		perror("Error opening device");
		return 1;
	}
	printf("My fd is %d\n", fd);

	while(1) {
		s_poll.fd = fd;
		s_poll.events = POLLIN;
		s_poll.revents = 0;
		poll(&s_poll, 1, -1);
		memset(&ctrl, 0, sizeof(ctrl));
		memset(&data, 0, sizeof(data));

		ctrl.buf = ctrlbuf;
		ctrl.maxlen = sizeof(ctrlbuf);
		data.buf = databuf;
		data.maxlen = sizeof(databuf);
		flags = 0;
		ret = getmsg(fd, &ctrl, &data, &flags);
		if(ret < 0) {
			perror("getmsg fail");
			continue;
		}
		printf("Got packet of size %d !\n", data.len);
		for(i = 0; i < data.len; i++) {
			printf("%02x%c", (unsigned char)data.buf[i], ((i + 1) % 16 == 0) ? '\n' : ' '); 
		}
		if(i % 16) printf("\n");
	}
}
