#include "asffmt.h"
#include "utils.h"
#include "avm_output.h"

#include <string.h>
#include <stdio.h>

#define Debug if (0)

AVM_BEGIN_NAMESPACE;

asf_packet::asf_packet(uint_t psize)
    : avm::vector<unsigned char>(psize), refcount(1)
{
}

int asf_packet::init(uint_t timeshift)
{
    uint8_t* memory = &((*this)[0]);
    uint8_t* memory2 = memory + 1;

    // parse packet header
    //for (int i = 0 ; i < 15; i++) printf("%02x ", memory[i]); printf("\n");
    length = 0xff;
    if (memory[0] & 0x80)
    {
	if (memory[0] & 0x60)
	{
	    AVM_WRITE("ASF reader", "WARNING: unknown ErrorCorrectionLength 0x%x\n", memory[0]);
	    const unsigned char* me = memory + size() - 64;
	    while (memory < me)
		if (*++memory == 0x82
		    && memory[1] == 0 && memory[2] == 0)
			break;
            if (memory == me)
		return -1;
	    AVM_WRITE("ASF reader", "WARNING: resynced to 0x82\n");
	    //printf(" 0x%2x", memory[i]);
	    //printf("\n");
	}
	int edl = memory[0] & 0x0f;
	if (edl == 2)
	{
	    if (memory[1] != 0 || memory[2] != 0)
	    {
		AVM_WRITE("ASF reader", "WARNING: unexpected ErrorCorrection for 0x%x\n", memory[0]);
		return -1;
	    }
	}
        memory2 += edl; // skip Error Correction bytes (usually [0, 0])
    }
    else
        memory2++;

    length_flags = *memory2++;
    property_flags = *memory2++;

    int psize;
    int padding;

#define DO_2BITS(bits, var, defval) \
    switch (bits & 3) \
    { \
    case 3: var = avm_get_le32(memory2); memory2 += 4; break; \
    case 2: var = avm_get_le16(memory2); memory2 += 2; break; \
    case 1: var = memory2[0]; memory2++; break; \
    default: var = defval; break; \
    }

    DO_2BITS(length_flags >> 5, psize, size());
    DO_2BITS(length_flags >> 1, padding, 0); //sequence -> ignored
    DO_2BITS(length_flags >> 3, padding, 0);

    send_time = avm_get_le32(memory2);
    if (timeshift > 0)
    {
	send_time -= timeshift;
	avm_set_le32(memory2, send_time);
    }
    memory2 += 4;
    duration = avm_get_le16(memory2); memory2 += 2;

    //AVM_WRITE("ASF reader", "time %f   len %d\n", send_time/1000.0, duration);
    if (length_flags & 0x01)
    {
	segments = memory2[0] & 0x3f;
	segsizetype = memory2[0];
        memory2++;
    }
    else
    {
	segments = 1;
	segsizetype = 0x80;
    }
    //printf("PSIZE  %d   PAD %d   LEN %d\n", psize, padding, length);
    length = memory2 - memory;
    packet_length = psize - padding - length;
    Debug AVM_WRITE("ASF reader", 2, "Created packet header for send_time %f, length %d, %d segments\n",
		    send_time/1000., packet_length, segments);

    uint_t offset = length;
    // find all fragments in this packet
    for (int i = 0; i < segments; i++)
    {
	int l = segment(memory, offset, timeshift);
	if (l < 0)
	{
	    segments = i;
	    break;
	}
	offset += l;
	if (offset > size())
	{
	    AVM_WRITE("ASF reader", "WARNING: packet size overflow %d - %d\n", offset, size());
	    segments = i;
	    break;
	}
    }
    return 0;
}

int asf_packet::segment(uint8_t* memory, uint_t offset, uint_t timeshift)
{
    uint8_t* memory2 = memory + offset;
    uint8_t* timepos = 0;
    asf_packet_fragment f;
    f.keyframe = (memory2[0] & 0x80) >> 7;
    f.stream_id = memory2[0] & 0x7f;
    memory2++;

    uint_t fragment_offset; // == present_time;
    uint_t replicated_length;

    DO_2BITS(property_flags >> 4, f.seq_num, 0);
    timepos = memory2; // remmeber for time modification
    DO_2BITS(property_flags >> 2, fragment_offset, 0); // 0 is illegal
    DO_2BITS(property_flags, replicated_length, 0);

    Debug AVM_WRITE("ASF reader", 2, "StreamId: %3d  flags: 0x%x   seq: %d  offset: %d\n", f.stream_id, length_flags, f.seq_num, fragment_offset);

    if (replicated_length > 1)
    {
        // by the ASF spec it should contain at least these two elements
	f.object_length = avm_get_le32(memory2);
	f.object_start_time = avm_get_le32(memory2 + 4);
	if (timeshift > 0)
	{
	    f.object_start_time -= timeshift;
	    avm_set_le32(memory2 + 4, f.object_start_time);
	}
	if (replicated_length
	    > (uint_t)(packet_length - (memory2 - (memory + offset))))
            return -1;
        memory2 += replicated_length;
    }
    else
    {
	f.object_start_time = send_time;
	if (replicated_length == 1) // compressed
	    memory2++; // skip presentation time delta
    }

    uint_t plen;
    if (length_flags & 0x01) // multiple fragment
    {
        // read packet length
	DO_2BITS(segsizetype >> 6, plen, 0); // 0 is illegal
	if (plen > (uint_t)(packet_length - (memory2 - (memory + offset))))
	    return -1;
    }
    else // single fragment
        // determine remaing packet length
	plen = packet_length - (memory2 - (memory + offset));

    //AVM_WRITE("ASF reader", "repli  %d   plen %d\n", replicated_length, plen);
    if (replicated_length == 0x01) // compressed
    {
	f.fragment_offset = 0;

	// fragment_offset becomes object_start_time
	if (timeshift > 0)
	{
	    fragment_offset -= timeshift;
	    avm_set_le32(timepos, fragment_offset);
	}
	f.object_start_time = fragment_offset; 
	const uint8_t* pend = memory2 + plen;
	while (memory2 < pend)
	{
	    f.object_length = f.data_length = *memory2++;
	    //printf("********* ------ %d\n", f.data_length);
	    f.pointer = memory2;
	    memory2 += f.data_length;
	    fragments.push_back(f);
	}
    }
    else
    {
	f.fragment_offset = fragment_offset;
	f.data_length = plen;
	//printf("____%d\n", plen);
	f.pointer = memory2;
	memory2 += f.data_length;
	fragments.push_back(f);
    }
    //AVM_WRITE("ASF reader", 2, "Created segment header for stream %d,   %d fragments\n",
    //          f.stream_id, fragments.size());

    return memory2 - (memory + offset);
}

AVM_END_NAMESPACE;
