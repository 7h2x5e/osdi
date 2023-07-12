#include <include/btree.h>
#include <include/types.h>
#include <include/mm.h>
#include <include/error.h>
#include <include/assert.h>
#include <include/kernel_log.h>

#define malloc(x) btree_page_malloc(x)
#define free(x) btree_page_free(x)
#define KEY_MAX 4 /* must >= 2 */

bool is_minimum(b_key *key)
{
    if (key && key->start == key->end && (key->entry == 0)) {
        return true;
    }
    return false;
}

bool is_maximum(b_key *key)
{
    if (key && key->start == key->end && ((uint64_t) key->entry == 1)) {
        return true;
    }
    return false;
}

static b_key *allocate_key(uint64_t start,
                           uint64_t end,
                           void *entry,
                           uint64_t f_addr,
                           size_t f_size,
                           uint32_t prot)
{
    b_key *key = malloc(sizeof(b_key));
    key->c_left = key->c_right = NULL;
    key->start = start;
    key->end = end;
    key->entry = entry;
    key->container = NULL;
    key->f_addr = f_addr;
    key->f_size = f_size;
    key->prot = prot;
    INIT_LIST_HEAD(&key->key_h);
    INIT_LIST_HEAD(&key->b_key_h);
    return key;
}

static b_node *allocate_node(enum node_type type)
{
    b_node *node = malloc(sizeof(b_node));
    node->count = 0;
    INIT_LIST_HEAD(&node->key_h);
    node->p_left = node->p_right = NULL;
    node->type = type;
    node->max_gap = node->min = node->max = 0;
    return node;
}

static void free_key(b_key *key)
{
    /* do not free entry */
    // if (key->entry && !is_minimum(key) && !is_maximum(key))
    //     free(key->entry);
    list_del(&key->b_key_h);
    list_del(&key->key_h);
    key->container->count--;
    free(key);
}

static void free_node(b_node *node)
{
    if (node->count > 0) {
        b_key *key, *tmp;
        if (node->type == BTREE_INTERNAL_NODE) {
            tmp = list_first_entry(&node->key_h, b_key, key_h);
            free_node(tmp->c_left);
            list_for_each_entry_safe(key, tmp, &node->key_h, key_h)
            {
                free_node(key->c_right);
                free_key(key);
            }
        } else if (node->type == BTREE_EXTERNAL_NODE) {
            list_for_each_entry_safe(key, tmp, &node->key_h, key_h)
            {
                free_key(key);
            }
        }
        free(node);
    }
}

/* return previous key of the node */
static b_key *leftmost(b_node *node)
{
    if (!node || !node->count)
        return NULL;

    b_key *left = list_entry(node->key_h.next, b_key, key_h);
    if (is_minimum(left)) {
        return left;
    }

    return node->p_left ? node->p_left : leftmost(node->p_right->container);
}

/* return next key of the node */
static b_key *rightmost(b_node *node)
{
    if (!node || !node->count)
        return NULL;

    b_key *right = list_entry(node->key_h.prev, b_key, key_h);
    if (is_maximum(right)) {
        return right;
    }

    return node->p_right ? node->p_right : rightmost(node->p_left->container);
}

/* return lower bound of node */
static uint64_t get_min(b_node *node)
{
    if (node->type == BTREE_INTERNAL_NODE) {
        return get_min(list_entry(node->key_h.next, b_key, key_h)->c_left);
    }
    return leftmost(node)->end;
}

/* return upper bound of node */
static uint64_t get_max(b_node *node)
{
    if (node->type == BTREE_INTERNAL_NODE) {
        return get_max(list_entry(node->key_h.prev, b_key, key_h)->c_right);
    }
    return rightmost(node)->start;
}

/* return max gap of node. update node if its keys/childs changes.
   we assume node is valid */
