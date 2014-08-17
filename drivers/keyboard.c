// keyboard.c -- Brad Slayter

#include "drivers/keyboard.h"

#include "kernel/isr.h"

#include "lib/stdio.h"

enum KB_ENCODER_IO {
	KB_ENC_INPUT_BUF = 0x60,
	KB_ENC_CMD_REG	 = 0x60
};

enum KB_ENC_CMDS {
	KB_ENC_CMD_SET_LED = 0xED,
	KB_ENC_CMD_ECHO = 0xEE,
	KB_ENC_CMD_SCAN_CODE_SET = 0xF0,
	KB_ENC_CMD_ID = 0xF2,
	KB_ENC_CMD_AUTODELAY = 0xF3,
	KB_ENC_CMD_ENABLE = 0xF4,
	KB_ENC_CMD_RESETWAIT = 0xF5,
	KB_ENC_CMD_RESETSCAN = 0xF6,
	KB_ENC_CMD_ALL_AUTO = 0xF7,
	KB_ENC_CMD_ALL_MAKEBREAK = 0xF8,
	KB_ENC_CMD_ALL_MAKEONLY = 0xF9,
	KB_ENC_CMD_ALL_MAKEBREAK_AUTO = 0xFA,
	KB_ENC_CMD_SINGLE_AUTOREPEAT = 0xFB,
	KB_ENC_CMD_SINGLE_MAKEBREAK = 0xFC,
	KB_ENC_CMD_SINGLE_BREAKONLY = 0xFD,
	KB_ENC_CMD_RESEND =	0xFE,
	KB_ENC_CMD_RESET = 0xFF
};

enum KB_CTRL_IO {

	KB_CTRL_STATS_REG=	0x64,
	KB_CTRL_CMD_REG	=	0x64
};

enum KB_CTRL_STATS_MASK {

	KB_CTRL_STATS_MASK_OUT_BUF	=	1,		//00000001
	KB_CTRL_STATS_MASK_IN_BUF	=	2,		//00000010
	KB_CTRL_STATS_MASK_SYSTEM	=	4,		//00000100
	KB_CTRL_STATS_MASK_CMD_DATA	=	8,		//00001000
	KB_CTRL_STATS_MASK_LOCKED	=	0x10,	//00010000
	KB_CTRL_STATS_MASK_AUX_BUF	=	0x20,	//00100000
	KB_CTRL_STATS_MASK_TIMEOUT	=	0x40,	//01000000
	KB_CTRL_STATS_MASK_PARITY	=	0x80	//10000000
};

enum KB_CTRL_CMDS {

	KB_CTRL_CMD_READ				=	0x20,
	KB_CTRL_CMD_WRITE			=	0x60,
	KB_CTRL_CMD_SELF_TEST		=	0xAA,
	KB_CTRL_CMD_INTERFACE_TEST	=	0xAB,
	KB_CTRL_CMD_DISABLE			=	0xAD,
	KB_CTRL_CMD_ENABLE			=	0xAE,
	KB_CTRL_CMD_READ_IN_PORT		=	0xC0,
	KB_CTRL_CMD_READ_OUT_PORT	=	0xD0,
	KB_CTRL_CMD_WRITE_OUT_PORT	=	0xD1,
	KB_CTRL_CMD_READ_TEST_INPUTS	=	0xE0,
	KB_CTRL_CMD_SYSTEM_RESET		=	0xFE,
	KB_CTRL_CMD_MOUSE_DISABLE	=	0xA7,
	KB_CTRL_CMD_MOUSE_ENABLE		=	0xA8,
	KB_CTRL_CMD_MOUSE_PORT_TEST	=	0xA9,
	KB_CTRL_CMD_MOUSE_WRITE		=	0xD4
};

enum KB_ERROR {

