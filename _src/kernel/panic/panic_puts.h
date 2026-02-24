#pragma once


#define ANSI_BG_RED "\x1b[41m"
#define ANSI_CLEAR "\x1b[0m"
#define ANSI_CLS "\x1b[2J"
#define ANSI_HOME "\x1b[H"

#define ANSI_SAVE_CURSOR "\x1b[s"
#define ANSI_RESTORE_CURSOR_POS "\x1b[u"

#define ANSI_MOVE_CURSOR_RIGHT(n) "\x1b[" #n "C"
#define ANSI_MOVE_CURSOR_LEFT(n) "\x1b[" #n "D"

#define ANSI_ERASE_FROM_CURSOR_TO_END_OF_SCREEN "\x1b[0J"
#define ANSI_ERASE_LINE "\x1b[K"



