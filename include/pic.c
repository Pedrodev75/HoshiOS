#include "pic.h"
#include "io.h"

static void pic_wait(void) {
    outb(0x80, 0);
}

void pic_remap(void) {
    uint8_t master_mask = inb(0x21);
    uint8_t slave_mask = inb(0xA1);

    outb(0x20, 0x11);
    pic_wait();
    outb(0xA0, 0x11);
    pic_wait();

    outb(0x21, 0x20);
    pic_wait();
    outb(0xA1, 0x28);
    pic_wait();

    outb(0x21, 0x04);
    pic_wait();
    outb(0xA1, 0x02);
    pic_wait();

    outb(0x21, 0x01);
    pic_wait();
    outb(0xA1, 0x01);
    pic_wait();

    outb(0x21, master_mask);
    outb(0xA1, slave_mask);
}

void pic_enable_timer_and_mouse(void) {
    outb(0x21, 0xFA);
    outb(0xA1, 0xEF);
}