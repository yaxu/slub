/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: server_thread.c,v 1.2 2004/11/17 17:34:46 theno23 Exp $
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "lo_types_internal.h"
#include "lo/lo.h"
#include "lo/lo_throw.h"

static void thread_func(void *data);

lo_server_thread lo_server_thread_new(const char *port, lo_err_handler err_h)
{
    lo_server_thread st = malloc(sizeof(struct _lo_server_thread));
    st->s = lo_server_new(port, err_h);
    st->active = 0;
    st->done = 0;

    if (!st->s) {
	free(st);

	return NULL;
    }

    return st;
}

void lo_server_thread_free(lo_server_thread st)
{
    if (st) {
	if (st->active) {
	    lo_server_thread_stop(st);
	    /* pthread_cancel(st->thread); */
	}
	lo_server_free(st->s);
    }
    free(st);
}

lo_method lo_server_thread_add_method(lo_server_thread st, const char *path,
			       const char *typespec, lo_method_handler h,
			       void *user_data)
{
    return lo_server_add_method(st->s, path, typespec, h, user_data);
}

void lo_server_thread_start(lo_server_thread st)
{
    if (!st->active) {
	st->active = 1;
	st->done = 0;
	pthread_create(&(st->thread), NULL, (void *)&thread_func, st);
    }
}

void lo_server_thread_stop(lo_server_thread st)
{
    if (st->active) {
	st->active = 0;
	while (!st->done) {
	    usleep(1000);
	}
    }
}

int lo_server_thread_get_port(lo_server_thread st)
{
    return lo_server_get_port(st->s);
}

char *lo_server_thread_get_url(lo_server_thread st) 
{
    return lo_server_get_url(st->s);
}

int lo_server_thread_events_pending(lo_server_thread st)
{
    return lo_server_events_pending(st->s);
}

static void thread_func(void *data)
{
    lo_server_thread st = (lo_server_thread)data;

    while (st->active) {
	lo_server_recv_noblock(st->s, 10);
    }
    st->done = 1;
}

void lo_server_thread_pp(lo_server_thread st)
{
    lo_server_pp(st->s);
}

/* vi:set ts=8 sts=4 sw=4: */
