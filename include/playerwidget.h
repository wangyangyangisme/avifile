#ifndef AVIFILE_PLAYERWIDGET_H
#define AVIFILE_PLAYERWIDGET_H
/**
 *  Implement this interface if you wish to manually handle
 *  playback-controlling events that come from movie window
 *  of IAviPlayer2 object. For example, you might want
 *  some kind of configuration popup menu ( implemented
 *  with your favorite widget toolkit ) to show up when
 *  user right-clicks in the movie window. You'll have to
 *  put the code for creation of popup menu into implementation
 *  of PW_menu_slot().
 *  Members of this interface will be called from within movie
 *  window event thread.
 **/

AVM_BEGIN_NAMESPACE;

class IPlayerWidget
{
public:
    enum Action {
        // key actions

	A_QUIT = 5555,
        // window actions
	A_RESIZE, A_REFRESH,

        // mouse actions
	A_MOUSE_LEFT, A_MOUSE_MIDDLE, A_MOUSE_RIGHT,
	A_MOUSE_WHEEL_UP, A_MOUSE_WHEEL_DOWN

    };
    // status of keyboard or other potentical modifiers
    enum Mod {
	M_NONE  = 0x0000,
	M_LSHIFT= 0x0001,
	M_RSHIFT= 0x0002,
	M_LCTRL = 0x0040,
	M_RCTRL = 0x0080,
	M_LALT  = 0x0100,
	M_RALT  = 0x0200,
	M_LMETA = 0x0400,
	M_RMETA = 0x0800,
	M_NUM   = 0x1000,
	M_CAPS  = 0x2000,
	M_MODE  = 0x4000,

	M_PRESSED = 0x8000,

	M_CTRL = M_LCTRL | M_RCTRL,
	M_SHIFT = M_LSHIFT | M_RSHIFT,
	M_ALT = M_LALT | M_RALT,
        M_META = M_LMETA | M_RMETA
    };
    // will be removed
    virtual ~IPlayerWidget() {}
    /* Unused */
    virtual void PW_showconf_func() {};
    /* Middle button click.
     Default action: switch zoom 0.5x->1x->2x
    */
    virtual void PW_middle_button() {};
    /* Key 'x' pressed.
     Default action: stop playback.
    */
    virtual void PW_stop_func() {};
    /* Key 'c' pressed.
     Default action: pause/resume playback.
     */     
    virtual void PW_pause_func() {};
    /* Key 'v' pressed.
     Default action: start playback after stop or pause.
     */
    virtual void PW_play_func() {};
    /* Key 'q' pressed.
     Default action: do nothing.
     */
    virtual void PW_quit_func() {};
    /* Right button click.
     Default action: do nothing.
     */
    virtual void PW_menu_slot() {};
    /* 'Esc' or 'Alt+Enter' pressed.
     Default action: toggle fullscreen mode.
     */
    virtual void PW_fullscreen() {};
    /* Movie window is resized with mouse.
     Default action: resize picture.
     */
    virtual void PW_maximize_func() {};

    virtual void PW_resize(int w, int h) {};
    virtual void PW_refresh()  {};
    virtual void PW_key_func(int sym, int mod) {};

    // or maybe use some Event structure with timestamps...
    //virtual void action(Action act, Mod mod,
    //    		int arg1, int arg2, int arg3, int arg4) {};
};

AVM_END_NAMESPACE;

typedef avm::IPlayerWidget PlayerWidget;

#endif // AVIFILE_PLAYERWIDGET_H