	KB_ERR_BUF_OVERRUN			=	0,
	KB_ERR_ID_RET				=	0x83AB,
	KB_ERR_BAT					=	0xAA,	//note: can also be L. shift key make code
	KB_ERR_ECHO_RET				=	0xEE,
	KB_ERR_ACK					=	0xFA,
	KB_ERR_BAT_FAILED			=	0xFC,
	KB_ERR_DIAG_FAILED			=	0xFD,
	KB_ERR_RESEND_CMD			=	0xFE,
	KB_ERR_KEY					=	0xFF
};

static char scancode;
static u8int numlock, capslock, scrolllock;
static u8int shift, ctrl, alt;
static int kb_error = 0;
static u8int kb_bat_res = 0;
static u8int kb_diag_res = 0;
static u8int kb_resend_res = 0;
static u8int _kb_disable = 0;

static int kb_scancode_std[] = {
	//! key			scancode
	KEY_UNKNOWN,	//0
	KEY_ESCAPE,		//1
	KEY_1,			//2
	KEY_2,			//3
	KEY_3,			//4
	KEY_4,			//5
	KEY_5,			//6
	KEY_6,			//7
	KEY_7,			//8
	KEY_8,			//9
	KEY_9,			//0xa
	KEY_0,			//0xb
	KEY_MINUS,		//0xc
	KEY_EQUAL,		//0xd
	KEY_BACKSPACE,	//0xe
	KEY_TAB,		//0xf
	KEY_Q,			//0x10
	KEY_W,			//0x11
	KEY_E,			//0x12
	KEY_R,			//0x13
	KEY_T,			//0x14
	KEY_Y,			//0x15
	KEY_U,			//0x16
	KEY_I,			//0x17
	KEY_O,			//0x18
	KEY_P,			//0x19
	KEY_LEFTBRACKET,//0x1a
	KEY_RIGHTBRACKET,//0x1b
	KEY_RETURN,		//0x1c
	KEY_LCTRL,		//0x1d
	KEY_A,			//0x1e
	KEY_S,			//0x1f
	KEY_D,			//0x20
	KEY_F,			//0x21
	KEY_G,			//0x22
	KEY_H,			//0x23
	KEY_J,			//0x24
	KEY_K,			//0x25
	KEY_L,			//0x26
	KEY_SEMICOLON,	//0x27
	KEY_QUOTE,		//0x28
	KEY_GRAVE,		//0x29
	KEY_LSHIFT,		//0x2a
	KEY_BACKSLASH,	//0x2b
	KEY_Z,			//0x2c
	KEY_X,			//0x2d
	KEY_C,			//0x2e
	KEY_V,			//0x2f
	KEY_B,			//0x30
	KEY_N,			//0x31
	KEY_M,			//0x32
	KEY_COMMA,		//0x33
	KEY_DOT,		//0x34
	KEY_SLASH,		//0x35
	KEY_RSHIFT,		//0x36
	KEY_KP_ASTERISK,//0x37
	KEY_RALT,		//0x38
	KEY_SPACE,		//0x39
	KEY_CAPSLOCK,	//0x3a
	KEY_F1,			//0x3b
	KEY_F2,			//0x3c
	KEY_F3,			//0x3d
	KEY_F4,			//0x3e
	KEY_F5,			//0x3f
	KEY_F6,			//0x40
	KEY_F7,			//0x41
	KEY_F8,			//0x42
	KEY_F9,			//0x43
	KEY_F10,		//0x44
	KEY_KP_NUMLOCK,	//0x45
	KEY_SCROLLLOCK,	//0x46
	KEY_HOME,		//0x47
	KEY_KP_8,		//0x48	//keypad up arrow
	KEY_PAGEUP,		//0x49
	KEY_KP_2,		//0x50	//keypad down arrow
	KEY_KP_3,		//0x51	//keypad page down
	KEY_KP_0,		//0x52	//keypad insert key
	KEY_KP_DECIMAL,	//0x53	//keypad delete key
	KEY_UNKNOWN,	//0x54
	KEY_UNKNOWN,	//0x55
	KEY_UNKNOWN,	//0x56
	KEY_F11,		//0x57
	KEY_F12			//0x58
};

