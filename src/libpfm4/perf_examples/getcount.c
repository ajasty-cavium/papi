
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

#include "perf_util.h"

static perf_event_desc_t **all_fds;
static int *num_fds;
static int cmax;
const char **eventnames;
static long long **cntr;

void printerr(const char *format,...)
{
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	exit(-1);	
}

int initialize()
{
	int ret, i, j;

	ret = pfm_initialize();
	if (ret != PFM_SUCCESS)
		printerr("Error initializing libpfm: $i.\n", ret);

	all_fds = calloc(cmax, sizeof(perf_event_desc_t*));
	num_fds = calloc(cmax, sizeof(int));

	for (i = 0; i < cmax; i++) {
		perf_event_desc_t *cpufd;
		ret = perf_setup_list_events(eventnames[1], &all_fds[i], &num_fds[i]);
		if (ret || (num_fds == 0)) 
			printerr("Error setting up event: %i %i.\n", ret, num_fds);
		cpufd = all_fds[i];
		cpufd[0].fd = -1;
		for (j = 0; j < num_fds[i]; j++) {
			//fprintf(stderr, "values: %s %x.\n", cpufd[j].name, cpufd[j].idx);
			cpufd[j].hw.disabled = 0;
			cpufd[j].hw.read_format = PERF_FORMAT_TOTAL_TIME_RUNNING;
			cpufd[j].fd = perf_event_open(&cpufd[j].hw, -1, i, -1, 0);
			if (cpufd[j].fd == -1)
				printerr("Cannot attach event to CPU%d %s.\n", i, cpufd[i].name);
		}
	}
	return 0;
}

int collect()
{
	int ret, i, c;
	perf_event_desc_t *cpufd;

	for (c = 0; c < cmax; c++) {
	cpufd = all_fds[c];
	for (i = 0; i < num_fds[c]; i++) {
		ret = ioctl(cpufd[i].fd, PERF_EVENT_IOC_ENABLE, 0);
		if (ret)
			printerr("Cannot enable event %s.\n", cpufd[i].name);
	}
	}
	sleep(1);
	for (c = 0; c < cmax; c++) {
	for (i = 0; i < num_fds[c]; i++) {
		ret = read(cpufd[i].fd, cpufd[i].values, sizeof(cpufd[i].values));
		if (ret != sizeof(cpufd[i].values)) {
			if (ret == -1)
				printerr("cannot read event %d:%d.\n", i, ret);
		}
		printf("%lu\t\t", cpufd[i].values[0]);
	}
	printf("\n");
	}
	//fprintf(stderr, "\n");

	return 0;
}

int main(int argc, const char **argv)
{
	cmax = sysconf(_SC_NPROCESSORS_ONLN);

	cntr = (long long**) malloc(sizeof(long long) * cmax);

	eventnames = argv;
	initialize();

	collect();

	pfm_terminate();
	return 0;
}
