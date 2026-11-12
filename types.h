#ifndef TYPES_H
#define TYPES_H

#include "common.h"

enum
{
    ID3V1 = 1,
    ID3V1_1,
    ID3V2_2,
    ID3V2_3
};

typedef struct mp3_information
{
    char mp3_file[MAX_FILE_NAME];
    char Title[MAX_TEXT];
    char Artist[MAX_TEXT];
    char Album[MAX_TEXT];
    int year;
    char genre[MAX_TEXT];
    unsigned char type_of_song;
    unsigned char track;
    int version;
    char comment[MAX_TEXT];
    char image_type[16];
    char image_path[MAX_FILE_NAME];
    unsigned int image_size;
    int image_found;

} MP3Info;

#endif