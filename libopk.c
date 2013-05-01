
#include <ini.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Public interface.
#pragma GCC visibility push(default)
#include "opk.h"
#pragma GCC visibility pop

// Internal interfaces.
#include "unsqfs.h"

#define HEADER "Desktop Entry"

struct OPK {
	struct PkgData *pdata;
	void *buf;
	struct INI *ini;
};

struct OPK *opk_open(const char *opk_filename)
{
	struct OPK *opk = malloc(sizeof(*opk));
	if (!opk)
		return NULL;

	opk->pdata = opk_sqfs_open(opk_filename);
	if (!opk->pdata) {
		free(opk);
		return NULL;
	}

	opk->buf = NULL;

	return opk;
}

int opk_read_pair(struct OPK *opk,
		const char **key_chars, size_t *key_size,
		const char **val_chars, size_t *val_size)
{
	return ini_read_pair(opk->ini,
				key_chars, key_size, val_chars, val_size);
}

const char *opk_open_metadata(struct OPK *opk)
{
	/* Free previous meta-data information */
	if (opk->buf) {
		ini_close(opk->ini);
		free(opk->buf);
	}
	opk->buf = NULL;

	/* Get the name of the next .desktop */
	const char *name = opk_sqfs_get_metadata(opk->pdata);
	if (!name)
		return NULL;

	/* Extract the meta-data from the OD package */
	void *buf;
	size_t buf_size;
	if (opk_sqfs_extract_file(opk->pdata, name, &buf, &buf_size))
		return NULL;

	struct INI *ini = ini_open_mem(buf, buf_size);
	if (!ini)
		goto err_free_buf;

	const char *section;
	size_t section_len;
	int has_section = ini_next_section(ini, &section, &section_len);
	if (has_section < 0)
		goto error_ini_close;

	/* The [Desktop Entry] section must be the first section of the
	 * .desktop file. */
	if (!has_section || strncmp(HEADER, section, section_len)) {
		fprintf(stderr, "%s: not a proper desktop entry file\n", name);
		goto error_ini_close;
	}

	opk->buf = buf;
	opk->ini = ini;
	return name;

error_ini_close:
	ini_close(ini);
err_free_buf:
	free(buf);
	return NULL;
}

void opk_close(struct OPK *opk)
{
	opk_sqfs_close(opk->pdata);

	if (opk->buf) {
		ini_close(opk->ini);
		free(opk->buf);
	}
	free(opk);
}

int opk_extract_file(struct OPK *opk,
			const char *name, void **data, size_t *size)
{
	return opk_sqfs_extract_file(opk->pdata, name, data, size);
}
