
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
static int secs = 1, argstart, core = -1, repeat = 1, aggregate = 0;

void printerr(const char *format,...)
{
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	exit(-1);	
}

int initialize(int eventidx)
{
	int ret, i, j;

	ret = pfm_initialize();
	if (ret != PFM_SUCCESS)
		printerr("Error initializing libpfm: $i.\n", ret);

	all_fds = calloc(cmax, sizeof(perf_event_desc_t*));
	num_fds = calloc(cmax, sizeof(int));

	for (i = 0; i < cmax; i++) {
		if ((core != -1) && (core != i)) continue;

		perf_event_desc_t *cpufd;
		ret = perf_setup_list_events(eventnames[eventidx], &all_fds[i], &num_fds[i]);
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
		for (c = 0; c < i; c++) printf("%llu ", accum[c]);
		printf("\n");
	}

	return 0;
}

int main(int argc, const char **argv)
{
	int i, j;

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
		}
	}

	cmax = sysconf(_SC_NPROCESSORS_ONLN);

	cntr = (long long**) malloc(sizeof(long long) * cmax);

	eventnames = argv;

	for (i = argstart; i < argc; i++) {
		initialize(i);

		for (j = 0; j < repeat; j++)
			collect();

		pfm_terminate();
	}
	return 0;
}
