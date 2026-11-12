#ifndef READ_H
#define READ_H

#include "common.h"
#include "types.h"

int validate_Mp3file(MP3Info *mp3);
int version_identify(MP3Info *mp3);
int read_metadata(MP3Info *mp3);
int read_id3v1(MP3Info *mp3);
int read_id3v2(MP3Info *mp3);

#endif