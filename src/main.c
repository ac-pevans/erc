/*
 * main.c
 *
 * Here we define the main entry point for the program; we also define a
 * couple of functions to run when we start (init) and finish
 * (...finish).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "apple2/apple2.h"
#include "apple2/draw.h"
#include "apple2/event.h"
#include "log.h"
#include "option.h"
#include "vm_di.h"
#include "vm_screen.h"

/*
 * This function will establish the base environment that we want to use
 * while we execute.
 */
static void
init(int argc, char **argv)
{
    int options_ok;

    // If the option_parse() function returns zero, that means that it's
    // signaled to us that we should stop now. Whether that means we are
    // stopping in _error_ (bad input), or just because you asked for
    // --help, is not really specified. We exit with a non-zero error
    // code in any case.
    options_ok = option_parse(argc, argv);
    if (options_ok == 0) {
        const char *err = option_get_error();

        if (strlen(err) > 0) {
            fprintf(stderr, "%s\n", err);
            option_print_help();
        }

        exit(1);
    }

    vm_di_set(VM_OUTPUT, stdout);

    FILE *stream = fopen(LOG_FILENAME, "w");
    if (stream == NULL) {
        perror("Can't open log file");
    }

    log_open(stream);

    if (vm_screen_init() != OK) {
        fprintf(stderr, "Couldn't initialize video\n");
        exit(1);
    }
}

/*
 * And this is the teardown function.
 */
static void
finish()
{
    FILE *stream[3];

    stream[0] = (FILE *)vm_di_get(VM_DISK1);
    stream[1] = (FILE *)vm_di_get(VM_DISK2);
    stream[2] = (FILE *)vm_di_get(VM_DISASM_LOG);

    for (int i = 0; i < 3; i++) {
        if (stream[i]) {
            fclose(stream[i]);
        }
    }

    log_close();

    vm_screen_finish();
}

/*
 * This is what will run when the program begins, if you were new to how
 * C works.
 */
int
main(int argc, char **argv)
{
    apple2 *mach;
    vm_screen *screen;
    int err;

    init(argc, argv);

    // When we exit, we want to wrap up a few loose ends. This syscall
    // will ensure that `finish()` runs whether we return from main
    // successfully or if we run `exit()` from elsewhere in the program.
    atexit(finish);

    int *width = (int *)vm_di_get(VM_WIDTH);
    int *height = (int *)vm_di_get(VM_HEIGHT);

    // Let's build the basic machine, using the width and height
    // indicated by the user.
    mach = apple2_create(*width, *height);

    vm_di_set(VM_MACHINE, mach);

    // FIXME: eh; what if we have a machine which doesn't define the cpu
    // field? Alternatively, we could require all implemented machines
    // _to_ define a cpu field.
    vm_di_set(VM_CPU, mach->cpu);

    apple2_event_init();

    // Ok, it's time to boot this up!
    err = apple2_boot(mach);
    if (err != OK) {
        fprintf(stderr, "Bootup failed!\n");
        exit(1);
    }

    apple2_draw_40col(mach);

    // This will run for as long as we want to hang out in the emulated
    // machine.
    apple2_run_loop(mach);

    // We're all done, so let's tear everything down.
    apple2_free(mach);

    // ha ha ha ha #nervous #laughter
    printf("Hello, world\n");
}
