/*
 * Copyright (c) 2023 Tim van der Molen <tim@kariliq.nl>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

__dead void
usage(void)
{
	fprintf(stderr, "usage: %s file sql\n", getprogname());
	exit(1);
}

int
unveil_dirname(const char *path, const char *perms)
{
	char *dir, *tmp;

	if ((tmp = strdup(path)) == NULL) {
		warn(NULL);
		return -1;
	}

	if ((dir = dirname(tmp)) == NULL) {
		warnx("dirname() failed");
		free(tmp);
		return -1;
	}

	if (unveil(dir, perms) == -1) {
		warn("unveil: %s", dir);
		free(tmp);
		return -1;
	}

	free(tmp);
	return 0;
}

int
callback(__unused void *cookie, int n, char **res, __unused char **names)
{
	int i;

	for (i = 0; i < n; i++) {
		if (res[i] != NULL)
			fputs(res[i], stdout);
		putchar(i + 1 < n ? '|' : '\n');
	}

	return 0;
}

int
main(int argc, char **argv)
{
	sqlite3 *db;
	char *errmsg;

	if (argc != 3)
		usage();

	if (unveil_dirname(argv[1], "rwc") == -1)
		return 1;

	if (unveil("/dev/urandom", "r") == -1)
		err(1, "unveil: /dev/urandom");

	if (unveil("/tmp", "rwc") == -1)
		err(1, "unveil: /tmp");

	if (pledge("stdio rpath wpath cpath flock", NULL) == -1)
		err(1, "pledge");

	if (sqlite3_open(argv[1], &db) != SQLITE_OK) {
		warnx("%s: %s", argv[1], sqlite3_errmsg(db));
		goto err;
	}

	if (sqlite3_db_config(db, SQLITE_DBCONFIG_DEFENSIVE, 0, NULL) !=
	    SQLITE_OK) {
		warnx("%s", sqlite3_errmsg(db));
		goto err;
	}

	if (sqlite3_db_config(db, SQLITE_DBCONFIG_WRITABLE_SCHEMA, 1, NULL) !=
	    SQLITE_OK) {
		warnx("%s", sqlite3_errmsg(db));
		goto err;
	}

	if (sqlite3_exec(db, argv[2], callback, NULL, &errmsg) != SQLITE_OK) {
		warnx("%s", errmsg);
		sqlite3_free(errmsg);
		goto err;
	}

	if (sqlite3_close(db) != SQLITE_OK) {
		warnx("%s", sqlite3_errmsg(db));
		return 1;
	}

	return 0;

err:
	sqlite3_close(db);
	return 1;
}
