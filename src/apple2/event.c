/*
 * apple2.reflect.c
 *
 * Implement the reflection handlers for the virtual machine when the
 * apple2 machine is being emulated.
 */

#include "apple2/apple2.h"
#include "apple2/event.h"
#include "vm_di.h"

/*
 * Initialize the reflection struct for the apple2 machine, setting up
 * all of the reflect methods we may want to use.
 */
void
apple2_event_init()
{
    vm_di_set(VM_PAUSE_FUNC, apple2_event_pause);
    vm_di_set(VM_DEBUG_FUNC, apple2_event_debug);
}

/*
 * Pause the running state of the apple2 machine
 */
EVENT_DO(apple2_event_pause)
{
    apple2 *mach = (apple2 *)_mach;
    mach->paused = !mach->paused;
}

/*
 * Enter into a debugging state. This also implies a pause in execution
 * (which can be resumed in the debugger).
 */
EVENT_DO(apple2_event_debug)
{
    apple2 *mach = (apple2 *)_mach;
    mach->debug = true;
    mach->paused = true;
}
