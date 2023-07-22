#include <include/arm/sysregs.h>
#include <include/irq.h>
#include <include/kernel_log.h>
#include <include/sched.h>
#include <include/string.h>
#include <include/task.h>
#include <include/types.h>
#include <include/mman.h>
#include <include/elf.h>

runqueue_t runqueue, waitqueue;
struct list_head zombie_list;
static task_t task_pool[MAX_TASK] __attribute__((section(".kstack")));
static uint8_t kstack_pool[MAX_TASK][KSTACK_SIZE]
    __attribute__((section(".kstack")));
static uint8_t ustack_pool[MAX_TASK][USTACK_SIZE]
    __attribute__((section(".ustack")));

/*
 * Return a pointer pointing to a task no matter what the task's state is.
 */
task_t *get_task_by_id(uint32_t id)
{
    if (id < MAX_TASK) {
        return &task_pool[id];
    }
    return NULL;
}

uint32_t do_get_taskid()
{
    const task_t *task = get_current();
    return task->tid;
}

void *get_kstack_by_id(uint32_t id)
{
    return (void *) &kstack_pool[id][0];
}

void *get_ustack_by_id(uint32_t id)
{
    return (void *) &ustack_pool[id][0];
}

void *get_kstacktop_by_id(uint32_t id)
{
    return (void *) &kstack_pool[id + 1][0];
}

void *get_ustacktop_by_id(uint32_t id)
{
    return (void *) &ustack_pool[id + 1][0];
}

void init_task()
{
    runqueue_init(&runqueue);
    runqueue_init(&waitqueue);
    INIT_LIST_HEAD(&zombie_list);

    for (uint32_t i = 0; i < MAX_TASK; ++i) {
        task_pool[i].state = TASK_UNUSED;
    }

    // initialize main() as idle task
    task_t *self = &task_pool[0];
    self->tid = 0;
    self->state = TASK_RUNNING;
    asm volatile("msr tpidr_el1, %0" ::"r"(self));
}

static inline int64_t get_pid()
{
    // look for unused task struct
    uint32_t tid = 0;
    for (tid = 0; tid < MAX_TASK; ++tid) {
        if (task_pool[tid].state == TASK_UNUSED)
            return tid;
    }
    return -1;
}

/*
 * The exec() functions return only if an error has occurred. The return value
 * is -1. User must call exit() to reclaim resources after error occured.
 */
int do_exec(uint64_t bin_start)
{
    uintptr_t sp =
        USER_VIRT_TOP -
        sizeof(uintptr_t);  // stack address grows towards lower memory address

    /* Reading ELF Header */
    Elf64_Ehdr *elf64_ehdr = (Elf64_Ehdr *) bin_start;
    KERNEL_LOG_DEBUG("ELF Header Header");
    KERNEL_LOG_DEBUG(" %s %x", "e_entry", elf64_ehdr->e_entry);
    KERNEL_LOG_DEBUG(" %s %x", "e_phoff", elf64_ehdr->e_phoff);
    KERNEL_LOG_DEBUG(" %s %x", "e_phnum", elf64_ehdr->e_phnum);

    /* Reading Program Header */
    Elf64_Phdr *elf64_phdr = NULL;
    for (int i = 0; i < elf64_ehdr->e_phnum; ++i) {
        elf64_phdr = (Elf64_Phdr *) (bin_start + elf64_ehdr->e_phoff +
                                     elf64_ehdr->e_phentsize * i);
        KERNEL_LOG_DEBUG("Program Header");
        KERNEL_LOG_DEBUG(" %s %x", "p_type", elf64_phdr->p_type);
        KERNEL_LOG_DEBUG(" %s %x", "p_vaddr", elf64_phdr->p_vaddr);
        KERNEL_LOG_DEBUG(" %s %x", "p_offset", elf64_phdr->p_offset);
        KERNEL_LOG_DEBUG(" %s %x", "p_align", elf64_phdr->p_align);
        KERNEL_LOG_DEBUG(" %s %x", "p_filesz", elf64_phdr->p_filesz);
        KERNEL_LOG_DEBUG(" %s %x", "p_memsz", elf64_phdr->p_memsz);
        KERNEL_LOG_DEBUG(" %s %x", "p_flags", elf64_phdr->p_flags);
        if (elf64_phdr->p_type == PT_LOAD) {
            break;
        }
    }
    if (elf64_phdr->p_type != PT_LOAD) {
        KERNEL_LOG_DEBUG("Could not find loadable segment!");
        return -1;
    }

    /*    p_vaddr_aligned
     *    v
     *    |----------------- virtual space -----------------|
     *   /                                                 /
     *  /                                                 /
     * |-----| loadable segment = MAX(p_memsz, p_filesz) |
     * ^     ^
     * |     p_offset
     * p_offset_aligned
     */
    uint64_t p_vaddr = elf64_phdr->p_vaddr;
    uint64_t p_filesz = elf64_phdr->p_filesz;
    uint32_t p_flags = elf64_phdr->p_flags;
    uint64_t p_offset = elf64_phdr->p_offset;
    uint64_t p_memsz = elf64_phdr->p_memsz;
    uint64_t p_vaddr_aligned = ROUNDDOWN(p_vaddr, PAGE_SIZE);
    uint64_t p_offset_aligned = ROUNDDOWN(p_offset, PAGE_SIZE);

    /* allocate memory for .txt & .bss section */
    if (MAP_FAILED ==
        do_mmap(p_vaddr_aligned,
                MAX(p_memsz, p_filesz) + (p_offset - p_offset_aligned), p_flags,
                MAP_FIXED, bin_start, p_offset_aligned))
        return -1;

    /* allocate stack */
    if (MAP_FAILED == do_mmap(sp, PAGE_SIZE, PROT_READ | PROT_WRITE,
                              MAP_ANONYMOUS, (uint64_t) NULL, 0)) {
        do_munmap(sp, PAGE_SIZE);
        return -1;
    }

    /* update ttbr0_el1 */
    task_t *task = (task_t *) get_current();
    create_pgd(&task->mm); /* demand paging. only allocate PGD in the beggining,
                              the others are delayed */
    update_pgd(task->mm.pgd);

    /* instead of zero-filling .bss section, we zero-fill a new page */

    /* switch to el0 */
    asm volatile(
        "msr     sp_el0, %0\n\t"
        "msr     elr_el1, %1\n\t"
        "msr     spsr_el1, %2\n\t"
        "eret" ::"r"(sp),
        "r"(p_vaddr_aligned + (p_offset - p_offset_aligned)),
        "r"(SPSR_EL1_VALUE));
    __builtin_unreachable();
}

