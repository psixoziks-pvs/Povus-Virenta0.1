/* ============================================================
 * Povus:Virenta — vfs.c
 * Разрешение путей вида /fat/a/b.txt или /tmp/x
 * ============================================================ */
#include "vfs.h"
#include "lib/string.h"
#include "mm/kheap.h"
#include "drivers/screen/vga.h"

static vfs_mount_t mounts[VFS_MOUNT_MAX];

int vfs_init(void) {
    memset(mounts, 0, sizeof(mounts));
    return 0;
}

int vfs_mount(const char *path, const struct vfs_ops *ops, vfs_node_t *root) {
    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        if (!mounts[i].used) {
            strncpy(mounts[i].mount_point, path, VFS_NAME_MAX - 1);
            mounts[i].ops = ops;
            mounts[i].root = *root;
            mounts[i].used = true;
            kprintf("vfs: mounted %s\n", path);
            return 0;
        }
    }
    return -1;
}

/* Разобрать путь: вернуть индекс монтирования + остаток */
static int split_path(const char *path, int *mi, const char **rest) {
    if (path[0] != '/') return -1;
    const char *slash = strchr(path + 1, '/');
    int best = -1;
    for (int i = 0; i < VFS_MOUNT_MAX; i++) {
        if (!mounts[i].used) continue;
        size_t ml = strlen(mounts[i].mount_point);
        if (!strncmp(path + 1, mounts[i].mount_point, ml) &&
            (path[1 + ml] == '/' || path[1 + ml] == 0)) {
            if ((int)ml > best) { best = i; }
        }
    }
    if (best < 0) return -1;
    *mi = best;
    *rest = path + 1 + strlen(mounts[best].mount_point);
    if (**rest == '/') (*rest)++;
    return 0;
}

vfs_node_t *vfs_lookup(const char *path) {
    int mi; const char *rest;
    if (split_path(path, &mi, &rest) < 0) return 0;
    vfs_node_t *node = &mounts[mi].root;
    char comp[VFS_NAME_MAX];
    while (*rest) {
        while (*rest == '/') rest++;
        if (!*rest) break;
        const char *slash = strchr(rest, '/');
        size_t len = slash ? (size_t)(slash - rest) : strlen(rest);
        if (len >= VFS_NAME_MAX) return 0;
        memcpy(comp, rest, len); comp[len] = 0;
        if (!strcmp(comp, ".")) { rest += len; continue; }
        if (!strcmp(comp, "..")) { if (node->parent) node = node->parent; rest += len; continue; }
        vfs_node_t *found = 0;
        if (mounts[mi].ops->finddir) found = mounts[mi].ops->finddir(node, comp);
        else for (vfs_node_t *c = node->children; c; c = c->sibling)
            if (!strcmp(c->name, comp)) { found = c; break; }
        if (!found) return 0;
        node = found;
        rest += len;
    }
    return node;
}

vfs_node_t *vfs_open(const char *path, int flags) {
    (void)flags;
    return vfs_lookup(path);
}

int vfs_read(vfs_node_t *n, uint32_t off, void *buf, uint32_t len) {
    if (!n) return -1;
    if (n->ops && n->ops->read) return n->ops->read(n, off, buf, len);
    /* fallback: RAM-данные узла */
    if (!n->data || off >= n->size) return -1;
    if (off + len > n->size) len = n->size - off;
    memcpy(buf, n->data + off, len);
    return (int)len;
}

int vfs_write(vfs_node_t *n, uint32_t off, const void *buf, uint32_t len) {
    if (!n) return -1;
    if (n->ops && n->ops->write) return n->ops->write(n, off, buf, len);
    if (!n->data) return -1;
    if (off + len > n->size) {
        uint8_t *nd = kmalloc(off + len);
        if (!nd) return -1;
        memcpy(nd, n->data, n->size);
        memset(nd + n->size, 0, off + len - n->size);
        n->data = nd;
    }
    memcpy(n->data + off, buf, len);
    if (off + len > n->size) n->size = off + len;
    return (int)len;
}

int vfs_create(vfs_node_t *dir, const char *name, vfs_node_t **out) {
    if (!dir || !name) return -1;
    if (dir->ops && dir->ops->create) return dir->ops->create(dir, name, out);
    vfs_node_t *n = kcalloc(1, sizeof(vfs_node_t));
    if (!n) return -1;
    strncpy(n->name, name, VFS_NAME_MAX - 1);
    n->parent = dir;
    n->data = kmalloc(16);
    n->size = 0;
    n->ops = dir->ops;
    /* вшить в конец списка детей */
    if (!dir->children) dir->children = n;
    else { vfs_node_t *t = dir->children; while (t->sibling) t = t->sibling; t->sibling = n; }
    *out = n;
    return 0;
}

int vfs_remove(vfs_node_t *dir, const char *name) {
    if (!dir) return -1;
    if (dir->ops && dir->ops->remove) return dir->ops->remove(dir, name);
    vfs_node_t **pp = &dir->children;
    while (*pp) {
        if (!strcmp((*pp)->name, name)) {
            vfs_node_t *d = *pp;
            *pp = d->sibling;
            kfree(d->data);
            kfree(d);
            return 0;
        }
        pp = &(*pp)->sibling;
    }
    return -1;
}

int vfs_flush(vfs_node_t *n) {
    if (n && n->ops && n->ops->flush) return n->ops->flush(n);
    return 0;
}

/* «Настроение» файла — фирменная фишка VirentaFS: цвет по содержимому */
const char *vfs_resolve_mood(vfs_node_t *n) {
    if (!n || !n->data || !n->size) return "grey";
    uint32_t letters = 0, digits = 0, punct = 0;
    for (uint32_t i = 0; i < n->size && i < 512; i++) {
        char c = (char)n->data[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) letters++;
        else if (c >= '0' && c <= '9') digits++;
        else if (c == '.' || c == '!' || c == '?') punct++;
    }
    if (letters > digits * 4) return "green";   /* текст — растёт */
    if (digits > letters)     return "yellow";  /* числа — солнце данных */
    if (punct > 10)           return "blue";    /* много знаков — вода эмоций */
    return "mint";
}
