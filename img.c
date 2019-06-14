/*
 * This is a wrapper file for libarhcive.
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the
 * file "LICENSE.txt".
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <archive_entry.h>
#include <archive.h>

#include "img.h"

// These variables must be initialized if image is in a different directory than this program
#define DEFAULT_BASE ""
#define DEFAULT_IMAGES ""
static char * img_path  = ".";
static char * base_path = ".";

char * get_base_cfg_path (void)
{
    if (base_path) {
        return base_path;
    } else {
        struct passwd * pw = getpwuid(getuid());
        const char * homedir = pw->pw_dir;
        base_path = calloc(PATH_MAX, 1);
        snprintf(base_path, PATH_MAX, "%s/" DEFAULT_BASE, homedir);
    }

    return base_path;
}


char * get_img_path (void)
{
    if (img_path) {
        return img_path;
    } else {
        struct passwd * pw = getpwuid(getuid());
        const char * homedir = pw->pw_dir;
        img_path = calloc(PATH_MAX, 1);
        snprintf(img_path, PATH_MAX, "%s/" DEFAULT_BASE "/" DEFAULT_IMAGES, homedir);
    }

    return img_path;
}

static int copy_data (struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	__LA_INT64_T offset;

	for (;;) {

		r = archive_read_data_block(ar, &buff, &size, &offset);

		if (r == ARCHIVE_EOF)
			return ARCHIVE_OK;

		if (r < ARCHIVE_OK)
			return r;

		r = archive_write_data_block(aw, buff, size, offset);

		if (r < ARCHIVE_OK) {
			fprintf(stderr, "%s\n", archive_error_string(aw));
			return r;
		}
	}

	return 0;
}

int img_extract (char * img)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int flags;
	int r;
    char path[PATH_MAX];

    snprintf(path, PATH_MAX, "%s/%s", get_img_path(), img);
    printf("img path:%s\n",get_img_path());

	/* Select which attributes we want to restore. */
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

    printf("Extracting image '%s'\n", img);

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_compression_all(a);
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);

	if ((r = archive_read_open_filename(a, path, 10240))) {
		fprintf(stderr, "Could not open archive file (%s): %s\n", path, strerror(errno));
        return -1;
	}

	for (;;) {
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF)
			break;

        //printf("%s\n", archive_entry_pathname(entry));

		if (r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(a));

		if (r < ARCHIVE_WARN) {
            fprintf(stderr, "Warning: %s\n", archive_error_string(a));
			return -1;
        }

        char newpath[PATH_MAX];
        snprintf(newpath, PATH_MAX, "%s/%s", get_img_path(), archive_entry_pathname(entry));
        archive_entry_set_pathname(entry, newpath);

		r = archive_write_header(ext, entry);

		if (r < ARCHIVE_OK) {
			fprintf(stderr, "%s\n", archive_error_string(ext));
        } else if (archive_entry_size(entry) > 0) {

			r = copy_data(a, ext);

			if (r < ARCHIVE_OK) {
				fprintf(stderr, "%s\n", archive_error_string(ext));
            }

			if (r < ARCHIVE_WARN) {
                fprintf(stderr, "Warning: %s\n", archive_error_string(ext));
				return -1;
            }
		}

		r = archive_write_finish_entry(ext);

		if (r < ARCHIVE_OK) {
			fprintf(stderr, "%s\n", archive_error_string(ext));
        }

		if (r < ARCHIVE_WARN) {
			fprintf(stderr, "Warning: %s\n", archive_error_string(ext));
			return -1;
        }

	}

	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);

    return 0;
}


/* int main (int argc, char **argv) */
/* { */
/*    base_path = argv[1]; */
/*    img_path = argv[2]; */
/*    img_extract("rootfs.tar"); */
/* } */
