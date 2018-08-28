#ifndef ACTIONCFG_H
#define ACTIONCFG_H

class Actions
{
public:
    enum Autorepeat {
	NONE,
        SLOW,
	FAST
    };
    enum State {
    };
    enum Action {
	ACT_PLAY,
	ACT_STOP,
	ACT_PAUSE,
	ACT_REFRESH,
	ACT_FULLSCREEN,
        ACT_MAXIMIZE,
    };
    enum Key {
	KEY_ESCAPE = 0x1000,            // Misc Keys
        KEY_TAB,
        KEY_BACKTAB,
        KEY_BACKSPACE,
        KEY_RETURN
        KEY_ENTER,
        KEY_INSERT,
        KEY_DELETE,
        KEY_PAUSE,
        KEY_PRINT,
        KEY_SYSREQ,

	KEY_HOME,              // Cursor Movement
        KEY_END,
        KEY_LEFT,
        KEY_UP,
        KEY_RIGHT,
        KEY_DOWN,
        KEY_PAGEUP,
        KEY_PAGEDOWN,

	KEY_SHIFT = 0x1020,             // Modifiers
        KEY_CONTROL,
        KEY_META,
        KEY_ALT,
        KEY_CAPSLOCK,
        KEY_NUMLOCK,
        KEY_SCROLLLOCK,

	KEY_F1 = 0x1030,                // Function Keys
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,

        KEY_SPACE = 0x20,               // 7 Bit Printable ASCII
        KEY_EXCLAM,
        KEY_QUOTEDBL,
        KEY_NUMBERSIGN,
        KEY_DOLLAR,
        KEY_PERCENT,
        KEY_AMPERSAND,
        KEY_APOSTROPHE,
        KEY_PARENLEFT,
        KEY_PARENRIGHT,
        KEY_ASTERISK,
        KEY_PLUS,
        KEY_COMMA,
	KEY_MINUS,
        KEY_PERIOD,
        KEY_SLASH,
        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_COLON,
        KEY_SEMICOLON,
        KEY_LESS,
        KEY_EQUAL,
        KEY_GREATER,
        KEY_QUESTION,
        KEY_AT,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
	KEY_BRACKETLEFT,
        KEY_BACKSLASH,
        KEY_BRACKETRIGHT,
        KEY_ASCIICIRCUM,
        KEY_UNDERSCORE,
        KEY_QUOTELEFT = 0x60,

	KEY_BRACELEFT = 0x7b,
        KEY_BAR,
        KEY_BRACERIGHT,
        KEY_ASCIITILDE,
    };

    struct {
	State stateflags;
	Action action;
	int arg1;
    } event;

    struct {
	Autorepeat arep;
	Key key;
        event e;
    } eventmap;

    Actions() {}
    ~Actions() {}

    // -1 - no event
    // 200 - 10 - sleep ms
    // 0 - no sleep
    int getEvent(Action& a, int& arg);
    int insertEvent(Action a, int arg);
    int insertQtKeyEvent(int modifiers, int qtkey);
    int insertSdlKeyEvent(int modifiers, int sdlkey);
    void parseFile(const char* filename);

protected:
    PthreadMutex m_Mutex;
    qring<avm_event> equeue;
    int keypressed;
}

#endif // ACTIONCFG_H