static uint64_t get_max_gap(b_node *node)
{
    uint64_t max_gap = 0;
    b_key *key, *first_key = list_entry(node->key_h.next, b_key, key_h),
                *next_key;

    if (node->type == BTREE_INTERNAL_NODE) {
        max_gap = first_key->c_left->max_gap;
        list_for_each_entry(key, &node->key_h, key_h)
        {
            max_gap = MAX(max_gap, key->c_right->max_gap);
        }
    } else if (node->type == BTREE_EXTERNAL_NODE) {
        max_gap = first_key->start - leftmost(node)->end;
        list_for_each_entry(key, &node->key_h, key_h)
        {
            // if (is_maximum(key))
            //   break;
            next_key = list_is_last(&key->key_h, &node->key_h)
                           ? rightmost(node)
                           : list_entry(key->key_h.next, b_key, key_h);
            max_gap = MAX(max_gap, next_key->start - key->end);
        }
    }
    return max_gap;
}

static b_node *get_parent(b_node *node)
{
    if (!node)
        return NULL;
    if (node->p_left || node->p_right)
        return node->p_left ? node->p_left->container
                            : node->p_right->container;
    return node;  // root
}

static uint64_t __attribute__((unused)) get_depth(b_node *node)
{
    /*
     *  depth
     *    0       [root]
     *           /      \
     *    1 [node]     [node]
     */
    uint64_t depth;
    for (depth = 0; node != get_parent(node); depth++, node = get_parent(node))
        ;
    return depth;
}

static void update(b_node *node)
{
    node->min = get_min(node);
    node->max = get_max(node);
    node->max_gap = get_max_gap(node);
}

static void update_until_root(b_node *node)
{
    b_node *parent = get_parent(node);
    update(node);
    if (node != parent) {
        update_until_root(parent);
    }
}

static bool right_rotation(b_node *node)
{
    if (!node || node->type != BTREE_EXTERNAL_NODE || !node->p_right)
        return false;

    /* ensure sibling of the current node has enough empty slot */
    b_node *right_sibling = node->p_right->c_right;
    if (right_sibling->count >= KEY_MAX)
        return false;

    /*   b
     *  / \
     * a   c
     */
    b_key *c = list_first_entry(&right_sibling->key_h, b_key, key_h),
          *b = right_sibling->p_left,
          *a = list_entry(node->key_h.prev, b_key, key_h);

    list_del(&a->key_h);
    list_add(&a->key_h, &b->key_h);
    list_del(&b->key_h);
    list_add_tail(&b->key_h, &c->key_h);

    // key
    a->c_left = b->c_left;
    a->c_right = b->c_right;
    a->container = b->container;
    b->container = c->container;
    b->c_left = b->c_right = NULL;

    // node
    node->p_right = a;
    right_sibling->p_left = a;
    node->count--;
    right_sibling->count++;

    update(right_sibling);
    update_until_root(node);

    return true;
}

static bool left_rotation(b_node *node)
{
    if (!node || node->type != BTREE_EXTERNAL_NODE || !node->p_left)
        return false;

    /* ensure sibling of the current node has enough empty slot */
    b_node *left_sibling = node->p_left->c_left;
    if (left_sibling->count >= KEY_MAX)
        return false;

    /*   b
     *  / \
     * c   a
     */
    b_key *a = list_first_entry(&node->key_h, b_key, key_h), *b = node->p_left,
          *c = list_entry(left_sibling->key_h.prev, b_key, key_h);

    list_del(&a->key_h);
    list_add(&a->key_h, &b->key_h);
    list_del(&b->key_h);
    list_add(&b->key_h, &c->key_h);

    // key
    a->c_left = b->c_left;
    a->c_right = b->c_right;
    a->container = b->container;
    b->container = c->container;
    b->c_left = b->c_right = NULL;

    // node
    node->p_left = a;
    left_sibling->p_right = a;
    node->count--;
    left_sibling->count++;

    update(left_sibling);
    update_until_root(node);

    return true;
}

