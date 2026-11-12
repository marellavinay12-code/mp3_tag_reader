
#include "read.h"

// Get ID3v2 synchsafe size
static unsigned int get_synchsafe_size(const unsigned char *data)
{
    return ((unsigned int)(data[0] & 0x7F) << 21) |
           ((unsigned int)(data[1] & 0x7F) << 14) |
           ((unsigned int)(data[2] & 0x7F) << 7) |
           (unsigned int)(data[3] & 0x7F);
}

/* Get 3-byte big-endian size */
static unsigned int get_be24(const unsigned char *data)
{
    return ((unsigned int)data[0] << 16) |
           ((unsigned int)data[1] << 8) |
           (unsigned int)data[2];
}

/* Get 4-byte big-endian size */
static unsigned int get_be32(const unsigned char *data)
{
    return ((unsigned int)data[0] << 24) |
           ((unsigned int)data[1] << 16) |
           ((unsigned int)data[2] << 8) |
           (unsigned int)data[3];
}

/* Copy text frame data */
static void copy_text_frame(char *destination, size_t destination_size,
                            const unsigned char *source,
                            unsigned int source_size)
{
    unsigned int i;
    unsigned int out = 0;

    if (destination_size == 0)
        return;

    destination[0] = '\0';

    if (source_size < 1)
        return;

    /* UTF-16 encoding */
    if (source[0] == 1 || source[0] == 2)
    {
        int little_endian = 0;

        i = 1;

        /* UTF-16 with BOM */
        if (source[0] == 1)
        {
            if (source_size < 3)
                return;

            if (source[1] == 0xFF && source[2] == 0xFE)
            {
                little_endian = 1;
                i = 3;
            }
            else if (source[1] == 0xFE && source[2] == 0xFF)
            {
                little_endian = 0;
                i = 3;
            }
            else
            {
                return;
            }
        }
        /* UTF-16BE without BOM */
        else
        {
            little_endian = 0;
        }

        while (i + 1 < source_size && out + 1 < destination_size)
        {
            unsigned char ch;
            unsigned char high_byte;

            if (little_endian)
            {
                ch = source[i];
                high_byte = source[i + 1];
            }
            else
            {
                ch = source[i + 1];
                high_byte = source[i];
            }

            if (ch == 0 && high_byte == 0)
                break;

            if (high_byte == 0 && ch >= 32 && ch < 127)
                destination[out++] = (char)ch;
            else
                destination[out++] = '?';

            i += 2;
        }
    }
    /* ISO-8859-1 / UTF-8 */
    else
    {
        for (i = 1; i < source_size && out + 1 < destination_size; i++)
        {
            if (source[i] == 0)
                break;

            destination[out++] = (char)source[i];
        }
    }

    destination[out] = '\0';
}

/* Read embedded picture information */
static void read_picture_frame(MP3Info *mp3,
                               const unsigned char *data,
                               unsigned int size,
                               int version)
{
    unsigned int pos = 1;
    unsigned int description_end;
    unsigned char encoding;

    if (size < 5)
        return;

    encoding = data[0];

    /* ID3v2.2 PIC frame */
    if (version == ID3V2_2)
    {
        char format[4] = {0};

        memcpy(format, data + pos, 3);
        pos += 3;

        if (strcmp(format, "JPG") == 0 ||
            strcmp(format, "jpg") == 0)
        {
            strcpy(mp3->image_type, "image/jpeg");
        }
        else if (strcmp(format, "PNG") == 0 ||
                 strcmp(format, "png") == 0)
        {
            strcpy(mp3->image_type, "image/png");
        }
        else
        {
            snprintf(mp3->image_type,
                     sizeof(mp3->image_type), "%s", format);
        }
    }
    /* ID3v2.3 APIC frame */
    else
    {
        unsigned int mime_length = 0;

        while (pos + mime_length < size &&
               data[pos + mime_length] != 0)
        {
            mime_length++;
        }

        if (pos + mime_length >= size)
            return;

        if (mime_length >= sizeof(mp3->image_type))
            mime_length = sizeof(mp3->image_type) - 1;

        memcpy(mp3->image_type, data + pos, mime_length);
        mp3->image_type[mime_length] = '\0';

        pos += mime_length + 1;
    }

    if (pos >= size)
        return;

    pos++; /* Picture type */

    /* Skip picture description */
    if (encoding == 1 || encoding == 2)
    {
        description_end = pos;

        while (description_end + 1 < size &&
               !(data[description_end] == 0 &&
                 data[description_end + 1] == 0))
        {
            description_end += 2;
        }

        if (description_end + 1 >= size)
            return;

        pos = description_end + 2;
    }
    else
    {
        while (pos < size && data[pos] != 0)
            pos++;

        if (pos >= size)
            return;

        pos++;
    }

    if (pos >= size)
        return;

    mp3->image_size = size - pos;
    mp3->image_found = 1;

    snprintf(mp3->image_path, sizeof(mp3->image_path),
             "%.230s (embedded image)", mp3->mp3_file);
}

