/*
 * apple2.nib.c
 */

#include "apple2.enc.h"
#include "apple2.dd.h"
#include "vm_segment.h"

static vm_8bit gcr62[] = {
//  00    01    02    03    04    05    06    07    08    09    0a    0b    0c    0d    0e    0f 
    0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6, 0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
    0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
    0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

vm_segment *
apple2_enc_dos(vm_segment *src)
{
    vm_segment *dest;
    int i, doff = 0;

    // Use the nibbilized size for a 140k image file
    dest = vm_segment_create(_140K_NIB_);

    for (i = 0; i < 35; i++) {
        doff += apple2_enc_track(dest, src, doff, i);
    }

    return dest;
}

int
apple2_enc_track(vm_segment *dest, vm_segment *src, 
                 int doff, int track)
{
    int soff = track * 4096;
    int orig = doff;
    int sect, i;

    // We'll start off with some self-sync bytes to separate this track
    // from any other
    for (i = 0; i < 48; i++) {
        vm_segment_set(dest, doff++, 0xff);
    }

    for (sect = 0; sect < 16; sect++) {
        // Each sector has a header with some metadata, plus some
        // markers and padding.
        doff += apple2_enc_sector_header(dest, doff, track, sect);
        doff += apple2_enc_sector(dest, src, doff, soff);

        // We're moving here in 256-byte blocks
        soff += 256;
    }

    return doff - orig;
}

/*
 * Encode the src segment of image data (e.g. from a disk) with 6-and-2
 * encoding; this will copy one 256 byte block from src into a 343-byte
 * block into dest. We work the given destination offset and source
 * offset, but care must be taken to ensure that dest contains enough
 * room to hold the values from src.
 */
int
apple2_enc_sector(vm_segment *dest, vm_segment *src,
                  int doff, int soff)
{
    int i, di, orig;
    vm_8bit lastval, curval;

    orig = doff;
    
    // The init array contains the src segment's 256 bytes converted
    // into 342 bytes, but more works needs to be done to get it into
    // proper 6-and-2 encoding. The xor array will contain the XOR'd
    // version of init, but with an extra value tagged in as a checksum.
    vm_8bit init[0x156], xor[0x157];

    // This loop is really complicated; I'll annotate it as best I can.
    // To begin with, we mean to write the first 86 bytes for the
    // initial array.
    for (i = 0; i < 0x56; i++) {
        vm_8bit v = 0, vac, v56, v00;
        vm_8bit offac, off56;

        // We compute the offsets for the values beginning at ac and 56
        // here, specifically in 8bit variables, because we _want_ any
        // overflow to be handled within the offset rather than spilling
        // beyond the border of the segment.
        offac = i + 0xAC;
        off56 = i + 0x56;

        // We do the write by working with the src segment in rough
        // thirds. vac is the value offset by 0xAC, which is 0x56 * 2.
        // v56 is offset by 0x56, and v00 has no offset. In decimal
        // terms, vac is 172 bytes offset from 0, and v56 is 86 bytes
        // offset from 0.
        vac = vm_segment_get(src, soff + offac);
        v56 = vm_segment_get(src, soff + off56);
        v00 = vm_segment_get(src, soff + i);

        // The value we ultimately want to write into the dest segment
        // is then mangled a bit. v begins life as zero, of course; it's
        // then OR'd with vac's first and second bits, but in reverse
        // order as it were; that is, bit 0 is promoted to bit 1 in v,
        // and bit 1 in vac is demoted to bit 0 in v. This is repeated
        // twice more, with v56 and v00. We now have filled in six bits.
        v = (v << 2) | ((vac & 0x1) << 1) | ((vac & 0x2) >> 1);
        v = (v << 2) | ((v56 & 0x1) << 1) | ((v56 & 0x2) >> 1);
        v = (v << 2) | ((v00 & 0x1) << 1) | ((v00 & 0x2) >> 1);

        // We then write this into the dest segment, shifted twice more,
        // so that all the bits that may be high will begin at the
        // "left" side, from bit 7 - bit 2, leaving bit 1 and 0 at zero.
        init[i] = v << 2;
    }

    // The last two bytes written must be AND'd so that only the first
    // six bits can be high. But because the bit 1 and 0 are already
    // low, this has the effect of limiting the high bits to bits 5-2.
    init[i-2] &= 0x3F;
    init[i-1] &= 0x3F;

    // The rest of the bytes may be copied from the src buffer into dest
    // without modification. (Phew!)
    for (i = 0x00, di = 0x56; i < 0x100; i++, di++) {
        init[di] = vm_segment_get(src, soff+i);
    }

    // Here we will XOR each byte with each successive byte, and store
    // that into the xor array.
    for (i = 0, lastval = 0; i < 0x156; i++) {
        curval = init[i];
        xor[i] = curval ^ lastval;
        lastval = curval;
    }

    // But we need one more byte in the xor array; this is just the last
    // value from init.
    xor[i] = lastval;

    // This is the marker of the beginning of sector data
    vm_segment_set(dest, doff++, 0xd5);
    vm_segment_set(dest, doff++, 0xaa);
    vm_segment_set(dest, doff++, 0xad);

    // Sure... let's just toss in a few self-sync bytes
    for (i = 0; i < 6; i++) {
        vm_segment_set(dest, doff++, 0xff);
    }

    // Now we use the gcr table for 6-and-2 encoding to take the XOR'd
    // values and represent them as they should be in the destination
    // segment. This constitutes the data field of the sector.
    for (i = 0; i < 0x157; i++) {
        vm_segment_set(dest, doff++, gcr62[xor[i] >> 2]);
    }

    // These three bytes mark the end of the data field
    vm_segment_set(dest, doff++, 0xde);
    vm_segment_set(dest, doff++, 0xaa);
    vm_segment_set(dest, doff++, 0xeb);

    // At the conclusion of a sector, we write 27 self-sync bytes.
    for (i = 0; i < 27; i++) {
        vm_segment_set(dest, doff++, 0xff);
    }

    return doff - orig;
}

int
apple2_enc_4n4(vm_segment *seg, int off, vm_8bit val)
{
    vm_segment_set(seg, off, ((val >> 1) & 0x55) | 0xaa);
    vm_segment_set(seg, off+1, (val & 0x55) | 0xaa);
    return 2;
}

int
apple2_enc_sector_header(vm_segment *seg, int off, 
                                int track, int sect)
{
    int orig = off;

    vm_segment_set(seg, off++, 0xd5);
    vm_segment_set(seg, off++, 0xaa);
    vm_segment_set(seg, off++, 0x96);

    off += apple2_enc_4n4(seg, off, 0xfe);
    off += apple2_enc_4n4(seg, off, track);
    off += apple2_enc_4n4(seg, off, sect);
    off += apple2_enc_4n4(seg, off, 0xfe ^ track ^ sect);

    vm_segment_set(seg, off++, 0xde);
    vm_segment_set(seg, off++, 0xaa);
    vm_segment_set(seg, off++, 0xeb);

    return off - orig;
}
