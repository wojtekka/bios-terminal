#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>

#define BUFSIZE 4096

void usage(const char *argv0, int retcode)
{
	printf("usage: %s [-o offset] [filename]\n\n", argv0);

	exit(retcode);
}

int main(int argc, char **argv)
{
	int ch, offset_set = 0, fd = 0;
	unsigned int offset = 0, fpos = 0, checksum32 = 0;
	unsigned char buf[BUFSIZE], checksum = 0;
	unsigned short checksum16 = 0;
	
	while ((ch = getopt(argc, argv, "o:h")) != -1) {
		switch (ch) {
			case 'o':
				errno = 0;
				offset = strtoul(optarg, NULL, 0);
				if (errno == ERANGE) {
					perror(argv[0]);
					exit(1);
				}
				offset_set = 1;
				break;

			case 'h':
				usage(argv[0], 0);
		}
	}

	if (argc - optind > 1)
		usage(argv[0], 1);

	if (argc - optind == 1)
		fd = open(argv[optind], (offset_set) ? O_RDWR : O_RDONLY);
	else {
		offset_set = 0;
		fd = 0;
	}

	if (fd == -1) {
		perror(argv[optind]);
		exit(1);
	}
	
	for (;;) {
		int res, i;

		res = read(fd, buf, sizeof(buf));

		if (res < 0) {
			perror(argv[0]);
			exit(1);
		}

		if (res == 0)
			break;

		for (i = 0; i < res; i++, fpos++) {
			if (offset_set && fpos == offset)
				continue;

			checksum += buf[i];
			checksum16 += buf[i];
			checksum32 += buf[i];
		}
	}

	printf("0x%.2x, 0x%.4x, 0x%.8x\n", checksum, checksum16, checksum32);

	if (checksum != 0)
		checksum = 256 - checksum;

	if (offset_set) {
		lseek(fd, offset, SEEK_SET);
		if (write(fd, &checksum, 1) < 1) {
			perror("write");
			exit(1);
		}
	}

	if (fd > 0)
		close(fd);

	return 0;
}
