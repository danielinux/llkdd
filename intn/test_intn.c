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
 * This programs tests the intn (/dev/intn) driver. It starts 2*N threads,
 * where every 2*i-th thread reads, and the (2*i) + 1-th thread writes
 * (increments intn by one). It is expected in the end, if mutual exclusion
 * is active, to have as a final result in intn the initial value (25) + N.
 * Otherwise is not possible foresee the final result in intn.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define NUM_THREADS 4
#define INT_LEN    13
#define DEVFILE    "/dev/intn"

struct tdata {
	uint32_t tnum;
	char  intn[INT_LEN];
};

void *read_devintn(void *data)
{
	struct tdata *t;
	int fd;
	char myint[INT_LEN];

	if (data == NULL)
		return NULL;

	t = data;
	fd = open(DEVFILE, O_RDONLY);

	if (fd == -1)
		return NULL;

	memset(myint, 0, INT_LEN);

	if (read(fd, myint, INT_LEN) < 0) {
		printf("thread[%d]: error reading from %s\n", t->tnum, DEVFILE);
	} else {
		strncpy(t->intn, myint, INT_LEN);
		printf("thread[%d]: value read from %s: %s\n", t->tnum, DEVFILE, t->intn);
	}

	close(fd);
	pthread_exit(NULL);
}

void *write_devintn(void *data)
{
	struct tdata *t;
	int fd, ret;
	char myint[INT_LEN];

	if (data == NULL)
		return NULL;

	t = data;
	fd = open(DEVFILE, O_RDWR);

	if (fd == -1)
		return NULL;

	memset(myint, 0, INT_LEN);

	if (read(fd, myint, INT_LEN) < 0) {
		printf("thread[%d]: error reading from %s\n", t->tnum, DEVFILE);
	} else {
		strncpy(t->intn, myint, INT_LEN);
		printf("thread[%d]: value read from %s: %s\n", t->tnum, DEVFILE, t->intn);
	}

	int tmp = (int)strtol(myint, NULL, 10);
	tmp++;
	memset(myint, 0, INT_LEN);

	if ((ret = snprintf(myint, INT_LEN, "%d", tmp)) < 0) {
		printf("thread[%d]: error converting integer (%d)\n", t->tnum, ret);
	} else {
		if ((ret = write(fd, myint, INT_LEN)) < 0) {
			printf("thread[%d]: error writing (%d)\n", t->tnum, ret);
		}
	}

	close(fd);
	pthread_exit(NULL);
}

int main(void)
{
	int32_t ret[NUM_THREADS];
	pthread_t threads[NUM_THREADS];
	int rc;
	long i;
	struct tdata mydata[NUM_THREADS];

	for(i = 0; i < NUM_THREADS; i++){
		mydata[i].tnum = i,
		memset(mydata[i].intn, 0, INT_LEN);
		printf("In main: creating thread %ld\n", i);

		if (i%2 != 0)
			rc = pthread_create(&threads[i], NULL, write_devintn, &mydata[i]);
		else
			rc = pthread_create(&threads[i], NULL, read_devintn, &mydata[i]);

		if (rc){
			printf("ERROR: thread[%ld] return code from pthread_create() is %d\n",
					i, rc);
			exit(-1);
		}
	}

	pthread_exit(NULL);

	for(i = 0; i < NUM_THREADS; i++){
		printf("(Main Thread)thread[%d]: value read from %s: %s\n", mydata[i].tnum,
				DEVFILE, mydata[i].intn);
	}
	return 0;
}
