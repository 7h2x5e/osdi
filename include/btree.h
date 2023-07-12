#ifndef BTREE
#define BTREE
#include <include/list.h>
#include <include/types.h>

enum node_type { BTREE_EXTERNAL_NODE, BTREE_INTERNAL_NODE };

typedef struct _b_key b_key;
typedef struct _b_node b_node;
typedef struct _btree btree;

struct _b_key {
    struct list_head key_h;   /* list of key */
    struct list_head b_key_h; /* list of all keys */
    uint64_t start, end;
    void *entry; /* point page's virtual address. if page wasn't allocated, it's
                    NULL */
    b_node *container;        /* node contain the key */
    b_node *c_left, *c_right; /* point to child */
    uint64_t f_addr;          /* file content copied to memory region */
    size_t f_size;            /* file content size */
    uint32_t prot;            /* page protection bit */
};

struct _b_node {
    enum node_type type;
    struct list_head key_h;
    uint8_t count; /* number of key */
    uint64_t max_gap, min, max;
    b_key *p_left, *p_right; /* point to parent */
};

struct _btree {
    b_node *root;             /* maintain VMAs */
    struct list_head b_key_h; /* list of B-Tree keys */
    uint64_t min, max;
};

bool is_minimum(b_key *);
bool is_maximum(b_key *);
void bt_init(btree *);
void bt_destroy(btree *);
b_key *bt_find_key(b_node *node, uint64_t start);
b_key *bt_find_empty_area(b_node *node,
                          uint64_t min,
                          uint64_t max,
                          uint64_t size,
                          uint64_t *start);
uint32_t bt_insert_range(b_node **root,
                         uint64_t start,
                         uint64_t end,
                         uint64_t size,
                         void *entry,
                         uint64_t f_addr,
                         size_t f_size,
                         uint32_t prot);

#endif
