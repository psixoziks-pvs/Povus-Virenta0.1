/* ============================================================
 * Povus:Virenta — VFS (Virtual File System)
 * Единый интерфейс: mount-таблица, inode-операции, fd-таблица.
 * Драйверы ФС: FAT32 (диск), RAMFS (tmpdisk), VFS-уровень общий.
 * ============================================================ */
#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VFS_NAME_MAX   128
#define VFS_OPEN_MAX   32
#define VFS_MOUNT_MAX  8

typedef struct vfs_node vfs_node_t;
struct vfs_node {
    char     name[VFS_NAME_MAX];
    uint32_t size;
    uint32_t flags;          /* тип/права */
    time_dummy_t mtime;      /* «настроение» файла — см. VirentaFS */
    vfs_node_t *parent;
    vfs_node_t *children;    /* список для каталогов */
    vfs_node_t *sibling;
    /* данные для ramfs/fat */
    uint8_t *data;
    uint32_t start_sector;   /* fat32 */
    uint32_t cluster;        /* fat32 */
    const struct vfs_ops *ops;
    void *priv;
};

typedef struct { uint32_t sec; } time_dummy_t;

struct vfs_ops {
    int   (*read)(vfs_node_t *n, uint32_t off, uint8_t *buf, uint32_t len);
    int   (*write)(vfs_node_t *n, uint32_t off, const uint8_t *buf, uint32_t len);
    int   (*create)(vfs_node_t *dir, const char *name, vfs_node_t **out);
    int   (*remove)(vfs_node_t *dir, const char *name);
    vfs_node_t *(*finddir)(vfs_node_t *dir, const char *name);
    int   (*flush)(vfs_node_t *n);
};

typedef struct {
    char mount_point[VFS_NAME_MAX];
    vfs_node_t root;
    const struct vfs_ops *ops;
    bool used;
} vfs_mount_t;

int  vfs_init(void);
int  vfs_mount(const char *path, const struct vfs_ops *ops, vfs_node_t *root);
int  vfs_unmount(const char *path);
vfs_node_t *vfs_open(const char *path, int flags);
int  vfs_read(vfs_node_t *n, uint32_t off, void *buf, uint32_t len);
int  vfs_write(vfs_node_t *n, uint32_t off, const void *buf, uint32_t len);
int  vfs_create(vfs_node_t *dir, const char *name, vfs_node_t **out);
int  vfs_remove(vfs_node_t *dir, const char *name);
vfs_node_t *vfs_lookup(const char *path);
int  vfs_flush(vfs_node_t *n);
const char *vfs_resolve_mood(vfs_node_t *n);   /* «настроение» для VirentaFS */

#endif
