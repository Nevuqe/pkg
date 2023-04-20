/*-
 * Copyright (c) 2020-2023 Baptiste Daroussin <bapt@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <sys/param.h>

#include <stdio.h>
#include <fetch.h>
#include <errno.h>

#include "pkg.h"
#include "private/pkg.h"
#include "private/event.h"
#include "private/utils.h"

int
file_open(struct pkg_repo *repo, struct url *u, off_t *sz)
{
	struct stat st;

	if (stat(u->doc, &st) == -1) {
		if (!repo->silent)
			pkg_emit_error("%s://%s%s%s%s: %s",
			    u->scheme,
			    u->user,
			    u->user[0] != '\0' ? "@" : "",
			    u->host,
			    u->doc,
			    strerror(errno));
		return (EPKG_FATAL);
	}
	*sz = st.st_size;
	u->ims_time = st.st_mtime;

	repo->fh = fopen(u->doc, "re");
	if (repo->fh == NULL)
		return (EPKG_FATAL);
	return (EPKG_OK);
}

void
fh_close(struct pkg_repo *repo)
{
	if (repo->fh != NULL)
		fclose(repo->fh);
	repo->fh = NULL;
}