static void split(b_node *node, b_node **root)
{
    if (!*root || !node)
        return;

    /* only update node's metadata */
    if (node->count <= KEY_MAX) {
        update_until_root(node);
        return;
    }

    /* Use slow/fast pointer to find the middle key

      [key_h]-[key 1]-[key 2]-[key 3]-[key 4]-[key 5]
            slow,fast

      [key_h]-[key 1]-[key 2]-[key 3]-[key 4]-[key 5]
                      slow   fast

      [key_h]-[key 1]-[key 2]-[key 3]-[key 4]-[key 5]
                              slow            fast
    */
    struct list_head *slow, *fast;
    for (slow = fast = node->key_h.next;
         !list_is_last(fast, &node->key_h) &&
         !list_is_last(fast->next, &node->key_h);
         fast = fast->next->next, slow = slow->next)
        ;

    /*
       left middle right
        / \   / \   / \

       =>

          middle
         /      \
     ...left     right...
         \      /
     */
    b_key *middle = list_entry(slow, b_key, key_h),
          *left = list_entry(middle->key_h.prev, b_key, key_h),
          *right = list_entry(middle->key_h.next, b_key, key_h);
    b_node *new_node = allocate_node(node->type),
           *parent_node = get_parent(node);

    /* create new root */
    if (parent_node == node) {
        *root = parent_node = allocate_node(BTREE_INTERNAL_NODE);
    }

    /* move right list to new node */
    b_key *tmp, *pos = right;
    list_for_each_entry_safe_from(pos, tmp, &node->key_h, key_h)
    {
        list_del(&pos->key_h);
        list_add(&pos->key_h, new_node->key_h.prev);
        pos->container = new_node;
        new_node->count++;
        node->count--;
    }

    /* move middle into parent node */
    list_del(&middle->key_h);
    if (node->p_left) {
        list_add(&middle->key_h, &node->p_left->key_h);
    } else if (node->p_right) {
        list_add_tail(&middle->key_h, &node->p_right->key_h);
    } else {
        list_add(&middle->key_h, &parent_node->key_h);
    }
    middle->container = parent_node;
    parent_node->count++;
    node->count--;

    /* update left/right's child */
    if (left->c_right)
        left->c_right->p_right = NULL;
    if (right->c_left)
        right->c_left->p_left = NULL;

    /* relink left/middle/right

        X     middle       Y
          \   /      \    /
        ...left      right...
     */
    if (node->p_right) {
        /* node isn't root */
        new_node->p_right = node->p_right;
        new_node->p_right->c_left = new_node;
    }
    new_node->p_left = middle;
    middle->c_right = new_node;
    node->p_right = middle;
    middle->c_left = node;

    /* update metadata */
    update(left->container);
    update(right->container);
    update(middle->container);

    /* recursively split */
    split(parent_node, root);
}

static void init_btree(btree *bt, uint64_t min, uint64_t max)
{
    b_node *root = allocate_node(BTREE_EXTERNAL_NODE);
    root->min = min;
    root->max = max;

    /* insert dummy nodes for available range [min, max) */
    b_key *min_key = allocate_key(min, min, (void *) 0, 0, 0,
                                  0), /* entry = 0/1 stands for min/max */
        *max_key = allocate_key(max, max, (void *) 1, 0, 0, 0);
    min_key->container = root;
    max_key->container = root;
    list_add_tail(&min_key->key_h, &root->key_h);
    list_add_tail(&max_key->key_h, &root->key_h);
    root->count = 2;

    /* update lower and upper bound of the node */
    root->min = get_min(root);
    root->max = get_max(root);
    root->max_gap = get_max_gap(root);

    INIT_LIST_HEAD(&bt->b_key_h);
    list_add_tail(&min_key->b_key_h, &bt->b_key_h);
    list_add_tail(&max_key->b_key_h, &bt->b_key_h);

    bt->root = root;
}

static void destroy_btree(b_node *root)
{
    free_node(root);
}

/* find a available area in node, return the last found key to insert
 * vma before or after it, otherwise return NULL */
