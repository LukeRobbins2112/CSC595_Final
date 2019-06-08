
#ifndef _IMG_H_
#define _IMG_H_

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

char * hkr_get_base_cfg_path (void);
char * hkr_get_img_path (void);
char * hkr_get_img (char * img);
void hkr_clear_cfg (void);
static int copy_data (struct archive *ar, struct archive *aw);
int hkr_img_exists (char * img);
int hkr_img_extract (char * img);



#endif