const int INVALID_SCANCODE = 0;

u8int kb_ctrl_read_status() {
	return inb(KB_CTRL_STATS_REG);
}

void kb_ctrl_send_cmd(u8int cmd) {
	// wait for kb
	while (1) {
		if (!(kb_ctrl_read_status() & KB_CTRL_STATS_MASK_IN_BUF))
			break;
	}

	outb(KB_CTRL_CMD_REG, cmd);
}

u8int kb_enc_read_buf() {
	return inb(KB_ENC_INPUT_BUF);
}

void kb_enc_send_cmd(u8int cmd) {
	while (1) {
		if (!(kb_ctrl_read_status() & KB_CTRL_STATS_MASK_IN_BUF))
			break;
	}

	outb(KB_CTRL_CMD_REG, cmd);
}

void kb_handler(registers_t regs) {
	static u8int extended = 0;

	int code = 0;
	if (kb_ctrl_read_status() & KB_CTRL_STATS_MASK_OUT_BUF) {
	 	code = kb_enc_read_buf();

	 	if (code == 0xE0 || code == 0xE1) {
	 		extended = 1;
	 	} else {
	 		extended = 0;

	 		if (code & 0x80) {// test if break code (releases key)
	 			code -= 0x80; // convert to make code
	 			
	 			int key = kb_scancode_std[code];

	 			switch (key) {
	 				case KEY_LCTRL:
	 				case KEY_RCTRL:
	 					ctrl = 0;
	 					break;
	 				case KEY_LSHIFT:
	 				case KEY_RSHIFT:
	 					shift = 0;
	 					break;
	 				case KEY_LALT:
	 				case KEY_RALT:
	 					alt = 0;
	 					break;
	 				default:
	 					break;
	 			}	
	 		} else {
	 			scancode = code; // set the scancode

	 			int key = kb_scancode_std[code];

	 			switch (key) {
	 				case KEY_LCTRL:
					case KEY_RCTRL:
						ctrl = 1;
						break;

					case KEY_LSHIFT:
					case KEY_RSHIFT:
						shift = 1;
						break;

					case KEY_LALT:
					case KEY_RALT:
						alt = 1;
						break;

					case KEY_CAPSLOCK:
						capslock = (capslock) ? 0 : 1;
						kb_set_leds(numlock, capslock, scrolllock);
						break;

					case KEY_KP_NUMLOCK:
						numlock = (numlock) ? 0 : 1;
						kb_set_leds(numlock, capslock, scrolllock);
						break;

					case KEY_SCROLLLOCK:
						scrolllock = (scrolllock) ? 0 : 1;
						kb_set_leds(numlock, capslock, scrolllock);
						break;

					default:
						break;
	 			}
	 		}
	 	}

	 	// error checking
	 	switch (code) {
	 		case KB_ERR_BAT_FAILED:
	 			kb_bat_res = 0;
	 			break;

	 		case KB_ERR_DIAG_FAILED:
	 			kb_diag_res = 0;
	 			break;

	 		case KB_ERR_RESEND_CMD:
	 			kb_resend_res = 1;
	 			break;

	 		default:
	 			break;
	 	}
	}
}

u8int kb_get_scroll_lock() {
	return scrolllock;
}

u8int kb_get_caps_lock() {
	return capslock;
}

u8int kb_get_num_lock() {
	return numlock;
}

u8int kb_get_alt() {
	return alt;
}

u8int kb_get_ctrl() {
	return ctrl;
}

u8int kb_get_shift() {
	return shift;
}

void kb_ignore_resend() {
	kb_resend_res = 0;
}

u8int kb_check_resend() {
	return kb_resend_res;
}

u8int kb_get_diagnostic_res() {
	return kb_diag_res;
}

u8int kb_get_bat_res() {
	return kb_bat_res;
}

u8int kb_get_last_scan() {
	return scancode;
}

