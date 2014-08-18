// keyboard.h -- Brad Slayter

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "lib/common.h"

// ASCII keycodes

enum KEYCODE { 
	KEY_SPACE = ' ',
	KEY_1 = '1',		// NUMBERS
	KEY_2 = '2',
	KEY_3 = '3',
	KEY_4 = '4',
	KEY_5 = '5',
	KEY_6 = '6',
	KEY_7 = '7',
	KEY_8 = '8',
	KEY_9 = '9',
	KEY_0 = '0',

	KEY_A = 'a',
	KEY_B = 'b',		// LETTERS
	KEY_C = 'c',
	KEY_D = 'd',
	KEY_E = 'e',
	KEY_F = 'f',
	KEY_G = 'g',
	KEY_H = 'h',
	KEY_I = 'i',
	KEY_J = 'j',
	KEY_K = 'k',
	KEY_L = 'l',
	KEY_M = 'm',
	KEY_N = 'n',
	KEY_O = 'o',
	KEY_P = 'p',
	KEY_Q = 'q',
	KEY_R = 'r',
	KEY_S = 's',
	KEY_T = 't',
	KEY_U = 'u',
	KEY_V = 'v',
	KEY_W = 'w',
	KEY_X = 'x',
	KEY_Y = 'y',
	KEY_Z = 'z',

	KEY_RETURN = '\n',
	KEY_ESCAPE = 0x1001,
	KEY_BACKSPACE = '\b',

	KEY_UP = 0x1100,		// ARROW KEYS
	KEY_DOWN = 0x1101,
	KEY_LEFT = 0x1102,
	KEY_RIGHT = 0x1103,

	KEY_F1 = 0x1201,		// FUNCTION KEYS
	KEY_F2 = 0x1202,
	KEY_F3 = 0x1203,
	KEY_F4 = 0x1204,
	KEY_F5 = 0x1205,
	KEY_F6 = 0x1206,
	KEY_F7 = 0x1207,
	KEY_F8 = 0x1208,
	KEY_F9 = 0x1209,
	KEY_F10 = 0x120A,
	KEY_F11 = 0x120B,
	KEY_F12 = 0x120C,
	KEY_F13 = 0x120D,
	KEY_F14 = 0x120E,
	KEY_F15 = 0x120F,

	KEY_DOT = '.',					// SYMBOLS
	KEY_COMMA = ',',
	KEY_COLON = ':',
	KEY_SEMICOLON = ';',
	KEY_SLASH = '/',
	KEY_BACKSLASH = '\\',
	KEY_PLUS = '+',
	KEY_MINUS = '-',
	KEY_ASTERISK = '*',
	KEY_EXCLAMATION = '!',
	KEY_QUESTION = '?',
	KEY_DOUBLEQUOTE = '\"',
	KEY_QUOTE = '\'',
	KEY_EQUAL = '=',
	KEY_HASH = '#',
	KEY_PERCENT = '%',
	KEY_AMPERSAND = '&',
	KEY_UNDERSCORE = '_',
	KEY_LEFTPARENTHESES = '(',
	KEY_RIGHTPARENTHESES = ')',
	KEY_LEFTBRACKET = '[',
	KEY_RIGHTBRACKET = ']',
	KEY_LEFTBRACE = '{',
	KEY_RIGHTBRACE = '}',
	KEY_DOLLAR = '$',
	KEY_LESS = '<',
	KEY_GREATER = '>',
	KEY_PIPE = '|',
	KEY_GRAVE = '`',
	KEY_TILDE = '~',
	KEY_AT = '@',
	KEY_CARRET = '^',

	KEY_KP_0 = '0',					// NUMPAD
	KEY_KP_1 = '1',
	KEY_KP_2 = '2',
	KEY_KP_3 = '3',
	KEY_KP_4 = '4',
	KEY_KP_5 = '5',
	KEY_KP_6 = '6',
	KEY_KP_7 = '7',
	KEY_KP_8 = '8',
	KEY_KP_9 = '9',
	KEY_KP_PLUS = '+',
	KEY_KP_MINUS = '-',
	KEY_KP_DECIMAL = '.',
	KEY_KP_DIVIDE = '/',
	KEY_KP_ASTERISK = '*',
	KEY_KP_NUMLOCK = 0x300F,
	KEY_KP_ENTER = 0x3010,

	KEY_TAB = 0x4000,
	KEY_CAPSLOCK = 0x4001,

	KEY_LSHIFT = 0x4002,			// MODIFIERS
	KEY_LCTRL = 0x4003,
	KEY_LALT = 0x4004,
	KEY_LSUPER = 0x4005,
	KEY_RSHIFT = 0x4006,
	KEY_RCTRL = 0x4007,
	KEY_RALT = 0x4008,
	KEY_RSUPER = 0x4009,

	KEY_INSERT = 0x400A,
	KEY_DELETE = 0x400B,
	KEY_HOME = 0x400C,
	KEY_END = 0x400D,
	KEY_PAGEUP = 0x400E,
	KEY_PAGEDOWN = 0x400F,
	KEY_SCROLLLOCK = 0x4010,
	KEY_PAUSE = 0x4011,

	KEY_UNKNOWN,
	KEY_NUMKEYCODES
};

// Get lock key states
u8int kb_get_scroll_lock();
u8int kb_get_num_lock();
u8int kb_get_caps_lock();

// Get modifier status
u8int kb_get_alt();
u8int kb_get_ctrl();
u8int kb_get_shift();

// resend last command
void kb_ignore_resend();
u8int kb_check_resend();

// return/run self tests
u8int kb_get_diagnostic_result();
u8int kb_get_bat_result();
u8int kb_self_test();

// get last scan code and keystroke
u8int kb_get_last_scan();
char kb_get_last_key();
void kb_discard_last_key();

// update LEDs
void kb_set_leds(u8int num, u8int caps, u8int scroll);

// convert keycode to ascii
char kb_key_to_ascii(int kc);

// enable/disable kb
void kb_disable();
void kb_enable();
u8int kb_is_disabled();

void kb_reset_system();

void kb_install_kb();

#endif