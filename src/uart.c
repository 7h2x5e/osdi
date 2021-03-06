#include "c_string.h"
#include "peripherals/gpio.h"
#include "peripherals/irq.h"
#include "peripherals/mbox.h"
#include "types.h"

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

void ringbuf_init(ringbuf_t *);
void ringbuf_reset(ringbuf_t *);
size_t ringbuf_size(const ringbuf_t *);
bool ringbuf_is_full(const ringbuf_t *);
bool ringbuf_is_empty(const ringbuf_t *);
void ringbuf_push(ringbuf_t *, void *);
void ringbuf_pop(ringbuf_t *, void *);

static inline void uart_write_polling(char);
static inline char uart_read_polling();

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

bool ringbuf_is_full(const ringbuf_t *rb)
{
    __sync_synchronize();
    return rb->head == rb->tail;
}

bool ringbuf_is_empty(const ringbuf_t *rb)
{
    __sync_synchronize();
    return rb->head == (rb->tail + 1) % ringbuf_size(rb);
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

void uart_init(unsigned int baudrate)
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

    // enable UART interrupt
    ringbuf_init(&PL011_TX_QUEUE);
    ringbuf_init(&PL011_RX_QUEUE);
    uart_enable_rx_interrupt();
    *ENABLE_IRQS_2 |= 1 << 25;
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

ssize_t uart_read(ringbuf_t *rb, void *dst, size_t count)
{
    // Non-blocking read. Attempts to read up to 'count' bytes.
    ssize_t num = 0;
    while (!ringbuf_is_empty(rb) && num < count) {
        ringbuf_pop(rb, dst++);
        num++;
    }
    uart_enable_rx_interrupt();
    return num;
}

ssize_t uart_write(ringbuf_t *rb, void *src, size_t count)
{
    // Non-blocking write. Attempts to write up to 'count' bytes.
    ssize_t num = 0;
    while (!ringbuf_is_full(rb) && num < count) {
        ringbuf_push(rb, src++);
        num++;
    }
    uart_enable_tx_interrupt();
    return num;
}


int uart_getc()
{
    // Blocking read
    // Reads a character from ring buffer and returns it as an
    // unsigned char cast to an int.
    unsigned char buf[1];
    while (1 != uart_read(&PL011_RX_QUEUE, buf, 1)) {
    }
    buf[0] = (buf[0] == '\r') ? '\n' : buf[0];
    // Blocking write. Echo back with the character.
    while (1 != uart_write(&PL011_TX_QUEUE, buf, 1)) {
    };
    return (int) buf[0];
}

int uart_getc_raw()
{
    // Blocking read
    // Reads a character from ring buffer and returns it as an
    // unsigned char cast to an int.
    unsigned char buf[1];
    while (1 != uart_read(&PL011_RX_QUEUE, buf, 1)) {
    }
    return (int) buf[0];
}

int uart_putc(unsigned char c)
{
    // Blocking write
    // Write a character to ring buffer and return the character
    // written as an unsigned char cast to an int.
    if (c == '\n') {
        while (1 != uart_write(&PL011_TX_QUEUE, &(char[1]){'\r'}, 1)) {
        }
    }
    while (1 != uart_write(&PL011_TX_QUEUE, &(char[1]){c}, 1)) {
    }
    return (int) c;
}

char *uart_fgets(char *buf, size_t len)
{
    // Attempts to read up to one less than 'len' characters.
    // Reading stops after a newline. If a newline is read, it
    // is stored into the buffer. A terminating null byte '\0'
    // is stored after the last character in the buffer.
    // Return 'buf' on success or NULL on error.
    size_t idx = 0;
    char c;
    if (len < 2)
        return NULL;
    do {
        c = uart_getc();
        buf[idx++] = c;
    } while (c != '\n' && idx < (len - 1));
    buf[idx] = '\0';
    return buf;
}

ssize_t uart_fputs(const char *buf)
{
    // Attempts to write the string 'buf' without its terminating
    // null byte. Return a nonnegative number on success, or EOF
    // on error.
    size_t count = 0;
    while (buf[count]) {
        uart_putc(buf[count++]);
    }
    return count;
}

void uart_printf(char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    extern char _end;
    char *dst = &_end;
    vsprintf(dst, fmt, args);
    __builtin_va_end(args);

    uart_fputs(dst);
}

void uart_flush()
{
    // Flush receive FIFO
    while (!(*UART0_FR & 0x10)) {
        (*UART0_DR);
    }
}

void uart_handler()
{
    uint32_t status = *UART0_MIS;

    if (status & MIS_RXMIS) {
        if (!ringbuf_is_full(&PL011_RX_QUEUE)) {
            uint8_t data = (uint8_t) *UART0_DR;
            ringbuf_push(&PL011_RX_QUEUE, &data);
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
        }
        if (ringbuf_is_empty(&PL011_TX_QUEUE)) {
            uart_disable_tx_interrupt();
        }
    }
}
