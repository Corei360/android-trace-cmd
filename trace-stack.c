/*
 * Copyright (C) 2009, 2010 Red Hat Inc, Steven Rostedt <srostedt@redhat.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  see <http://www.gnu.org/licenses>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "trace-local.h"

#define PROC_FILE "/proc/sys/kernel/stack_tracer_enabled"

enum stack_type {
	STACK_START,
	STACK_STOP,
	STACK_RESET,
	STACK_REPORT
};

static void test_available(void)
{
	struct stat buf;
	int fd;

	fd = stat(PROC_FILE, &buf);
	if (fd < 0)
		die("stack tracer not configured on running kernel");
}

/*
 * Returns:
 *   -1 - Something went wrong
 *    0 - File does not exist (stack tracer not enabled)
 *    1 - Success
 */
static int read_proc(int *status)
{
	struct stat stat_buf;
	char buf[64];
	long num;
	int fd;
	int n;

	if (stat(PROC_FILE, &stat_buf) < 0) {
		/* stack tracer not configured on running kernel */
		*status = 0; /* not configured means disabled */
		return 0;
	}

	fd = open(PROC_FILE, O_RDONLY);

	if (fd < 0)
		return -1;

	n = read(fd, buf, sizeof(buf));
	close(fd);

	if (n <= 0)
		return -1;

	if (n >= sizeof(buf))
		return -1;

	buf[n] = 0;
	errno = 0;
	num = strtol(buf, NULL, 10);

	/* Check for various possible errors */
	if (num > INT_MAX || num < INT_MIN || (!num && errno))
		return -1;

	*status = num;
	return 1; /* full success */
}

/* Public wrapper of read_proc() */
int tracecmd_stack_tracer_status(int *status)
{
	return read_proc(status);
}

/* NOTE: this implementation only accepts new_status in the range [0..9]. */
static void change_stack_tracer_status(unsigned new_status)
{
	char buf[1];
	int status;
	int ret;
	int fd;
	int n;

	if (new_status > 9) {
		warning("invalid status %d\n", new_status);
		return;
	}

	ret = read_proc(&status);
	if (ret < 0)
		die("error reading %s", PROC_FILE);

	if (ret > 0 && status == new_status)
		return; /* nothing to do */

	fd = open(PROC_FILE, O_WRONLY);
	if (fd < 0)
		die("writing %s", PROC_FILE);

	buf[0] = new_status + '0';

	n = write(fd, buf, 1);
	if (n < 0)
		die("writing into %s", PROC_FILE);
	close(fd);
}

static void start_trace(void)
{
	change_stack_tracer_status(1);
}

static void stop_trace(void)
{
	change_stack_tracer_status(0);
}

static void reset_trace(void)
{
	char *path;
	char buf[1];
	int fd;
	int n;

	path = tracecmd_get_tracing_file("stack_max_size");
	fd = open(path, O_WRONLY);
	if (fd < 0)
		die("writing %s", path);

	buf[0] = '0';
	n = write(fd, buf, 1);
	if (n < 0)
		die("writing into %s", path);
	tracecmd_put_tracing_file(path);
	close(fd);
}

static void read_trace(void)
{
	char *buf = NULL;
	int status;
	char *path;
	FILE *fp;
	size_t n;
	int r;

	if (read_proc(&status) <= 0)
		die("Invalid stack tracer state");

	if (status > 0)
		printf("(stack tracer running)\n");
	else
		printf("(stack tracer not running)\n");

	path = tracecmd_get_tracing_file("stack_trace");
	fp = fopen(path, "r");
	if (!fp)
		die("reading to '%s'", path);
	tracecmd_put_tracing_file(path);

	while ((r = getline(&buf, &n, fp)) >= 0) {
		/*
		 * Skip any line that starts with a '#'.
		 * Those talk about how to enable stack tracing
		 * within the debugfs system. We don't care about that.
		 */
		if (buf[0] != '#')
			printf("%s", buf);

		free(buf);
		buf = NULL;
	}

	fclose(fp);
}

enum {
	OPT_reset	= 253,
	OPT_stop	= 254,
	OPT_start	= 255,
};

void trace_stack (int argc, char **argv)
{
	enum stack_type trace_type = STACK_REPORT;
	int c;

	if (argc < 2)
		usage(argv);

	if (strcmp(argv[1], "stack") != 0)
		usage(argv);

	for (;;) {
		int option_index = 0;
		static struct option long_options[] = {
			{"start", no_argument, NULL, OPT_start},
			{"stop", no_argument, NULL, OPT_stop},
			{"reset", no_argument, NULL, OPT_reset},
			{"help", no_argument, NULL, '?'},
			{NULL, 0, NULL, 0}
		};

		c = getopt_long (argc-1, argv+1, "+h?",
			long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv);
			break;
		case OPT_start:
			trace_type = STACK_START;
			break;
		case OPT_stop:
			trace_type = STACK_STOP;
			break;
		case OPT_reset:
			trace_type = STACK_RESET;
			break;
		default:
			usage(argv);
		}
	}

	test_available();

	switch (trace_type) {
	case STACK_START:
		start_trace();
		break;
	case STACK_STOP:
		stop_trace();
		break;
	case STACK_RESET:
		reset_trace();
		break;
	default:
		read_trace();
		break;
	}

	return;
}
