/*
 * vm_event.c
 *
 * The code here handles events for the virtual machine; it essentially
 * is a wrapper for SDL, presenting an interface to events that is
 * independent of SDL itself. You would handle keyboard events here,
 * mouse events, and more.
 */

#include "log.h"
#include "vm_di.h"
#include "vm_event.h"

/*
 * Look through all of the events that are queued up and, whatever we
 * need to do for them, do that.
 */
void
vm_event_poll(vm_screen *scr)
{
    vm_event ev;

    ev.screen = scr;
    while (SDL_PollEvent(&ev.event)) {
        if (ev.event.type == SDL_KEYDOWN || ev.event.type == SDL_KEYUP) {
            vm_event_keyboard(&ev);
        }
    }
}

/*
 * Handle any keyboard events from the event queue. Those would be
 * things like pressing a key, releasing a key... boring stuff, really.
 */
void
vm_event_keyboard(vm_event *ev)
{
    char ch;

    // The sym field is of type SDL_Keycode; this type, however,
    // maps roughly to Unicode, which of course maps roughly to
    // ASCII in the low range.
    ch = (char)ev->event.key.keysym.sym;

    switch (ev->event.type) {
        case SDL_KEYDOWN:
            ev->screen->dirty = true;
            ev->screen->key_pressed = true;
            vm_event_keyboard_normal(ev, ch);
            break;

        case SDL_KEYUP:
            // Note we do not erase the last_key value.
            ev->screen->key_pressed = false;
            vm_event_keyboard_special(ev, ch);

            break;

        default:
            break;
    }
}

/*
 * Handle the keyboard event for a normal (printable) character; this is
 * basically anything alphanumeric, but also includes symbols like $#?!
 * etc.
 *
 * This function only fires in the case of a KEYDOWN event.
 */
void
vm_event_keyboard_normal(vm_event *ev, char ch)
{
    // Basically, we only care about printable characters. Sorry to be
    // exclusionary to other characters! They are handled in other
    // functions.
    if (!isprint(ch)) {
        return;
    }

    // If we had shift pressed, we need to uppercase the
    // character.
    if (ev->event.key.keysym.mod & KMOD_LSHIFT ||
        ev->event.key.keysym.mod & KMOD_RSHIFT
       ) {
        ch = toupper(ch);
    }

    ev->screen->last_key = ch;
}

/*
 * Handle keyboard events for "special" characters, which are
 * essentially those which are not printable. ESC, RET, TAB...you get
 * the idea.
 *
 * Unlike the normal event, this function should only fire in the case
 * of a KEYUP event.
 */
void
vm_event_keyboard_special(vm_event *ev, char ch)
{
    int mod = ev->event.key.keysym.mod;

    if (mod & KMOD_ALT) {
        switch (ch) {
            case 'q':
                ev->screen->should_exit = true;
                break;

            case 'd':
                vm_event_do(VM_DEBUG_FUNC);
                break;

            case 'p':
                vm_event_do(VM_PAUSE_FUNC);
                break;
        }
    }
}

/*
 * Given a DI identifier, assume that we have registered a function that
 * does something with the also-registered machine object. If the result
 * of the lookup from difunc is NULL, return ERR_INVALID; otherwise,
 * execute the function and return OK.
 */
int
vm_event_do(int difunc)
{
    vm_event_fn func = (vm_event_fn)vm_di_get(difunc);
    void *mach = vm_di_get(VM_MACHINE);

    if (func == NULL) {
        return ERR_INVALID;
    }

    func(mach);
    return OK;
}
