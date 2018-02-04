#include <criterion/criterion.h>

#include "apple2.dd.h"

static apple2dd *drive;

static void
setup()
{
    drive = apple2_dd_create();
}

static void
teardown()
{
    apple2_dd_free(drive);
}

TestSuite(apple2_dd, .init = setup, .fini = teardown);

Test(apple2_dd, create)
{
    cr_assert_eq(drive->data, NULL);
    cr_assert_eq(drive->image, NULL);
    cr_assert_eq(drive->image_type, DD_NOTYPE);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 0);
    cr_assert_eq(drive->online, false);
    cr_assert_eq(drive->write_protect, true);
    cr_assert_eq(drive->mode, DD_READ);
}

/* Test(apple2_dd, free) */

Test(apple2_dd, insert)
{
    FILE *stream;

    // In a successful drive open, we would also reset the track and
    // sector pos.
    drive->track_pos = 123;
    drive->sector_pos = 33;

    stream = fopen("../data/zero.img", "r");
    cr_assert_eq(apple2_dd_insert(drive, stream, DD_DOS33), OK);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 0);
    cr_assert_eq(drive->image_type, DD_DOS33);
    fclose(stream);

    stream = fopen("../data/bad.img", "r");
    cr_assert_eq(apple2_dd_insert(drive, stream, DD_DOS33), ERR_BADFILE);
    fclose(stream);
}

Test(apple2_dd, position)
{
    // Without any data, the drive should return a null position
    // regardless of track position
    drive->track_pos = 3;
    drive->sector_pos = 44;
    cr_assert_eq(apple2_dd_position(drive), 0);

    // FIXME: we need some dummy data for the drive...
}

Test(apple2_dd, read)
{
    drive->data = vm_segment_create(_140K_);
    vm_segment_set(drive->data, 0, 123);
    vm_segment_set(drive->data, 1, 234);

    cr_assert_eq(apple2_dd_read(drive), 123);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 1);

    cr_assert_eq(apple2_dd_read(drive), 234);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 2);
}

Test(apple2_dd, eject)
{
    drive->data = vm_segment_create(1000);
    apple2_dd_eject(drive);
    cr_assert_eq(drive->data, NULL);
}

Test(apple2_dd, set_mode)
{
    apple2_dd_set_mode(drive, DD_WRITE);
    cr_assert_eq(drive->mode, DD_WRITE);
    apple2_dd_set_mode(drive, DD_READ);
    cr_assert_eq(drive->mode, DD_READ);

    // let's try shenanigans
    apple2_dd_set_mode(drive, 111111111);
    cr_assert_eq(drive->mode, DD_READ);
}

Test(apple2_dd, shift)
{
    apple2_dd_shift(drive, 5);
    cr_assert_eq(drive->sector_pos, 5);

    // Push it beyond the sector boundary; see if the track position
    // updates as it should.
    apple2_dd_shift(drive, MAX_SECTOR_POS + 3);
    cr_assert_eq(drive->track_pos, 2);

    // this should be the mod of sector_pos and MAX_SECTOR_POS
    cr_assert_eq(drive->sector_pos, 7);
}

Test(apple2_dd, step)
{
    // Does step work at all?
    apple2_dd_step(drive, 5);
    cr_assert_eq(drive->track_pos, 5);
    apple2_dd_step(drive, 3);
    cr_assert_eq(drive->track_pos, 8);
    apple2_dd_step(drive, -2);
    cr_assert_eq(drive->track_pos, 6);

    // Do we handle going over the maximum track position properly?
    apple2_dd_step(drive, 100);
    cr_assert_eq(drive->track_pos, MAX_DRIVE_STEPS);

    // Do we handle going to the 0 track properly if we get a radically
    // high number of negative track shifts?
    apple2_dd_step(drive, -1000);
    cr_assert_eq(drive->track_pos, 0);
}

Test(apple2_dd, turn_on)
{
    apple2_dd_turn_on(drive, true);
    cr_assert(drive->online);
    apple2_dd_turn_on(drive, false);
    cr_assert(!drive->online);

    // I mean, ok
    apple2_dd_turn_on(drive, 1111333);
    cr_assert(drive->online);
}

Test(apple2_dd, write)
{
    drive->data = vm_segment_create(_140K_);

    drive->latch = 123;
    apple2_dd_write(drive);
    cr_assert_eq(vm_segment_get(drive->data, 0), 123);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 1);

    drive->latch = 234;
    apple2_dd_write(drive);
    cr_assert_eq(vm_segment_get(drive->data, 1), 234);
    cr_assert_eq(drive->track_pos, 0);
    cr_assert_eq(drive->sector_pos, 2);
}

Test(apple2_dd, write_protect)
{
    apple2_dd_write_protect(drive, true);
    cr_assert(drive->write_protect);
    apple2_dd_write_protect(drive, false);
    cr_assert(!drive->write_protect);
    apple2_dd_write_protect(drive, 2222);
    cr_assert(drive->write_protect);
}
