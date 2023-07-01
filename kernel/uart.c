#include <include/irq.h>
#include <include/peripherals/gpio.h>
#include <include/peripherals/irq.h>
#include <include/peripherals/mbox.h>
#include <include/peripherals/uart.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/types.h>

/* PL011 UART registers */
#define UART0_DR \
    ((volatile unsigned int *) (MMIO_BASE + 0x00201000))  // Data register
#define UART0_FR \
    ((volatile unsigned int *) (MMIO_BASE + 0x00201018))  // Flag register
#define UART0_IBRD ((volatile unsigned int *) (MMIO_BASE + 0x00201024))
#define UART0_FBRD ((volatile unsigned int *) (MMIO_BASE + 0x00201028))
#define UART0_LCRH ((volatile unsigned int *) (MMIO_BASE + 0x0020102C))
#define UART0_CR ((volatile unsigned int *) (MMIO_BASE + 0x00201030))
#define UART0_IMSC ((volatile unsigned int *) (MMIO_BASE + 0x00201038))
#define UART0_MIS ((volatile unsigned int *) (MMIO_BASE + 0x00201040))
#define UART0_ICR ((volatile unsigned int *) (MMIO_BASE + 0x00201044))
#define IMSC_RXIM (1 << 4)
#define IMSC_TXIM (1 << 5)
#define MIS_RXMIS (1 << 4)
#define MIS_TXMIS (1 << 5)

/* PL011 UART ring buffer */
#define PL011_RINGBUFF_SIZE (1 << 10)
typedef struct ringbuf_t {
    uint8_t buf[PL011_RINGBUFF_SIZE];
    size_t head, tail;
    size_t size, mask;
} ringbuf_t;

static ringbuf_t PL011_TX_QUEUE, PL011_RX_QUEUE;
static bool mode;

void ringbuf_init(ringbuf_t *);
void ringbuf_reset(ringbuf_t *);
size_t ringbuf_size(const ringbuf_t *);
bool ringbuf_is_full(const ringbuf_t *);
bool ringbuf_is_empty(const ringbuf_t *);
void ringbuf_push(ringbuf_t *, void *);
void ringbuf_pop(ringbuf_t *, void *);

static inline void uart_write_polling(char);
static inline char uart_read_polling();
int uart_getc();
int uart_putc(unsigned char);

void ringbuf_init(ringbuf_t *rb)
{
    // Ensure size is greater than zero and two to the power of n, n > 0.
    if (!PL011_RINGBUFF_SIZE || (PL011_RINGBUFF_SIZE & 0x80000000))
        return;
    rb->size = 1 << (31 - __builtin_clz(PL011_RINGBUFF_SIZE));
    rb->mask = rb->size - 1;
    ringbuf_reset(rb);
}

void ringbuf_reset(ringbuf_t *rb)
{
    rb->head = rb->tail = 0;
}

size_t ringbuf_size(const ringbuf_t *rb)
{
    return rb->size;
}

bool ringbuf_is_empty(const ringbuf_t *rb)
{
    __sync_synchronize();
    return rb->head == rb->tail;
}

bool ringbuf_is_full(const ringbuf_t *rb)
{
    __sync_synchronize();
    return (rb->head == (rb->tail + 1)) & rb->mask;
}

void ringbuf_push(ringbuf_t *rb, void *src)
{
    // assume ring buffer is not full
    rb->buf[rb->tail++] = *((uint8_t *) src);
    rb->tail &= rb->mask;
}

void ringbuf_pop(ringbuf_t *rb, void *dst)
{
    // assume ring buffer is not empty
    *((uint8_t *) dst) = rb->buf[rb->head++];
    rb->head &= rb->mask;
}

void uart_set_mode(bool _mode)
{
    mode = _mode;
};

void uart_enable_tx_interrupt()
{
    *UART0_IMSC |= IMSC_TXIM;
}

void uart_disable_tx_interrupt()
{
    *UART0_IMSC &= ~IMSC_TXIM;
}

void uart_enable_rx_interrupt()
{
    *UART0_IMSC |= IMSC_RXIM;
}

void uart_disable_rx_interrupt()
{
    *UART0_IMSC &= ~IMSC_RXIM;
}

