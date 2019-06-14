#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "img.h"
#define TRY(x) if (x) fatal_errno(__LINE__)

void fatal_errno(int line)
{
   printf("error at line %d, errno=%d\n", line, errno);
   exit(1);
}

int my_pivot_root(void)
{

   int ret;

   /* Ensure that our image directory exists. It doesn't really matter what's in it. */
   if (mkdir("/home/lukerobbins2112/Project/newroot", 0755) && errno != EEXIST)
      TRY (errno);

   if (rename("./rootfs.tar", "/home/lukerobbins2112/Project/newroot/rootfs.tar") && errno != EEXIST)
	TRY (errno);

   /* Enter the mount and user namespaces. Note that in some cases (e.g., RHEL
      6.8), this will succeed even though the userns is not created. In that
      case, the following mount(2) will fail with EPERM. */
   // TRY (unshare(CLONE_NEWNS|CLONE_NEWUSER));

   /* Claim the image for our namespace by recursively bind-mounting it over
      itself. This standard trick avoids conditions 1 and 2. */
   TRY (mount("/home/lukerobbins2112/Project/newroot", "/home/lukerobbins2112/Project/newroot", NULL,
              MS_REC | MS_BIND | MS_PRIVATE, NULL));

   /* The next few calls deal with condition 3. The solution is to overmount
      the root filesystem with literally anything else. We use the parent of
      the image, /tmp. This doesn't hurt if / is not a rootfs, so we always do
      it for simplicity. */

   /* Claim /tmp for our namespace. You would think that because /tmp contains
      /tmp/newroot and it's a recursive bind mount, we could claim both in the
      same call. But, this causes pivot_root(2) to fail later with EBUSY. */

   TRY (mount("/home/lukerobbins2112/Project", "/home/lukerobbins2112/Project", NULL, MS_REC | MS_BIND | MS_PRIVATE, NULL));

   /* chdir to /tmp. This moves the process' special "." pointer to
      the soon-to-be root filesystem. Otherwise, it will keep pointing to the
      overmounted root. See the e-mail at the end of:
      https://git.busybox.net/busybox/tree/util-linux/switch_root.c?h=1_24_2 */

   TRY (chdir("/home/lukerobbins2112/Project/newroot"));

   ret = img_extract("rootfs.tar");
   printf("ret:%d\n",ret);


   TRY (chdir("/home/lukerobbins2112/Project/"));

   /* Move /tmp to /. (One could use this to directly enter the image,
      avoiding pivot_root(2) altogether. However, there are ways to remove all
      active references to the root filesystem. Then, the image could be
      unmounted, exposing the old root filesystem underneath. While
      Charliecloud does not claim a strong isolation boundary, we do want to
      make activating the UDSS irreversible.) */
   TRY (mount("/home/lukerobbins2112/Project", "/", NULL, MS_MOVE, NULL));

   /* Move the "/" special pointer to the new root filesystem, for the reasons
      above. (Similar reasoning applies for why we don't use chroot(2) to
      directly activate the UDSS.) */
   TRY (chroot("."));

   /* Make a place for the old (intermediate) root filesystem to land. */
   if (mkdir("/newroot/oldroot", 0755) && errno != EEXIST)
      TRY (errno);

   /* Re-mount the image read-only. */
   //TRY (mount(NULL, "/newroot", NULL, MS_REMOUNT | MS_BIND | MS_RDONLY, NULL));
   TRY (mount(NULL, "/newroot", NULL, MS_REMOUNT | MS_BIND , NULL));

   /* Finally, make our "real" newroot into the root filesystem. */
   TRY (chdir("/newroot"));
   TRY (syscall(SYS_pivot_root, "/newroot", "/newroot/oldroot"));
   TRY (chroot("."));

   /* Unmount the old filesystem and it's gone for good. */
   TRY (umount2("/oldroot", MNT_DETACH));

   /* Report success. */
   printf("ok\n");


   return 0;
}

