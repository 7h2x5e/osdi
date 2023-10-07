#include <include/slab.h>
#include <include/types.h>
#include <include/list.h>
#include <include/mm.h>
#include <include/assert.h>
#include <include/compiler.h>

DEFINE_SLAB(kmalloc_slab_96, 96);
DEFINE_SLAB(kmalloc_slab_192, 192);
DEFINE_SLAB(kmalloc_slab_8, 8);
DEFINE_SLAB(kmalloc_slab_16, 16);
DEFINE_SLAB(kmalloc_slab_32, 32);
DEFINE_SLAB(kmalloc_slab_64, 64);
DEFINE_SLAB(kmalloc_slab_128, 128);
DEFINE_SLAB(kmalloc_slab_256, 256);
DEFINE_SLAB(kmalloc_slab_512, 512);
DEFINE_SLAB(kmalloc_slab_1024, 1024);
DEFINE_SLAB(kmalloc_slab_2048, 2048);

static uint8_t size_index[24] = {
    3, /* 8 */
    4, /* 16 */
    5, /* 24 */
    5, /* 32 */
    6, /* 40 */
    6, /* 48 */
    6, /* 56 */
    6, /* 64 */
    1, /* 72 */
    1, /* 80 */
    1, /* 88 */
    1, /* 96 */
    7, /* 104 */
    7, /* 112 */
    7, /* 120 */
    7, /* 128 */
    2, /* 136 */
    2, /* 144 */
    2, /* 152 */
    2, /* 160 */
    2, /* 168 */
    2, /* 176 */
    2, /* 184 */
    2  /* 192 */
};

struct kmalloc_info_struct kmalloc_info[] = {
    {NULL, 0, NULL},
    {"kmalloc-96", 96, &kmalloc_slab_96},
    {"kmalloc-192", 192, &kmalloc_slab_192},
    {"kmalloc-8", 8, &kmalloc_slab_8},
    {"kmalloc-16", 16, &kmalloc_slab_16},
    {"kmalloc-32", 32, &kmalloc_slab_32},
    {"kmalloc-64", 64, &kmalloc_slab_64},
    {"kmalloc-128", 128, &kmalloc_slab_128},
    {"kmalloc-256", 256, &kmalloc_slab_256},
    {"kmalloc-512", 512, &kmalloc_slab_512},
    {"kmalloc-1k", 1024, &kmalloc_slab_1024},
    {"kmalloc-2k", 2048, &kmalloc_slab_2048},
};

static void *slab_alloc(struct slab *slab)
{
    if (list_empty(&slab->free_list)) {
        page_t *allocated_page = buddy_alloc(0);
        allocated_page->page_slab = slab;

        struct slab_page *page =
            (struct slab_page *) PA_TO_KVA(page2pa(allocated_page));
        list_add(&page->page_list, &slab->page_list);
        page->nr_free = 0;

        for (kernaddr_t begin = (kernaddr_t) page->begin;
             begin +
                 ROUNDUP(sizeof(struct slab_object) + slab->object_size, 8) <=
             (kernaddr_t) page + PAGE_SIZE;
             begin +=
             ROUNDUP(sizeof(struct slab_object) + slab->object_size, 8)) {
            list_add(&((struct slab_object *) begin)->free_list,
                     &slab->free_list);
            page->nr_free++;
        }
    }

    assert(!list_empty(&slab->free_list));

    struct slab_object *obj =
        list_first_entry(&slab->free_list, struct slab_object, free_list);
    struct slab_page *page =
        (struct slab_page *) ROUNDDOWN((kernaddr_t) obj, PAGE_SIZE);

    assert(page->nr_free > 0);

    page->nr_free--;
    list_del(&obj->free_list);

    return (void *) obj->ptr;
}

static void slab_free(struct slab *slab, void *ptr)
{
    struct slab_object *obj = container_of(ptr, struct slab_object, ptr);
    list_add(&obj->free_list, &slab->free_list);

    struct slab_page *page =
        (struct slab_page *) ROUNDDOWN((kernaddr_t) obj, PAGE_SIZE);
    page->nr_free++;

    if (page->nr_free ==
        (PAGE_SIZE - sizeof(struct slab_page)) /
            ROUNDUP(sizeof(struct slab_object) + slab->object_size, 8)) {
        for (kernaddr_t begin = (kernaddr_t) page->begin;
             begin +
                 ROUNDUP(sizeof(struct slab_object) + slab->object_size, 8) <=
             (kernaddr_t) page + PAGE_SIZE;
             begin +=
             ROUNDUP(sizeof(struct slab_object) + slab->object_size, 8)) {
            list_del(&((struct slab_object *) begin)->free_list);
            page->nr_free--;
        }
        assert(page->nr_free == 0);
        list_del(&page->page_list);
        buddy_free(pa2page(KVA_TO_PA(page)));
    }
}

void *kmalloc(size_t size)
{
    /* if size > 192, round up to the next highest power of 2 */
    uint64_t mask = (size <= 192) ? 8 : 1ULL << (63 - __builtin_clzll(size));
    size = ROUNDUP(size, mask);

    /* if size > 2048, allocate continuous page */
    if (size > (PAGE_SIZE >> 1)) {
        uint8_t order = 63 - __builtin_clzll(size >> PAGE_SHIFT);
        page_t *page = buddy_alloc(order);
        page->page_slab = NULL;
        return (void *) PA_TO_KVA(page2pa(page));
    }

    struct kmalloc_info_struct *k_info =
        (size <= 192) ? &kmalloc_info[size_index[(size >> 3) - 1]]
                      : &kmalloc_info[63 - __builtin_clzll(size)];
    return slab_alloc(k_info->slab);
}

void kfree(const void *x)
{
    if (unlikely(x == NULL))
        return;

    page_t *page = pa2page(KVA_TO_PA((kernaddr_t) ROUNDDOWN(x, PAGE_SIZE)));
    if (unlikely(page->page_slab == NULL)) {
        buddy_free(page);
        return;
    }
    slab_free(page->page_slab, (void *) x);
}