/*
 * On success, the task id of the child process is returned
 * in the parent, and 0 is returned in the child. On failure,
 * -1 is returned in the parent.
 */
int64_t do_fork(struct TrapFrame *tf)
{
    // create a new task: child
    int64_t new_task_id = privilege_task_create(NULL);
    if (new_task_id < 0)
        return -1;
    task_t *new_task = get_task_by_id(new_task_id);
    const task_t *cur_task = get_current();

    // fork page table. btree must be forked earlier than page table
    if (-1 == fork_btree(&new_task->mm, &cur_task->mm) ||
        -1 == fork_page_table(&new_task->mm, &cur_task->mm)) {
        /* reclaim new task's resource */
        new_task->state = TASK_ZOMBIE;
        list_add_tail(&new_task->node, &zombie_list);
        mm_struct_destroy(&new_task->mm);
        return -1;
    }

    // page permission changed, flush TLB
    asm volatile(
        "tlbi vmalle1is\n"  // invalidate all TLB entries
        "dsb ish"           // ensure completion of TLB invalidatation
    );

    // parent process and child process have the same content of TrapFrame
    void *kstacktop_new = get_kstacktop_by_id(new_task->tid);
    struct TrapFrame *tf_new =
        (struct TrapFrame *) (kstacktop_new - sizeof(struct TrapFrame));
    *tf_new = *tf;

    // set child's return value
    tf_new->x[0] = 0;

    // set child task's task_struct
    extern void ret_to_user();
    new_task->task_context.lr = (uint64_t) ret_to_user;
    new_task->task_context.sp = (uint64_t) tf_new;
    new_task->state = TASK_RUNNABLE;
    new_task->sig_blocked = cur_task->sig_blocked;
    new_task->sig_pending = cur_task->sig_pending;

    return new_task_id;
}

void do_exit()
{
    task_t *cur = (task_t *) get_current();
    cur->state = TASK_ZOMBIE;
    list_add_tail(&cur->node, &zombie_list);
    mm_struct_destroy(&cur->mm);
    schedule();
    __builtin_unreachable();
}

int64_t privilege_task_create(void (*func)())
{
    int64_t tid = get_pid();
    if (tid < 0 || runqueue_is_full(&runqueue))
        return -1;

    task_t *task = &task_pool[tid];
    task->tid = tid;
    task->task_context.sp = (uint64_t) get_kstacktop_by_id(tid);
    task->task_context.lr = (uint64_t) *func;
    task->state = TASK_RUNNABLE;
    task->counter = TASK_EPOCH;
    task->sig_pending = 0;
    task->sig_blocked = 0;
    mm_struct_init(&task->mm);
    INIT_LIST_HEAD(&task->node);
    runqueue_push(&runqueue, &task);

    return (int64_t) tid;
}

void runqueue_init(runqueue_t *rq)
{
    rq->size = RUNQUEUE_SIZE;
    rq->mask = rq->size - 1;
    runqueue_reset(rq);
}

void runqueue_reset(runqueue_t *rq)
{
    rq->head = rq->tail = 0;
}

size_t runqueue_size(const runqueue_t *rq)
{
    return rq->size;
}

bool runqueue_is_full(const runqueue_t *rq)
{
    __sync_synchronize();
    return (rq->head == ((rq->tail + 1) & rq->mask));
}

bool runqueue_is_empty(const runqueue_t *rq)
{
    __sync_synchronize();
    return rq->head == rq->tail;
}

void runqueue_push(runqueue_t *rq, task_t **t)
{
    // assume buffer isn't full
    rq->buf[rq->tail++] = *t;
    rq->tail &= rq->mask;
}

void runqueue_pop(runqueue_t *rq, task_t **t)
{
    // assume buffer isn't empty
    *t = rq->buf[rq->head++];
    rq->head &= rq->mask;
}

__attribute__((optimize("O0"))) static void delay(uint64_t count)
{
    for (uint64_t i = 0; i < count; ++i)
        asm("nop");
}

void idle()
{
    while (1) {
        if (runqueue_is_empty(&runqueue)) {
            break;
        }
        reschedule();
        delay(100000000);
    }
    KERNEL_LOG_DEBUG("Test finished");
    while (1)
        ;
}

void zombie_reaper()
{
    while (1) {
        if (list_empty(&zombie_list)) {
            schedule();
            enable_irq();
        } else {
            /* free zombie task's resource */
            task_t *zt, *tmp;
            list_for_each_entry_safe(zt, tmp, &zombie_list, node)
            {
                list_del_init(&zt->node);
                zt->state = TASK_UNUSED;
                KERNEL_LOG_DEBUG(
                    "Zombie reaper frees process [PID %d] resources", zt->tid);
            }
        }
    }
}