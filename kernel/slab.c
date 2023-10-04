#include <include/slab.h>
#include <include/types.h>
#include <include/list.h>
#include <include/mm.h>
#include <include/assert.h>

void *slab_alloc(struct slab *slab)
{
    if (list_empty(&slab->free_list)) {
        struct slab_page *page =
            (struct slab_page *) PA_TO_KVA(page2pa(buddy_alloc(0)));
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

void slab_free(struct slab *slab, void *ptr)
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