void kb_set_leds(u8int num, u8int caps, u8int scroll) {
	u8int data = 0;

	data = (scroll) ? (data | 1) : (data & 1);
	data = (num) ? (data | 2) : (data & 2);
	data = (caps) ? (data | 4) : (data & 4);

	kb_enc_send_cmd(KB_ENC_CMD_SET_LED);
	kb_enc_send_cmd(data);
}

char kb_get_last_key() {
	return (scancode != INVALID_SCANCODE) ? ((char)kb_scancode_std[scancode]) : (KEY_UNKNOWN);
}

void kb_discard_last_key() {
	scancode = INVALID_SCANCODE;
}

char kb_key_to_ascii(int code) {
	u8int key = code;

	//if (isascii(key)) {
		if (shift || capslock) {
			if (key >= 'a' && key <= 'z')
				key -= 32;
		}

		if (shift && !capslock) {
			if (key >= '0' && key <= '9') {
				switch (key) {
					case '0':
						key = KEY_RIGHTPARENTHESES;
						break;
					case '1':
						key = KEY_EXCLAMATION;
						break;
					case '2':
						key = KEY_AT;
						break;
					case '3':
						key = KEY_EXCLAMATION;
						break;
					case '4':
						key = KEY_HASH;
						break;
					case '5':
						key = KEY_PERCENT;
						break;
					case '6':
						key = KEY_CARRET;
						break;
					case '7':
						key = KEY_AMPERSAND;
						break;
					case '8':
						key = KEY_ASTERISK;
						break;
					case '9':
						key = KEY_LEFTPARENTHESES;
						break;
					default:
						break;
				}
			} else {
				switch (key) {
					case KEY_COMMA:
						key = KEY_LESS;
						break;

					case KEY_DOT:
						key = KEY_GREATER;
						break;

					case KEY_SLASH:
						key = KEY_QUESTION;
						break;

					case KEY_SEMICOLON:
						key = KEY_COLON;
						break;

					case KEY_QUOTE:
						key = KEY_DOUBLEQUOTE;
						break;

					case KEY_LEFTBRACKET :
						key = KEY_LEFTBRACE;
						break;

					case KEY_RIGHTBRACKET :
						key = KEY_RIGHTBRACE;
						break;

					case KEY_GRAVE:
						key = KEY_TILDE;
						break;

					case KEY_MINUS:
						key = KEY_UNDERSCORE;
						break;

					case KEY_PLUS:
						key = KEY_EQUAL;
						break;

					case KEY_BACKSLASH:
						key = KEY_PIPE;
						break;
				}
			}
		}

		if (key == KEY_RETURN)
			key = '\n';

		return key;
	//}

	return 0;
}

void kb_disable() {
	kb_ctrl_send_cmd(KB_CTRL_CMD_DISABLE);
	_kb_disable = 1;
}

void kb_enable() {
	kb_ctrl_send_cmd(KB_CTRL_CMD_ENABLE);
	_kb_disable = 0;
}

u8int kb_is_disabled() {
	return _kb_disable;
}

void kb_reset_system() {
	kb_ctrl_send_cmd(KB_CTRL_CMD_WRITE_OUT_PORT);
	kb_enc_send_cmd(0xFE);
}

u8int kb_self_test() {
	kb_ctrl_send_cmd(KB_CTRL_CMD_SELF_TEST);

	while (1) {
		if (kb_ctrl_read_status() & KB_CTRL_STATS_MASK_OUT_BUF)
			break;
	}

	return (kb_enc_read_buf() == 0x55) ? 1 : 0;
}

void kb_install_kb(int irq) {
	kprintf(K_WARN, "Installing KB\n");
	register_interrupt_handler(IRQ1, &kb_handler);

	kb_bat_res = 1;
	scancode = 0;

	numlock = scrolllock = capslock = 0;
	kb_set_leds(numlock, scrolllock, capslock);

	shift = ctrl = alt = 0;
}