b_key *bt_find_empty_area(b_node *node,
                          uint64_t min,
                          uint64_t max,
                          uint64_t size,
                          uint64_t *start)
{
    if (!node || !node->count)
        return NULL;

    if (!(node->min <= min && max <= node->max)) {
        return NULL;
    }

    if (node->max_gap < size)
        return NULL;

    if (node->type == BTREE_EXTERNAL_NODE) {
        b_key *prev, *cur, *next;

        /* we assume min+size <= max */
        {
            /* consider previous key */
            prev = leftmost(node);
            cur = list_first_entry(&node->key_h, b_key, key_h);

            if (!is_minimum(cur)) {
                /* area = [min, min+size) */
                if (prev->end <= min && (min + size <= cur->start)) {
                    *start = min;
                    return cur;
                }

                /* area = [prev->end, prev->end + size) */
                if (prev->end > min && (prev->end + size <= max) &&
                    (prev->end + size <= cur->start)) {
                    *start = prev->end;
                    return cur;
                }
            }
        }

        /* iterate all keys of the node */
        list_for_each_entry(cur, &node->key_h, key_h)
        {
            next = list_entry(cur->key_h.next, b_key, key_h);
            /* area = [min, min+size) */
            if (cur->end <= min && (min + size <= next->start)) {
                *start = min;
                return cur;
            }

            /* area = [cur->end, cur->end + size) */
            if (cur->end > min && (cur->end + size <= max) &&
                (cur->end + size <= next->start)) {
                *start = cur->end;
                return cur;
            }
        }

        return NULL;
    }

    if (node->type == BTREE_INTERNAL_NODE) {
        b_key *key, *pos;
        b_node *n;

        /* find the key that meets the equation key->start >= min */
        list_for_each_entry(pos, &node->key_h, key_h)
        {
            key = pos;
            if (pos->start >= min)
                break;
        }

        if (key->start >= min) {
            n = key->c_left;
        } else {
            n = key->c_right;
        }

        while (1) {
            if ((key = bt_find_empty_area(n, min, max, size, start)))
                return key;
            if (!n->p_right)
                break;
            n = n->p_right->c_right;  // next node
        }

        return NULL;
    }

    return NULL;
}

/* search from the start up until an key is found. return NULL if not found */
b_key *bt_find_key(b_node *node, uint64_t start)
{
    if (!node || !node->count)
        return NULL;

    if (!(node->min <= start && node->max > start)) {
        return NULL;
    }

    if (node->type == BTREE_EXTERNAL_NODE) {
        b_key *cur;

        /* iterate all keys of the node */
        list_for_each_entry(cur, &node->key_h, key_h)
        {
            if (!is_maximum(cur) && !is_minimum(cur) && cur->start >= start)
                return cur;
        }
        return NULL;
    }

    if (node->type == BTREE_INTERNAL_NODE) {
        b_key *key, *pos;
        b_node *n;

        /* find the key that meets the equation key->start >= start */
        list_for_each_entry(pos, &node->key_h, key_h)
        {
            key = pos;
            if (key->start >= start)
                break;
        }

        if (key->start == start) {
            return key; /* found */
        } else if (key->start > start) {
            n = key->c_left; /* maybe in left tree */
        } else {
            n = key->c_right; /* maybe in right tree */
        }

        return bt_find_key(n, start);
    }

    return NULL;
}

/* return zero if success, otherwise return error number */
uint32_t bt_insert_range(b_node **root,
                         uint64_t start,
                         uint64_t end,
                         uint64_t size,
                         void *entry,
                         uint64_t f_addr,
                         size_t f_size,
                         uint32_t prot)
{
    uint64_t addr;
    b_key *key;

    if (!(end > start && (end - start) >= size))
        return E_INVAL;
    if (!*root || !(*root)->count)
        return E_FAULT;
    if (!(key = bt_find_empty_area(*root, start, end, size, &addr)))
        return E_NO_MEM;
    if (is_minimum(key) && !(key->end <= addr))
        return E_NO_MEM;
    if (is_maximum(key) && !(addr + size <= key->start))
        return E_NO_MEM;

    /* insert into node */
    b_node *leaf = key->container;
    b_key *new_key =
        allocate_key(addr, addr + size, entry, f_addr, f_size, prot);
    new_key->container = leaf;
    if (addr >= key->end) {
        /* key | new_key */
        list_add(&new_key->key_h, &key->key_h);
        list_add(&new_key->b_key_h, &key->b_key_h);
    } else {
        /* new_key | key */
        list_add_tail(&new_key->key_h, &key->key_h);
        list_add_tail(&new_key->b_key_h, &key->b_key_h);
    }
    leaf->count++;
    update_until_root(leaf);

    if (leaf->count > KEY_MAX &&
        (!(left_rotation(leaf) || right_rotation(leaf)))) {
        split(leaf, root);
    }

    return 0;
}

void bt_init(btree *bt)
{
    bt->min = 0;
    bt->max = 1ULL << 48;
    init_btree(bt, bt->min, bt->max);
}

void bt_destroy(btree *bt)
{
    destroy_btree(bt->root);
    bt->root = NULL;  // failsafe
}
