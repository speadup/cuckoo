#include <stdlib.h>             
#include <stdio.h>             
#include <string.h>           
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <strings.h>


#define RAETH_SET_DBGLVL 0x1
#define RAETH_TOOGLE_HWNAT 0x2

struct raeth_ioctl_args
{
    int hwnat;      /* 0 -> disable hook, 1 -> enable hook. */
    int dbglvl;     /* 0 -> disable dbg message */
};

void usage()
{
    printf("ethdbg -d N		// set debug level to N\n");
    printf("ethdbg -h X		// enable/disable hwnat\n");
}


static int raeth_set_dbglvl(int fd, unsigned int debug)
{
    struct raeth_ioctl_args opt;
    opt.dbglvl = debug;

	printf("%s(%d,%d)\n", __FUNCTION__, fd, debug);
    if(ioctl(fd, RAETH_SET_DBGLVL, &opt)<0)
	{
		printf("%s error : %s\n", __FUNCTION__, strerror(errno));
		return -1;
    }

    return 0;
}


static int raeth_toggle_hwnat(int fd, unsigned int enable)
{
    struct raeth_ioctl_args opt;
    opt.hwnat = enable;

	printf("%s(%d,%d)\n", __FUNCTION__, fd, enable);
    if(ioctl(fd, RAETH_TOOGLE_HWNAT, &opt)<0)
	{
		printf("%s error : %s\n", __FUNCTION__, strerror(errno));
		return -1;
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int opt;
    char options[] = "d:h:";
    int fd;
	unsigned int debug;
	unsigned int hwnat;

    fd = open("/dev/raeth0", O_RDONLY);
    if (fd < 0)
    {
		printf("Open %s failed\n","/dev/raeth0");
		return 0;
    }

	if(argc < 2)
	{
		usage();
		return;
	}

    while ((opt = getopt (argc, argv, options)) != -1)
	{
		switch (opt)
	 	{
			case 'd':
				debug = strtoll(optarg, NULL, 10);
				raeth_set_dbglvl(fd, debug);
				break;
			case 'h':
				hwnat = strtoll(optarg, NULL, 10);
				raeth_toggle_hwnat(fd, hwnat);
				break;
			default:
				break;
		}
	}

	close(fd);
}






