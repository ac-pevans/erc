/*
 * vm_screen.c
 *
 * Functions here support drawing to the virtual machine's "screen";
 * exactly how that is done is an abstraction to the rest of the
 * program, which only knows to call the functions here.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#include "log.h"
#include "vm_event.h"
#include "vm_screen.h"

struct timeval refresh_time;

/*
 * Initialize the video of the vm_screen abstraction. This ends up being
 * something that depends on our third-party graphics library; in other
 * words, what do we need to do before we can even create a drawing
 * surface to work with? If we are unable to properly do so, we return
 * with an error.
 */
int
vm_screen_init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        log_crit("Could not initialize video: %s", SDL_GetError());
        return ERR_GFXINIT;
    }

    memset(&refresh_time, 0, sizeof(struct timeval));

    return OK;
}

/*
 * Something to do in the teardown phase of the program, undoing
 * whatever we might have set up in the init function.
 */
void
vm_screen_finish()
{
    SDL_Quit();
}

/*
 * Return a new screen. This ends up blanking out whatever we have here;
 * we also do _not_ create any drawing surface, which is done separately
 * with the add_window function.
 */
vm_screen *
vm_screen_create()
{
    vm_screen *screen;

    screen = (vm_screen *)malloc(sizeof(vm_screen));
    if (screen == NULL) {
        log_crit("Failed to allocate vm_screen");
        exit(1);
    }

    screen->xcoords = 0;
    screen->ycoords = 0;
    screen->last_key = '\0';
    screen->key_pressed = false;
    screen->dirty = false;
    screen->should_exit = false;
    screen->usec_until_refresh = 33333;

    memset(&screen->reftime, 0, sizeof(struct timeval));

    screen->window = NULL;
    screen->render = NULL;

    return screen;
}

/*
 * The logical coordinates of a screen is a grid dimension separate from
 * what the literal window size of our drawing surface. For instance, a
 * machine may assume it has 320x240 pixels, but we are drawing on a
 * surface that is twice the size, 640x480. In effect we aim to decouple
 * the presumed drawing size from the actual drawing size, so the
 * machine we emulate does not need to think about it.
 */
void
vm_screen_set_logical_coords(vm_screen *screen, int xcoords, int ycoords)
{
    screen->xcoords = xcoords;
    screen->ycoords = ycoords;

    // We allow you to set the x and y coordinates in absence of a
    // screen renderer, but it's kind of pointless without one.
    if (screen->render) {
        SDL_RenderSetLogicalSize(screen->render, 
                                 screen->xcoords, 
                                 screen->ycoords);
    }
}

/*
 * This function will add a physical windowed, drawing surface to the
 * screen object. This ends up being done through the third-party
 * graphics library.
 */
int
vm_screen_add_window(vm_screen *screen, int width, int height)
{
    // If HEADLESS is defined, we will assume we _don't_ want an actual
    // drawing surface, but want to pretend we've added one. 
#ifndef HEADLESS
    SDL_CreateWindowAndRenderer(
        width, height, 0, &screen->window, &screen->render);

    if (screen->window == NULL || screen->render == NULL) {
        log_crit("Could not create window: %s", SDL_GetError());
        return ERR_GFXINIT;
    }
#endif

    // We plan to draw onto a surface that is xcoords x ycoords in area,
    // regardless of the actual size of the window.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    // We default to a logical coordinate system of exactly the given
    // width and height. For emulated systems like the Apple II, this
    // will not be correct, and the underlying machine system will rerun
    // the set_logical_coords function with different values.
    vm_screen_set_logical_coords(screen, width, height);

    vm_color clr;
    memset(&clr, 0, sizeof(vm_color));

    vm_screen_set_color(screen, clr);
    vm_screen_prepare(screen);

    return OK;
}

/*
 * Return the limit of the x coordinates our logical size can support.
 * (This is not the literal width of the window, but rather the width of
 * the drawing surface the machine will want to work with.)
 */
int
vm_screen_xcoords(vm_screen *screen)
{
    return screen->xcoords;
}

/*
 * Like the xcoords function in every way and meaning; except here we
 * return the y coordinate limit.
 */
int
vm_screen_ycoords(vm_screen *screen)
{
    return screen->ycoords;
}

/*
 * Free the contents of a screen.
 */
void
vm_screen_free(vm_screen *screen)
{
    SDL_DestroyRenderer(screen->render);
    SDL_DestroyWindow(screen->window);
    free(screen);
}

/*
 * Is the screen active? This function will be used by machines to
 * determine if there's still a need to continue their run loops.
 */
bool
vm_screen_active(vm_screen *scr)
{
    // If something happened in the event loop that caused the user to
    // signal an exit, then returning false here will do the trick
    if (scr->should_exit) {
        return false;
    }

    return true;
}

/*
 * Prepare the screen to be drawn and/or rendered. (Currently, this is
 * just a RenderClear in SDL.)
 */
void
vm_screen_prepare(vm_screen *scr)
{
    SDL_RenderClear(scr->render);
}

/*
 * Do whatever is required to refresh the screen with the changes we've
 * made recently.
 */
void
vm_screen_refresh(vm_screen *screen)
{
    SDL_RenderPresent(screen->render);
    screen->dirty = false;
}

/*
 * Set the color of a screen screen to a given RGBA value.
 */
void
vm_screen_set_color(vm_screen *scr, vm_color clr)
{
    if (scr->render) {
        SDL_SetRenderDrawColor(scr->render, clr.r, clr.g, clr.b,
                               SDL_ALPHA_OPAQUE);
    }
}

/*
 * Draw a rectangle on the screen at a given x/y position, with a given
 * set of x/y dimensions, with a given screen.
 */
void
vm_screen_draw_rect(vm_screen *screen, vm_area *area)
{
    // The renderer will take care of translating the positions and
    // sizes into whatever the window is really at.
    MAKE_SDL_RECT(rect, *area);

    SDL_RenderFillRect(screen->render, &rect);
    screen->dirty = true;
}

/*
 * Just a simple wrapper around the key pressed state (mostly in case we
 * ever switch from SDL to something else, and a field stops making
 * sense).
 */
bool
vm_screen_key_pressed(vm_screen *scr)
{
    return scr->key_pressed;
}

/*
 * Similar logic as for key_pressed; this is just a dumb getter for the
 * last_key field.
 */
char
vm_screen_last_key(vm_screen *scr)
{
    return scr->last_key;
}

bool
vm_screen_needs_frame(vm_screen *scr)
{
    struct timeval now;
    unsigned int diff;

    if (gettimeofday(&now, NULL) < 0) {
        log_crit("Failed call to gettimeofday()");
        return false;
    }

    diff = 
        ((now.tv_sec - scr->reftime.tv_sec) * 1000000) +
        (now.tv_usec - scr->reftime.tv_usec);

    if (diff > scr->usec_until_refresh) {
        memcpy(&scr->reftime, &now, sizeof(struct timeval));
        return true;
    }

    return false;
}

/*
 * Return true if the screen is considered dirty (i.e., if the screen
 * needs to be redrawn).
 */
bool
vm_screen_dirty(vm_screen *scr)
{
    if (vm_screen_needs_frame(scr)) {
        vm_event_poll(scr);
        return scr->dirty;
    }

    return false;
}
