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
 *  $Id: bundle.c,v 1.4 2005/01/19 07:59:50 theno23 Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lo_types_internal.h"
#include "lo/lo.h"

lo_bundle lo_bundle_new(lo_timetag tt)
{
    lo_bundle b = calloc(1, sizeof(struct _lo_bundle));

    b->size = 4;
    b->len = 0;
    b->ts = tt;
    b->msgs = calloc(b->size, sizeof(lo_message));
    b->paths = calloc(b->size, sizeof(char *));

    return b;
}

void lo_bundle_add_message(lo_bundle b, const char *path, lo_message m)
{
    if (!m) {
	return;
    }

    if (b->len >= b->size) {
	b->size *= 2;
	b->msgs = realloc(b->msgs, b->size * sizeof(lo_message));
	b->paths = realloc(b->paths, b->size * sizeof(char *));
    }

    b->msgs[b->len] = m;
    b->paths[b->len] = (char *)path;

    (b->len)++;
}

size_t lo_bundle_length(lo_bundle b)
{
    size_t size = 16; /* "#bundle" and the timetag */
    int i;

    if (!b) {
	return 0;
    }

    size += b->len * 4; /* sizes */
    for (i = 0; i < b->len; i++) {
	size += lo_message_length(b->msgs[i], b->paths[i]);
    }

    return size;
}

void *lo_bundle_serialise(lo_bundle b, void *to, size_t *size)
{
    size_t s, skip;
    int32_t *bes;
    int i;
    char *pos;
    lo_pcast64 be;

    if (!b) {
	return NULL;
    }

    s = lo_bundle_length(b);
    if (size) {
	*size = s;
    }

    if (!to) {
	to = calloc(1, s);
    }

    pos = to;
    strcpy(pos, "#bundle");
    pos += 8;

    be.tt = b->ts;
    be.nl = lo_htoo64(be.nl);
    memcpy(pos, &be, 8);
    pos += 8;

    for (i = 0; i < b->len; i++) {
	lo_message_serialise(b->msgs[i], b->paths[i], pos + 4, &skip);
	bes = (uint32_t *)pos;
	*bes = lo_htoo32(skip);
	pos += skip + 4;

	if (pos > (char *)to + s) {
	    fprintf(stderr, "liblo: data integrity error at message %d\n", i);

	    return NULL;
	}
    }
    if (pos != to + s) {
	fprintf(stderr, "liblo: data integrity error\n");

	return NULL;
    }

    return to;
}

void lo_bundle_free(lo_bundle b)
{
    if (!b) {
	return;
    }

    free(b->msgs);
    free(b->paths);
    free(b);
}

void lo_bundle_pp(lo_bundle b)
{
    int i;

    if (!b) return;

    printf("bundle(%f):\n", (double)b->ts.sec + b->ts.frac / 4294967296.0);
    for (i = 0; i < b->len; i++) {
	lo_message_pp(b->msgs[i]);
    }
    printf("\n");
}

/* vi:set ts=8 sts=4 sw=4: */
