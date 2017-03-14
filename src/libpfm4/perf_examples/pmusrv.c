
#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/socket.h>
//#include <linux/in.h>
#include <netinet/in.h>

#include "perf_util.h"

#if defined(__aarch64__)
#define	ARCH_CODE 'a'
#elif defined(__amd64__)
#define ARCH_CODE 'x'
#else
#error ARCH_CODE not defined
#endif


static perf_event_desc_t **all_fds;
static int *num_fds;
static int cmax;
const char **eventnames;
static long long **cntr;
static int secs = 1, argstart, core = -1, repeat = 1, aggregate = 1;
static int sockfd;

void printerr(const char *format,...)
{
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	exit(-1);	
}

void makesock(int port)
{
    struct sockaddr_in addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) printerr("Error initializing socket: %s.\n", strerror(errno));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)))
        printerr("Bind failure: %s\n", strerror(errno));
    listen(sockfd, 1);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
	    printerr("setsockopt(SO_REUSEADDR) failed");
}

int initialize(const char *eventstring)
{
	int ret, c, j;

	ret = pfm_initialize();
	if (ret != PFM_SUCCESS)
		printerr("Error initializing libpfm: $i.\n", ret);

	all_fds = calloc(cmax, sizeof(perf_event_desc_t*));
	num_fds = calloc(cmax, sizeof(int));

	/* For each core */
	for (c = 0; c < cmax; c++) {
		if ((core != -1) && (core != c)) continue;

		perf_event_desc_t *cpufd;
		ret = perf_setup_list_events(eventstring, &all_fds[c], &num_fds[c]);
		if (ret || (num_fds == 0)) {
			printf("Error setting up event: %i %i.\n", ret, num_fds[c]);
			return -1;
		}
		cpufd = all_fds[c];
		cpufd[0].fd = -1;
		/* For each event */
		for (j = 0; j < num_fds[c]; j++) {
			//fprintf(stderr, "values: %s %x.\n", cpufd[j].name, cpufd[j].idx);
			cpufd[j].hw.disabled = 1;
			cpufd[j].hw.read_format = PERF_FORMAT_TOTAL_TIME_RUNNING;
			cpufd[j].fd = perf_event_open(&cpufd[j].hw, -1, c, -1, 0);
			if (cpufd[j].fd == -1)
				printerr("Cannot attach event to CPU%d %s.\n", c, cpufd[c].name);
		}
	}
	return 0;
}

int close_all()
{
    int c, j;
    for (c = 0; c < cmax; c++) {
	if ((core != -1) && (core != c)) continue;
	perf_event_desc_t *cpufd = all_fds[c];
	for (j = 0; j < num_fds[c]; j++) {
	    close(cpufd[j].fd);
	}
    }
    free(num_fds);
    free (all_fds);
    return 0;
}

int collect(char *buffer)
{
	int ret, i, c;
	long long accum[8] = { 0 };
	perf_event_desc_t *cpufd;

	for (c = 0; c < cmax; c++) {
		if ((core != -1) && (core != c)) continue;
		cpufd = all_fds[c];
		for (i = 0; i < num_fds[c]; i++) {
			ret = ioctl(cpufd[i].fd, PERF_EVENT_IOC_ENABLE, 0);
			if (ret)
				printerr("Cannot enable event %s.\n", cpufd[i].name);

			ret = ioctl(cpufd[i].fd, PERF_EVENT_IOC_RESET, 0);
			if (ret)
				printerr("Cannot reset event %s.\n", cpufd[i].name);
		}
	}
	sleep(secs);
	for (c = 0; c < cmax; c++) {
		if ((core != -1) && (core != c)) continue;
		for (i = 0; i < num_fds[c]; i++) {
			cpufd = all_fds[c];
			ret = read(cpufd[i].fd, cpufd[i].values, sizeof(cpufd[i].values));
			if (ret != sizeof(cpufd[i].values)) {
				if (ret == -1)
					printerr("cannot read event %d:%d.\n", i, ret);
			}
			if (aggregate) accum[i] += cpufd[i].values[0];
			else printf("%lu ", cpufd[i].values[0]);

		}
		if (!aggregate) printf("\n");
	}
	if (aggregate) {
		for (c = 0; c < i; c++) { 
		    int l = sprintf(buffer, "%llu ", accum[c]);
		    buffer += l;
		}
	}

	return 0;
}

int main(int argc, const char **argv)
{
	int cfd, lport = 9999;
	char arch_code = ARCH_CODE;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	for (argstart = 1; argstart < argc; argstart++) {
		if (argv[argstart][0] != '-') break;
		switch (argv[argstart][1]) {
		case 'c':
			core = atoi(argv[++argstart]);
			continue;
		case 's':
			secs = atoi(argv[++argstart]);
			continue;
		case 'r':
			repeat = atoi(argv[++argstart]);
			continue;
		case 'a':
			aggregate = 1;
			continue;
		case 'p':
			lport = atoi(argv[++argstart]);
			continue;
		}
	}


	cmax = sysconf(_SC_NPROCESSORS_ONLN);

	cntr = (long long**) malloc(sizeof(long long) * cmax);

	makesock(lport);

	do {
	    int count;
	    char spec[1024];
	    cfd = accept(sockfd, &addr, &addrlen);
	    fprintf(stderr, "Connection: %x.\n", addr.sin_addr.s_addr);

	    if (cfd == -1) printerr("Error accepting client: %s.\n", strerror(errno));

	    write(cfd, &arch_code, 1);

	    do {
		count = 0;
		if (read(cfd, &count, 4) != 4) {
		    close(cfd);
		    break;
		}
		if (count == -9) {
		    if (read(cfd, &count, 4) != 4) {
			close(cfd);
			break;
		    }
		    core = count;
		    continue;
		}
		if (count == -10) {
		    aggregate = 0;
		    continue;
		}
		if (count == -11) {
		    aggregate = 1;
		    continue;
		}
		int ret = read(cfd, spec, count);
		if (ret != count) printerr("Error reading spec %i/%i: %s.\n", ret, count, strerror(errno));

		spec[count] = '\0';

		if (initialize(spec) < 0) {
		    close(cfd);
		    continue;
		}

		collect(spec);
		close_all();

		count = strlen(spec) + 1;
		write(cfd, &count, sizeof(int));
		write(cfd, spec, count);
	    } while (1);
	} while (1);

	pfm_terminate();
	return 0;
}

