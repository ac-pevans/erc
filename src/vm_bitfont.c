/*
 * vm_bitfont.c
 *
 * The bitfont code allows us to define and work with a bitmapped font.
 * You can find the bitmap font glyphs in the `/fonts` subdir within the
 * repository root, as well as bmp files that are compiled from those
 * glyphs (via `/tools/build-fonts`).
 *
 * We do not have support for truetype fonts (nor other types of fonts)
 * at this time, but the glyph system is pretty easy to work with, even
 * if it is a hack.
 */

#include "vm_bitfont.h"

/*
 * Create a new bitfont from a given font name. This font name must
 * indicate something that exists in the filesystem relative to a point,
 * which means it must have been created during installation.
 *
 * This also indicates what the width and height of each glyph will be,
 * along with a character mask we will apply when figuring out where to
 * access bitmap data. (For example, if we only support 7-bit
 * characters, the cmask should limit us to 0-127 so that we can't
 * accidentally pass in a value that uses the 8th bit and crashes us
 * out.
 */
vm_bitfont *
vm_bitfont_create(vm_screen *screen, 
                  const vm_8bit *fontdata, int fontsize,
                  int width, int height, char cmask)
{
    SDL_Surface *surf;
    SDL_RWops *rw;
    vm_bitfont *font;

    // Build the RWops object from the given fontdata; we have to use
    // FromConstMem because we passed a const pointer into this
    // function.
    rw = SDL_RWFromConstMem(fontdata, fontsize);
    if (rw == NULL) {
        log_crit("Failed to create RWops from font data: %s", 
                     SDL_GetError());
        return NULL;
    }

    // And here we build a surface from the RWops, which is a nifty way
    // of getting a bitmap from memory rather than loading from a file.
    surf = SDL_LoadBMP_RW(rw, 0);
    if (surf == NULL) {
        log_crit("Failed to create bitmap from RWops: %s",
                     SDL_GetError());
        return NULL;
    }

    font = malloc(sizeof(vm_bitfont));
    if (font == NULL) {
        log_crit("Could not allocate memory for font");
        return NULL;
    }

    // The texture is what we can use to blit onto the renderer
    font->texture = SDL_CreateTextureFromSurface(screen->render, surf);

    font->width = width;
    font->height = height;
    font->cmask = cmask;

    return font;
}

/*
 * This just frees the memory for the font.
 */
void
vm_bitfont_free(vm_bitfont *font)
{
    SDL_DestroyTexture(font->texture);
    free(font);
}

/*
 * This function will provide the correct offset within the bitmap data
 * where you can find the glyph that matches the given character.
 * Because we want to note both an x and a y coordinate, we require
 * pointers to those, which we simply dereference to assign the right
 * value.
 */
void
vm_bitfont_offset(vm_bitfont *font, char ch, vm_area *area)
{
    int row = (ch & 0xf0) >> 4;
    int col = ch & 0x0f;

    area->xoff = col * font->width;
    area->yoff = row * font->height;
}

/*
 * Render the given character, in the given font, on the given screen at
 * the given destination.
 */
int
vm_bitfont_render(vm_bitfont *font, 
                  vm_screen *screen, 
                  vm_area *dest, 
                  char ch)
{
    vm_area src;

    // Our bitmap font may not be able to support all 256 possible
    // values that a character can hold; the cmask will limit us to
    // what's safe to query in the bitmap.
    ch = ch & font->cmask;

    // The width and height of the glyph are as indicated by the font
    // struct
    src.width = font->width;
    src.height = font->height;

    // Get the spot in the bitmap where the glyph is found
    vm_bitfont_offset(font, ch, &src);

    // Bring the destination attributes into the SDL_Rect we need to
    // pass into SDL_RenderCopy(). Also bring in the src attributes
    // we've built earlier.
    MAKE_SDL_RECT(dest_rect, *dest);
    MAKE_SDL_RECT(src_rect, src);

    if (screen->render) {
        if (SDL_RenderCopy(screen->render, font->texture, 
                           &src_rect, &dest_rect) < 0
           ) {
            log_crit("Failed to render glyph: %s", SDL_GetError());
            return ERR_GFXOP;
        }
    }

    screen->dirty = true;
    return OK;
}