/* Validate MP3 file */
int validate_Mp3file(MP3Info *mp3)
{
    FILE *fp;
    long file_size;

    fp = fopen(mp3->mp3_file, "rb");

    if (fp == NULL)
    {
        printf("ERROR: Unable to open MP3 file\n");
        return FAILURE;
    }

    if (fseek(fp, 0, SEEK_END) != 0 ||
        (file_size = ftell(fp)) < 10)
    {
        printf("ERROR: Invalid or incomplete MP3 file\n");
        fclose(fp);
        return FAILURE;
    }

    fclose(fp);

    return SUCCESS;
}

/* Identify ID3 version */
int version_identify(MP3Info *mp3)
{
    FILE *fp;
    unsigned char header[10];
    unsigned char tail[128];

    fp = fopen(mp3->mp3_file, "rb");

    if (fp == NULL)
        return FAILURE;

    /* Check ID3v2 header */
    if (fread(header, 1, 10, fp) == 10 &&
        memcmp(header, "ID3", 3) == 0)
    {
        if (header[3] == 2)
        {
            mp3->version = ID3V2_2;
        }
        else if (header[3] == 3)
        {
            mp3->version = ID3V2_3;
        }
        else
        {
            printf("ERROR: Unsupported ID3v2 version: %u\n",
                   header[3]);

            fclose(fp);
            return FAILURE;
        }

        fclose(fp);
        return SUCCESS;
    }

    /* Check ID3v1 tag at end of file */
    if (fseek(fp, -128, SEEK_END) != 0 ||
        fread(tail, 1, 128, fp) != 128)
    {
        fclose(fp);
        printf("ERROR: ID3 tag not found\n");
        return FAILURE;
    }

    fclose(fp);

    if (memcmp(tail, "TAG", 3) != 0)
    {
        printf("ERROR: ID3 tag not found\n");
        return FAILURE;
    }

    if (tail[125] == 0 && tail[126] != 0)
        mp3->version = ID3V1_1;
    else
        mp3->version = ID3V1;

    return SUCCESS;
}

/* Read ID3v1 metadata */
int read_id3v1(MP3Info *mp3)
{
    FILE *fp;
    unsigned char tag[128];
    char year_text[5] = {0};

    fp = fopen(mp3->mp3_file, "rb");

    if (fp == NULL)
        return FAILURE;

    if (fseek(fp, -128, SEEK_END) != 0 ||
        fread(tag, 1, 128, fp) != 128)
    {
        fclose(fp);
        return FAILURE;
    }

    fclose(fp);

    if (memcmp(tag, "TAG", 3) != 0)
    {
        printf("ERROR: ID3v1 TAG marker not found\n");
        return FAILURE;
    }

    memcpy(mp3->Title, tag + 3, 30);
    mp3->Title[30] = '\0';

    memcpy(mp3->Artist, tag + 33, 30);
    mp3->Artist[30] = '\0';

    memcpy(mp3->Album, tag + 63, 30);
    mp3->Album[30] = '\0';

    memcpy(year_text, tag + 93, 4);
    mp3->year = atoi(year_text);

    if (mp3->version == ID3V1_1)
    {
        memcpy(mp3->comment, tag + 97, 28);
        mp3->comment[28] = '\0';

        mp3->track = tag[126];
    }
    else
    {
        memcpy(mp3->comment, tag + 97, 30);
        mp3->comment[30] = '\0';

        mp3->track = 0;
    }

    mp3->type_of_song = tag[127];

    snprintf(mp3->genre, sizeof(mp3->genre),
             "%u", tag[127]);

    return SUCCESS;
}

