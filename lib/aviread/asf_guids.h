#ifndef AVIFILE_GUIDS_H
#define AVIFILE_GUIDS_H

#include "avifmt.h"

typedef enum
{
    GUID_ERROR = 0,

    // base ASF objects
    GUID_ASF_HEADER,
    GUID_ASF_DATA,
    GUID_ASF_SIMPLE_INDEX,

    // header ASF objects
    GUID_ASF_FILE_PROPERTIES,
    GUID_ASF_STREAM_PROPERTIES,
    GUID_ASF_STREAM_BITRATE_PROPERTIES,
    GUID_ASF_CONTENT_DESCRIPTION,
    GUID_ASF_EXTENDED_CONTENT_ENCRYPTION,
    GUID_ASF_SCRIPT_COMMAND,
    GUID_ASF_MARKER,
    GUID_ASF_HEADER_EXTENSION,
    GUID_ASF_BITRATE_MUTUAL_EXCLUSION,
    GUID_ASF_CODEC_LIST,
    GUID_ASF_EXTENDED_CONTENT_DESCRIPTION,
    GUID_ASF_ERROR_CORRECTION,
    GUID_ASF_PADDING,

    // stream properties object stream type
    GUID_ASF_AUDIO_MEDIA,
    GUID_ASF_VIDEO_MEDIA,
    GUID_ASF_COMMAND_MEDIA,

    // stream properties object error correction type
    GUID_ASF_NO_ERROR_CORRECTION,
    GUID_ASF_AUDIO_SPREAD,

    // mutual exclusion object exlusion type
    GUID_ASF_MUTEX_BITRATE,
    GUID_ASF_MUTEX_UKNOWN,

    // header extension
    GUID_ASF_RESERVED_1,

    // script command
    GUID_ASF_RESERVED_SCRIPT_COMMNAND,

    // marker object
    GUID_ASF_RESERVED_MARKER,

    // various
    GUID_ASF_HEAD2,
    GUID_ASF_AUDIO_CONCEAL_NONE,
    GUID_ASF_CODEC_COMMENT1_HEADER,
    GUID_ASF_2_0_HEADER,

    GUID_END
} guidid_t;

guidid_t guid_get_guidid(const GUID* guid);
int guid_is_guidid(const GUID* guid, guidid_t gid);
//
char* guid_to_string(char* buffer, const GUID* guid);
const char* guidid_to_text(guidid_t g);

#endif // AVIFILE_GUIDS_H
