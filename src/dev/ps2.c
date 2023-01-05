#include "ps2.h"
#include "console/console.h"
#include "dev/keyboard.h"
#include "idt/idt.h"
#include "io/io.h"
#include "task/process.h"

#include <stdint.h>

#define ISR_KEYBOARD_INTERRUPT 0x21

#define PS2_COMMAND_PORT 0x64
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

#define PS2_KEY_RELEASED 0x80

#define PS2_KEYBOARD_CAPSLOCK 0x3A

int ps2_init();

static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
    '-',  '=',  0x08, '\t', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    'O',  'P',  '[',  ']',  0x0d, 0x00, 'A',  'S',  'D',  'F',  'G',  'H',
    'J',  'K',  'L',  ';',  '\'', '`',  0x00, '\\', 'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  ',',  '.',  '/',  0x00, '*',  0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '7',  '8',  '9',
    '-',  '4',  '5',  '6',  '+',  '1',  '2',  '3',  '0',  '.'};

struct keyboard ps2_keyboard = {
    .name = {"PS/2 Keyboard"},
    .init = ps2_init,
};

struct keyboard *get_ps2_keyboard() { return &ps2_keyboard; }

uint8_t ps2_scancode_to_char(uint8_t scancode) {
    if (scancode > sizeof(keyboard_scan_set_one) / sizeof(uint8_t)) {
        return 0x00;
    }
    char c = keyboard_scan_set_one[scancode];
    if (keyboard_get_capslock(&ps2_keyboard) == KEYBOARD_STATE_CAPS_OFF) {
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
    }
    return c;
}

void keyboard_intr_handler(struct interrupt_frame *frame) {
    uint8_t scancode = port_io_input_byte(PS2_DATA_PORT);
    port_io_input_byte(PS2_DATA_PORT); // Ignore the second byte

    if (scancode & PS2_KEY_RELEASED) {
        goto out;
    }
    if (scancode == PS2_KEYBOARD_CAPSLOCK) {
        keyboard_toggle_capslock(&ps2_keyboard);
        // goto out;
        // TODO: Should we pass the capslock keypress to the userspace?
    }

    uint8_t c = ps2_scancode_to_char(scancode);
    if (c != 0x00) {
        keyboard_push(c, process_current());
    }

out:
    // Don't forget to acknowledge the interrupt
    port_io_out_byte(MASTER_PIC_PORT, MASTER_PIC_INTR_ACK);
}

int ps2_init() {
    idt_register_interrupt_call_back(ISR_KEYBOARD_INTERRUPT,
                                     keyboard_intr_handler);

    keyboard_set_capslock(&ps2_keyboard, KEYBOARD_STATE_CAPS_OFF);
    port_io_out_byte(PS2_COMMAND_PORT, 0xAE); // Enable first PS/2 port
    return 0;
}