#include <include/vfs.h>
#include <include/error.h>
#include <include/string.h>
#include <include/slab.h>

static struct mount *rootfs = NULL;

int register_filesystem(struct filesystem *fs)
{
    if (!fs)
        return E_INVAL;

    // mount tmpfs to rootfs
    if (strcmp(fs->name, "tmpfs") == 0) {
        if (!rootfs) {
            rootfs = (struct mount *) kzalloc(sizeof(struct mount));
            fs->setup_mount(fs, rootfs);
        }
        return 0;
    }

    return E_INVAL;
}