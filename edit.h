#ifndef EDIT_H
#define EDIT_H

#include "common.h"
#include "types.h"

void edit_metadata(MP3Info *mp3);
int edit_metadata_option(MP3Info *mp3, char option, const char *value);


#endif