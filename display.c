#include "display.h"

void display_metadata(MP3Info *mp3)
{
    printf("\n----------------------------------------\n");
    printf("           MP3 TAG INFORMATION\n");
    printf("----------------------------------------\n");

    printf("ID3 Version : ");

    switch (mp3->version)
    {
        case ID3V1:
            printf("ID3v1\n");
            break;

        case ID3V1_1:
            printf("ID3v1.1\n");
            break;

        case ID3V2_2:
            printf("ID3v2.2\n");
            break;

        case ID3V2_3:
            printf("ID3v2.3\n");
            break;
    }

    printf("Title       : %s\n", mp3->Title);
    printf("Artist      : %s\n", mp3->Artist);
    printf("Album       : %s\n", mp3->Album);
    printf("Year        : %d\n", mp3->year);
    printf("Genre       : %s\n", mp3->genre);

    if (mp3->track != 0)
        printf("Track       : %u\n", mp3->track);

    if (mp3->comment[0] != '\0')
        printf("Comment     : %s\n", mp3->comment);

    if (mp3->image_found)
    {
        printf("Image type  : %s\n", mp3->image_type);
        printf("Image path  : %s\n", mp3->image_path);
        printf("Image size  : %u bytes\n", mp3->image_size);
    }
    else
    {
        printf("Embedded image : Not found\n");
    }

    printf("----------------------------------------\n");
}