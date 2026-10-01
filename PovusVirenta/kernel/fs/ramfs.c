/* ============================================================
 * Povus:Virenta — ramfs.c
 * Простейшая ФС в ОЗУ поверх стандартных VFS-операций над
 * деревом vfs_node_t с RAM-буферами. Плюс «фирменные» файлы:
 * /tmp/motd.txt, /tmp/theme.hex (палитра Virenta).
 * ============================================================ */
#include "ramfs.h"
#include "lib/string.h"
#include "mm/kheap.h"
#include "drivers/screen/vga.h"

static vfs_node_t tmp_root;

static void add_file(vfs_node_t *dir, const char *name, const char *content) {
    vfs_node_t *n = kcalloc(1, sizeof(vfs_node_t));
    if (!n) return;
    strncpy(n->name, name, VFS_NAME_MAX - 1);
    n->size = (uint32_t)strlen(content);
    n->data = kmalloc(n->size + 1);
    memcpy(n->data, content, n->size);
    ((char *)n->data)[n->size] = 0;
    n->parent = dir;
    n->ops = 0;   /* дефолтные операции VFS работают с data */
    if (!dir->children) dir->children = n;
    else { vfs_node_t *t = dir->children; while (t->sibling) t = t->sibling; t->sibling = n; }
}

int ramfs_init(void) {
    memset(&tmp_root, 0, sizeof(tmp_root));
    strcpy(tmp_root.name, "/");
    tmp_root.flags = 1;   /* каталог */
    add_file(&tmp_root, "motd.txt",
        "Povus:Virenta 0.5 'Canopy'\n"
        "Verni cvet v svoi okna.\n"
        "\n"
        "Dobro pozhalovat' v zelyonuyu OS.\n"
        "Naberi 'help' chtoby nachat'.\n");
    add_file(&tmp_root, "theme.hex",
        "#7CFC00 #FFEB3B #00BFFF #FFFFFF\n"
        "#2E7D32 #FF9800 #FF6F61 #A5D6A7\n"
        "#00796B\n");
    add_file(&tmp_root, "hello.txt", "Hello from Virenta!\n");
    kprintf("ramfs: mounted /tmp (%d files)\n", 3);
    return vfs_mount("tmp", 0, &tmp_root);
}