/* Read ID3v2 metadata */
int read_id3v2(MP3Info *mp3)
{
    FILE *fp;
    unsigned char header[10];
    unsigned char frame_header[10];
    unsigned char *frame_data = NULL;

    unsigned int tag_size;
    unsigned int frame_size;
    unsigned int header_size;

    long tag_end;
    int count = 0;

    fp = fopen(mp3->mp3_file, "rb");

    if (fp == NULL)
        return FAILURE;

    /* Read ID3 header */
    if (fread(header, 1, 10, fp) != 10 ||
        memcmp(header, "ID3", 3) != 0)
    {
        fclose(fp);
        return FAILURE;
    }

    /* Check unsupported tag flags */
    if ((header[5] & 0x80) ||
        (mp3->version == ID3V2_2 && (header[5] & 0x40)))
    {
        printf("ERROR: Unsupported ID3 tag flags\n");
        fclose(fp);
        return FAILURE;
    }

    /* Get total tag size */
    tag_size = get_synchsafe_size(header + 6);
    tag_end = 10L + (long)tag_size;

    if (fseek(fp, 0, SEEK_END) != 0 ||
        ftell(fp) < tag_end ||
        fseek(fp, 10, SEEK_SET) != 0)
    {
        fclose(fp);
        printf("ERROR: Invalid ID3 tag size\n");
        return FAILURE;
    }

    /* Handle ID3v2.3 extended header */
    if (mp3->version == ID3V2_3 && (header[5] & 0x40))
    {
        unsigned char ext_size_bytes[4];
        unsigned int ext_size;

        if (fread(ext_size_bytes, 1, 4, fp) != 4)
        {
            fclose(fp);
            return FAILURE;
        }

        ext_size = get_be32(ext_size_bytes);

        if (ext_size > (unsigned int)(tag_end - ftell(fp)))
        {
            fclose(fp);
            printf("ERROR: Invalid extended header size\n");
            return FAILURE;
        }

        if (fseek(fp, (long)ext_size, SEEK_CUR) != 0)
        {
            fclose(fp);
            return FAILURE;
        }
    }

    /* Frame header size */
    header_size = (mp3->version == ID3V2_2) ? 6 : 10;

    /* Read frames one by one */
    while (ftell(fp) + (long)header_size <= tag_end)
    {
        char frame_id[5] = {0};

        if (fread(frame_header, 1, header_size, fp) != header_size)
            break;

        /* Stop at padding */
        if (frame_header[0] == 0)
            break;

        /* Get frame ID and frame size */
        if (mp3->version == ID3V2_2)
        {
            memcpy(frame_id, frame_header, 3);
            frame_size = get_be24(frame_header + 3);
        }
        else
        {
            memcpy(frame_id, frame_header, 4);
            frame_size = get_be32(frame_header + 4);
        }

        /* Validate frame size */
        if (frame_size == 0 ||
            frame_size > (unsigned int)(tag_end - ftell(fp)))
        {
            break;
        }

        frame_data = malloc(frame_size);

        if (frame_data == NULL)
        {
            fclose(fp);
            return FAILURE;
        }

        if (fread(frame_data, 1, frame_size, fp) != frame_size)
        {
            free(frame_data);
            fclose(fp);
            return FAILURE;
        }

        /* Skip compressed or encrypted ID3v2.3 frames */
        if (mp3->version == ID3V2_3 &&
            (frame_header[9] & 0xC0))
        {
            free(frame_data);
            frame_data = NULL;
            continue;
        }

        /* Title */
        if (strcmp(frame_id, "TT2") == 0 ||
            strcmp(frame_id, "TIT2") == 0)
        {
            copy_text_frame(mp3->Title, sizeof(mp3->Title),
                            frame_data, frame_size);

            count++;
        }
        /* Artist */
        else if (strcmp(frame_id, "TP1") == 0 ||
                 strcmp(frame_id, "TPE1") == 0)
        {
            copy_text_frame(mp3->Artist, sizeof(mp3->Artist),
                            frame_data, frame_size);

            count++;
        }
        /* Album */
        else if (strcmp(frame_id, "TAL") == 0 ||
                 strcmp(frame_id, "TALB") == 0)
        {
            copy_text_frame(mp3->Album, sizeof(mp3->Album),
                            frame_data, frame_size);

            count++;
        }
        /* Year */
        else if (strcmp(frame_id, "TYE") == 0 ||
                 strcmp(frame_id, "TYER") == 0)
        {
            char year_text[16] = {0};

            copy_text_frame(year_text, sizeof(year_text),
                            frame_data, frame_size);

            mp3->year = atoi(year_text);

            count++;
        }
        /* Genre */
        else if (strcmp(frame_id, "TCO") == 0 ||
                 strcmp(frame_id, "TCON") == 0)
        {
            copy_text_frame(mp3->genre, sizeof(mp3->genre),
                            frame_data, frame_size);

            count++;
        }
        /* Track */
        else if (strcmp(frame_id, "TRK") == 0 ||
                 strcmp(frame_id, "TRCK") == 0)
        {
            char track_text[16] = {0};

            copy_text_frame(track_text, sizeof(track_text),
                            frame_data, frame_size);

            mp3->track = (unsigned char)atoi(track_text);

            count++;
        }
        /* Embedded picture */
        else if (strcmp(frame_id, "PIC") == 0 ||
                 strcmp(frame_id, "APIC") == 0)
        {
            read_picture_frame(mp3, frame_data,
                               frame_size, mp3->version);

            count++;
        }
        /* Comment */
        else if (strcmp(frame_id, "COM") == 0 ||
                 strcmp(frame_id, "COMM") == 0)
        {
            unsigned int pos = (frame_size > 4) ? 4 : frame_size;

            while (pos < frame_size && frame_data[pos] != 0)
                pos++;

            if (pos < frame_size)
                pos++;

            if (pos < frame_size)
            {
                unsigned int copy_size = frame_size - pos;

                if (copy_size >= sizeof(mp3->comment))
                    copy_size = sizeof(mp3->comment) - 1;

                memcpy(mp3->comment, frame_data + pos, copy_size);
                mp3->comment[copy_size] = '\0';
            }

            count++;
        }

        free(frame_data);
        frame_data = NULL;
    }

    fclose(fp);

    if (count == 0)
    {
        printf("ERROR: No supported metadata frames found\n");
        return FAILURE;
    }

    return SUCCESS;
}

/* Read complete MP3 metadata */
int read_metadata(MP3Info *mp3)
{
    memset(mp3->Title, 0, sizeof(mp3->Title));
    memset(mp3->Artist, 0, sizeof(mp3->Artist));
    memset(mp3->Album, 0, sizeof(mp3->Album));
    memset(mp3->genre, 0, sizeof(mp3->genre));
    memset(mp3->comment, 0, sizeof(mp3->comment));
    memset(mp3->image_type, 0, sizeof(mp3->image_type));
    memset(mp3->image_path, 0, sizeof(mp3->image_path));

    mp3->image_size = 0;
    mp3->image_found = 0;
    mp3->year = 0;
    mp3->track = 0;

    if (mp3->version == ID3V1 || mp3->version == ID3V1_1)
        return read_id3v1(mp3);

    if (mp3->version == ID3V2_2 || mp3->version == ID3V2_3)
        return read_id3v2(mp3);

    return FAILURE;
}
