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
 *  $Id: blob.c,v 1.1.1.1 2004/08/07 22:21:02 theno23 Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "lo_types_internal.h"
#include "lo/lo.h"

lo_blob lo_blob_new(int32_t size, void *data)
{
    lo_blob b;

    if (size < 1) {
	return NULL;
    }

    b = malloc(sizeof(size) + size);

    b->size = size;

    if (data) {
	memcpy(&(b->data), data, size);
    }

    return b;
}

void lo_blob_free(lo_blob b)
{
    free(b);
}

uint32_t lo_blob_datasize(lo_blob b)
{
    return b->size;
}

void *lo_blob_dataptr(lo_blob b)
{
    return &(b->data);
}

uint32_t lo_blobsize(lo_blob b)
{
    const int len = sizeof(uint32_t) + b->size;

    return 4 * (len / 4 + 1);
}

/* vi:set ts=8 sts=4 sw=4: */
