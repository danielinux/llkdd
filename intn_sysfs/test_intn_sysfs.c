/*
 * Userspace test programm for the /dev/intn driver
 *
 * Copyright (C) 2014 Rafael do Nascimento Pereira <rnp@25ghz.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This programs tests the intn (/dev/intn) driver. It starts N (N = 4 if the
 * user does not provide a value on the command line) threads, where each one
 * of them increments intn value by one concurrently. At the end the it is
 * expected to have a final value of X (initial) + N.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define NUM_THREADS 4
#define INT_LEN    13
#define SYSFSFILE  "/sys/devices/platform/intn_sysfs/intn"

const char *opthelp = "-h\0";

struct tdata {
	uint32_t tnum;
	char  intn[INT_LEN];
};


void *inc_devintn(void *data)
{
	struct tdata *t;
	int fd, ret;
	char myint[INT_LEN];

	if (data == NULL)
		return NULL;

	t = data;
	fd = open(SYSFSFILE, O_RDWR);

	if (fd == -1) {
		printf("thread[%d]: error opening from %s (%s)\n",
				t->tnum, SYSFSFILE, strerror(errno));
		return NULL;
	}

	memset(myint, 0, INT_LEN);

	if (read(fd, myint, INT_LEN) < 0) {
		printf("thread[%d]: error reading from %s (%s)\n",
				t->tnum, SYSFSFILE, strerror(errno));
		return NULL;
	} else {
		strncpy(t->intn, myint, INT_LEN);
		printf("thread[%d]: value read from %s: %s\n", t->tnum, SYSFSFILE, t->intn);
	}

	int tmp = (int)strtol(myint, NULL, 10);
	tmp++;
	memset(myint, 0, INT_LEN);

	if ((ret = snprintf(myint, INT_LEN, "%d", tmp)) < 0) {
		printf("thread[%d]: error converting integer (%d)\n", t->tnum, ret);
	} else {
		if ((ret = write(fd, myint, INT_LEN)) < 0) {
			printf("thread[%d]: error writing (%d)\n", t->tnum, ret);
		} else {
			strncpy(t->intn, myint, INT_LEN);
			printf("thread[%d]: value writen to %s: %s\n", t->tnum, SYSFSFILE, t->intn);
		}
	}

	close(fd);
	pthread_exit(NULL);
}

void help(void)
{
	fprintf(stderr,
		"llkdd  Copyright (C) 2014 Rafael do Nascimento Pereira\n"
		"intn device driver userspace test programm\n\n"
		"test_intn <thread_number>\n"
		"  <thread_number>:  concurrent threads accessing /dev/intn\n"
		"                    if not specified default to 4 threads.\n"
		"  -h                show this help message\n");
}

int main(int argc, const char *argv[])
{
	uint32_t nthreads = NUM_THREADS;
	uint32_t i;

	if (argc > 1 && argv[1] != NULL) {
		if (!strncmp(argv[1], opthelp, strlen(opthelp))) {
			help();
			return 0;
		} else if (atoi(argv[1]) > 0) {
			nthreads = (uint32_t)atoi(argv[1]);
			printf("Number of threads: %u\n", nthreads);
		} else {
			printf("invalid option. exiting..\n");
			return -1;
		}
	}

	uint32_t ret[nthreads];
	pthread_t threads[nthreads];
	struct tdata mydata[nthreads];

	for(i = 0; i < nthreads; i++){
		mydata[i].tnum = i,
		memset(mydata[i].intn, 0, INT_LEN);
		printf("Creating thread %u\n", i);
		ret[i] = pthread_create(&threads[i], NULL, inc_devintn, &mydata[i]);

		if (ret[i]){
			printf("ERROR: thread[%u] return code from pthread_create() is %u\n",
					i, ret[i]);
			//exit(-1);
		}
	}

	pthread_exit(NULL);
	return 0;
}