void uart_init(unsigned int baudrate, bool _mode)
{
    /* 1. Turn off UART0 */
    *UART0_CR = 0;

    /* 2. Set up clock for consistent divisor values */
    mbox[0] = 9 * 4;
    mbox[1] = MBOX_REQUEST;
    mbox[2] = MBOX_TAG_SETCLKRATE;  // set clock rate
    mbox[3] = 12;                   // Buffer's length
    mbox[4] = MBOX_TAG_REQ;
    mbox[5] = 2;        // UART clock
    mbox[6] = 4000000;  // 4Mhz
    mbox[7] = 0;        // clear turbo
    mbox[8] = MBOX_TAG_LAST;
    mbox_call(MBOX_CH_PROP);

    /* 3. Map UART0 to GPIO pins
     *
     * Goal: Set GPIO14, 15 to ALT0
     * GPIO 0-9   => GPFSEL0
     * GPIO 10-19 => GPFSEL1
     * GPIO 20-29 => GPFSEL2
     * ...
     * GPIO14-15 use FSEL4 and FSEL5 field in GPFSEL1.
     */
    register unsigned int r;
    r = *GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |= (4 << 12) | (4 << 15);  // ALT0 = 100
    *GPFSEL1 = r;

    /* 4. Disable GPIO pull-up/down */
    *GPPUD = 0;
    // Wait 150 cycles (required set-up time for the control signal)
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    // Wait 150 cycles (required hold time for the control signal)
    r = 150;
    while (r--) {
        asm volatile("nop");
    }
    *GPPUDCLK0 = 0;  // Flush GPIO setup

    // 5. Enable Tx, Rx
    float buddiv = 4000000 / baudrate / 16;
    *UART0_ICR = 0x7FF;                   // clear interrupts
    *UART0_IBRD = (unsigned int) buddiv;  // 115200 baud, IBRD=0x2, FBRD=0xB
    *UART0_FBRD = (unsigned int) ((buddiv - *UART0_IBRD) * 64);
    *UART0_LCRH = 0x7 << 4;  // 8bits, enable FIFOs
    *UART0_CR = 0x301;       // enable Tx, Rx, UART

    // deal with qemu bug
    uart_write_polling(0);

    uart_set_mode(_mode);
    if (UART_INTERRUPT_MODE == mode) {
        // enable UART interrupt
        ringbuf_init(&PL011_TX_QUEUE);
        ringbuf_init(&PL011_RX_QUEUE);
        uart_enable_rx_interrupt();
        uart_enable_tx_interrupt();
        *ENABLE_IRQS_2 |= 1 << 25;
    }
}

static inline void uart_write_polling(char c)
{
    /* Wait until we can send */
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x20);
    *UART0_DR = c;
}

static inline char uart_read_polling()
{
    /* Wait until something is in the buffer */
    do {
        asm volatile("nop");
    } while (*UART0_FR & 0x10);
    return (char) (*UART0_DR);
}

ssize_t _uart_read(void *dst, size_t count)
{
    if (!count)
        return count;

    ringbuf_t *rb = &PL011_RX_QUEUE;
    ssize_t num = 0;
    if (UART_INTERRUPT_MODE == mode) {
        /*
         * Non-blocking read. Read up to 'count' bytes
         * If there isn't enough data in buffer, we put task to the waitqueue.
         */
        while (true) {
            /* prevent irq handler from accessing RX ring buffer concurrently */
            disable_irq();
            {
                // Attempts to read up to 'count' bytes.
                while (!ringbuf_is_empty(rb) && num < count) {
                    ringbuf_pop(rb, dst++);
                    num++;
                }
                uart_enable_rx_interrupt();
            }
            enable_irq();

            if (num >= count) {
                return num;
            }

            /*
             * If don't have enough data, mark the task as TASK_BLOCKED and push
             * it to waitqueue
             */

            /* prevent irq handler from accessing wait queue concurrently */
            disable_irq();
            {
                task_t *cur = (task_t *) get_current();
                cur->state = TASK_BLOCKED;
                runqueue_push(&waitqueue, &cur);
            }
            enable_irq();
            schedule();
        }
    }
    // Blocking read. Read 'count' bytes
    while (num < count) {
        ((char *) dst)[num++] = uart_read_polling();
    }
    return count;
}

ssize_t _uart_write(void *src, size_t count)
{
    ringbuf_t *rb = &PL011_TX_QUEUE;
    ssize_t num = 0;
    if (UART_INTERRUPT_MODE == mode) {
        // Non-blocking write. Attempts to write up to 'count' bytes.
        while (!ringbuf_is_full(rb) && num < count) {
            ringbuf_push(rb, src++);
            num++;
        }
        uart_enable_tx_interrupt();
        return num;
    }
    // Blocking write. Write 'count' bytes
    while (num < count) {
        uart_write_polling(((char *) src)[num++]);
    }
    return count;
}

void uart_flush()
{
    // Flush receive FIFO
    while (!(*UART0_FR & 0x10)) {
        (*UART0_DR);
    }
}

/*
 * @return 0b01 - data has been pushed to RX buffer
 * @return 0b10 - data has been poped out from TX buffer
 * @return 0b11 - the above of all
 */
int8_t uart_handler()
{
    uint32_t status = *UART0_MIS;
    int8_t ret = 0;

    if (status & MIS_RXMIS) {
        if (!ringbuf_is_full(&PL011_RX_QUEUE)) {
            uint8_t data = (uint8_t) *UART0_DR;
            ringbuf_push(&PL011_RX_QUEUE, &data);
            ret |= 1;
        }
        if (ringbuf_is_full(&PL011_RX_QUEUE)) {
            uart_disable_rx_interrupt();
        }
    }

    if (status & MIS_TXMIS) {
        if (!ringbuf_is_empty(&PL011_TX_QUEUE)) {
            uint8_t data;
            ringbuf_pop(&PL011_TX_QUEUE, &data);
            *UART0_DR = (uint32_t) data;
            ret |= 2;
        }
        if (ringbuf_is_empty(&PL011_TX_QUEUE)) {
            uart_disable_tx_interrupt();
        }
    }
    return ret;
}
