
#include "edit.h"

// Converts 4-byte synchsafe size into an integer.
static unsigned int get_synchsafe_size(const unsigned char *data)
{
    return ((unsigned int)(data[0] & 0x7F) << 21) |
           ((unsigned int)(data[1] & 0x7F) << 14) |
           ((unsigned int)(data[2] & 0x7F) << 7) |
           (unsigned int)(data[3] & 0x7F);
}

// Converts an integer into 4-byte synchsafe format.
static void put_synchsafe_size(unsigned char *data, unsigned int size)
{
    data[0] = (unsigned char)((size >> 21) & 0x7F);
    data[1] = (unsigned char)((size >> 14) & 0x7F);
    data[2] = (unsigned char)((size >> 7) & 0x7F);
    data[3] = (unsigned char)(size & 0x7F);
}

// Reads ID3v2.3 frame size (4 bytes).
static unsigned int get_frame_size_v23(const unsigned char *header)
{
    return ((unsigned int)header[4] << 24) |
           ((unsigned int)header[5] << 16) |
           ((unsigned int)header[6] << 8) |
           (unsigned int)header[7];
}

// Reads ID3v2.2 frame size (3 bytes).
static unsigned int get_frame_size_v22(const unsigned char *header)
{
    return ((unsigned int)header[3] << 16) |
           ((unsigned int)header[4] << 8) |
           (unsigned int)header[5];
}

// Stores an integer in 4-byte big-endian format.
static void put_be32(unsigned char *data, unsigned int value)
{
    data[0] = (unsigned char)(value >> 24);
    data[1] = (unsigned char)(value >> 16);
    data[2] = (unsigned char)(value >> 8);
    data[3] = (unsigned char)value;
}

// Selects the frame ID and field name based on ID3 version.
static int get_edit_details(int choice, int version,
                            char *frame_id,
                            const char **field_name)
{
    if (version == ID3V2_2)
    {
        switch (choice)
        {
            case 1:
                strcpy(frame_id, "TT2");
                *field_name = "Title";
                break;

            case 2:
                strcpy(frame_id, "TP1");
                *field_name = "Artist";
                break;

            case 3:
                strcpy(frame_id, "TAL");
                *field_name = "Album";
                break;

            case 4:
                strcpy(frame_id, "TYE");
                *field_name = "Year";
                break;

            case 5:
                strcpy(frame_id, "TCO");
                *field_name = "Genre";
                break;

            case 6:
                strcpy(frame_id, "TRK");
                *field_name = "Track";
                break;

            case 7:
                strcpy(frame_id, "COM");
                *field_name = "Comment";
                break;

            default:
                return FAILURE;
        }
    }
    else
    {
        switch (choice)
        {
            case 1:
                strcpy(frame_id, "TIT2");
                *field_name = "Title";
                break;

            case 2:
                strcpy(frame_id, "TPE1");
                *field_name = "Artist";
                break;

            case 3:
                strcpy(frame_id, "TALB");
                *field_name = "Album";
                break;

            case 4:
                strcpy(frame_id, "TYER");
                *field_name = "Year";
                break;

            case 5:
                strcpy(frame_id, "TCON");
                *field_name = "Genre";
                break;

            case 6:
                strcpy(frame_id, "TRCK");
                *field_name = "Track";
                break;

            case 7:
                strcpy(frame_id, "COMM");
                *field_name = "Comment";
                break;

            default:
                return FAILURE;
        }
    }

    return SUCCESS;
}

// Edits ID3v1 or ID3v1.1 metadata using user input.
static int edit_id3v1(MP3Info *mp3)
{
    FILE *fp;
    unsigned char tag[128];

    int choice;
    char new_value[MAX_TEXT];

    unsigned int offset;
    unsigned int field_size;

    // Opens the MP3 file in read and write mode.
    fp = fopen(mp3->mp3_file, "rb+");

    if (fp == NULL)
        return FAILURE;

    // Checks whether the MP3 file contains an ID3v1 tag.
    if (fseek(fp, -128, SEEK_END) != 0 ||
        fread(tag, 1, 128, fp) != 128 ||
        memcmp(tag, "TAG", 3) != 0)
    {
        fclose(fp);
        printf("ERROR: ID3v1 tag not found\n");
        return FAILURE;
    }

    // Displays the available metadata fields for editing.
    printf("\nEDIT MP3 TAG\n");
    printf("1. Title\n");
    printf("2. Artist\n");
    printf("3. Album\n");
    printf("4. Year\n");
    printf("5. Comment\n");
    printf("6. Genre ID\n");

    if (mp3->version == ID3V1_1)
        printf("7. Track\n");

    printf("Enter your choice: ");

    if (scanf("%d", &choice) != 1)
    {
        fclose(fp);
        return FAILURE;
    }

    while (getchar() != '\n')
    {
    }

    // Gets the selected field's offset and size.
    switch (choice)
    {
        case 1:
            offset = 3;
            field_size = 30;
            break;

        case 2:
            offset = 33;
            field_size = 30;
            break;

        case 3:
            offset = 63;
            field_size = 30;
            break;

        case 4:
            offset = 93;
            field_size = 4;
            break;

        case 5:
            offset = 97;
            field_size =
                (mp3->version == ID3V1_1) ? 28 : 30;
            break;

        case 6:
            printf("Enter genre ID (0-255): ");

            if (scanf("%d", &choice) != 1 ||
                choice < 0 || choice > 255)
            {
                fclose(fp);
                return FAILURE;
            }

            tag[127] = (unsigned char)choice;
            goto save_tag;

        case 7:
            if (mp3->version != ID3V1_1)
            {
                fclose(fp);
                printf("ERROR: Invalid choice\n");
                return FAILURE;
            }

            printf("Enter track number (1-255): ");

            if (scanf("%d", &choice) != 1 ||
                choice < 1 || choice > 255)
            {
                fclose(fp);
                return FAILURE;
            }

            tag[126] = (unsigned char)choice;
            goto save_tag;

        default:
            fclose(fp);
            printf("ERROR: Invalid choice\n");
            return FAILURE;
    }

    // Accepts a new metadata value from the user.
    printf("Enter new value: ");

    if (fgets(new_value, sizeof(new_value), stdin) == NULL)
    {
        fclose(fp);
        return FAILURE;
    }

    new_value[strcspn(new_value, "\n")] = '\0';

    // Clears the old field data and copies the new value.
    memset(tag + offset, 0, field_size);

    memcpy(tag + offset, new_value,
           strlen(new_value) < field_size ?
           strlen(new_value) : field_size);

save_tag:

    // Writes the modified ID3v1 tag back to the MP3 file.
    if (fseek(fp, -128, SEEK_END) != 0 ||
        fwrite(tag, 1, 128, fp) != 128)
    {
        fclose(fp);
        printf("ERROR: Unable to update ID3v1 tag\n");
        return FAILURE;
    }

    fclose(fp);

    printf("Metadata updated successfully\n");
    return SUCCESS;
}

// Edits ID3v1 metadata using command-line options.
static int edit_id3v1_option(MP3Info *mp3,
                             char option,
                             const char *value)
{
    FILE *fp;
    unsigned char tag[128];

    unsigned int offset = 0;
    unsigned int field_size = 0;

    int number;
    char *end;

    fp = fopen(mp3->mp3_file, "rb+");

    if (fp == NULL)
        return FAILURE;

    // Checks whether the MP3 file contains an ID3v1 tag.
    if (fseek(fp, -128, SEEK_END) != 0 ||
        fread(tag, 1, 128, fp) != 128 ||
        memcmp(tag, "TAG", 3) != 0)
    {
        fclose(fp);
        printf("ERROR: ID3v1 tag not found\n");
        return FAILURE;
    }

    // Identifies the metadata field using the command-line option.
    switch (option)
    {
        case 't':
            offset = 3;
            field_size = 30;
            break;

        case 'a':
            offset = 33;
            field_size = 30;
            break;

        case 'A':
            offset = 63;
            field_size = 30;
            break;

        case 'y':
            offset = 93;
            field_size = 4;
            break;

        case 'c':
            offset = 97;
            field_size =
                (mp3->version == ID3V1_1) ? 28 : 30;
            break;

        case 'g':
            // Validates the Genre ID range.
            number = (int)strtol(value, &end, 10);

            if (*value == '\0' || *end != '\0' ||
                number < 0 || number > 255)
            {
                fclose(fp);
                printf("ERROR: Genre must be from 0 to 255\n");
                return FAILURE;
            }

            tag[127] = (unsigned char)number;
            goto write_tag;

        case 'T':
            // Validates the Track number for ID3v1.1.
            if (mp3->version != ID3V1_1)
            {
                fclose(fp);
                printf("ERROR: Track is not available in ID3v1.0\n");
                return FAILURE;
            }

            number = (int)strtol(value, &end, 10);

            if (*value == '\0' || *end != '\0' ||
                number < 1 || number > 255)
            {
                fclose(fp);
                printf("ERROR: Track must be from 1 to 255\n");
                return FAILURE;
            }

            tag[126] = (unsigned char)number;
            goto write_tag;

        default:
            fclose(fp);
            return FAILURE;
    }

    // Clears the old field and copies the new value.
    memset(tag + offset, 0, field_size);

    memcpy(tag + offset, value,
           strlen(value) < field_size ?
           strlen(value) : field_size);

write_tag:

    // Writes the updated ID3v1 tag into the MP3 file.
    if (fseek(fp, -128, SEEK_END) != 0 ||
        fwrite(tag, 1, 128, fp) != 128)
    {
        fclose(fp);
        printf("ERROR: Unable to update ID3v1 tag\n");
        return FAILURE;
    }

    fclose(fp);

    printf("Metadata updated successfully\n");
    return SUCCESS;
}

// Edits ID3v2.2 or ID3v2.3 metadata.
static int edit_id3v2_file(MP3Info *mp3,
                           int requested_choice,
                           const char *requested_value)
{
    FILE *source = NULL;
    FILE *temporary = NULL;

    unsigned char header[10];

    unsigned char *old_tag = NULL;
    unsigned char *new_tag = NULL;

    unsigned int old_tag_size;
    unsigned int frame_header_size;

    unsigned int position = 0;
    unsigned int new_position = 0;
    unsigned int new_capacity;

    char selected_id[5] = {0};
    const char *field_name = NULL;

    char new_value[MAX_TEXT];

    unsigned char *new_frame = NULL;

    unsigned int new_frame_size;
    unsigned int payload_size;
    unsigned int id_size;
    unsigned int choice;

    int found = 0;

    char temp_name[MAX_FILE_NAME + 8];

    int result = FAILURE;

    // Opens the original MP3 file for reading.
    source = fopen(mp3->mp3_file, "rb");

    if (source == NULL)
    {
        printf("ERROR: Unable to open file\n");
        return FAILURE;
    }

    // Reads and validates the ID3v2 header.
    if (fread(header, 1, 10, source) != 10 ||
        memcmp(header, "ID3", 3) != 0)
        goto cleanup;

    // Checks unsupported ID3v2 tag flags.
    if ((header[5] & 0x80) ||
        (mp3->version == ID3V2_2 && (header[5] & 0x40)))
    {
        printf("ERROR: Unsupported ID3v2 tag flags\n");
        goto cleanup;
    }

    // Gets the existing ID3v2 tag size.
    old_tag_size = get_synchsafe_size(header + 6);

    // Allocates memory and reads the existing tag.
    old_tag = malloc(old_tag_size ? old_tag_size : 1);

    if (old_tag == NULL)
        goto cleanup;

    if (fread(old_tag, 1, old_tag_size, source) != old_tag_size)
        goto cleanup;

    // Skips the extended header when present in ID3v2.3.
    if (mp3->version == ID3V2_3 && (header[5] & 0x40))
    {
        unsigned int extended_size;

        if (old_tag_size < 4)
            goto cleanup;

        extended_size =
            ((unsigned int)old_tag[0] << 24) |
            ((unsigned int)old_tag[1] << 16) |
            ((unsigned int)old_tag[2] << 8) |
            (unsigned int)old_tag[3];

        if (extended_size > old_tag_size - 4)
            goto cleanup;

        position = 4 + extended_size;
    }

    // Gets the metadata field selected by the user.
    if (requested_choice == 0)
    {
        printf("\nEDIT MP3 TAG\n");
        printf("1. Title\n");
        printf("2. Artist\n");
        printf("3. Album\n");
        printf("4. Year\n");
        printf("5. Genre\n");
        printf("6. Track\n");
        printf("7. Comment\n");
        printf("Enter your choice: ");

        if (scanf("%u", &choice) != 1)
            goto cleanup;

        while (getchar() != '\n')
        {
        }
    }
    else
    {
        choice = (unsigned int)requested_choice;
    }

    if (get_edit_details((int)choice, mp3->version,
                         selected_id, &field_name) == FAILURE)
    {
        printf("ERROR: Invalid choice\n");
        goto cleanup;
    }

    // Reads the new metadata value.
    if (requested_value != NULL)
    {
        if (strlen(requested_value) >= sizeof(new_value))
        {
            printf("ERROR: New value is too long\n");
            goto cleanup;
        }

        strcpy(new_value, requested_value);
    }
    else
    {
        printf("Enter new %s: ", field_name);

        if (fgets(new_value, sizeof(new_value), stdin) == NULL)
            goto cleanup;

        new_value[strcspn(new_value, "\n")] = '\0';
    }

    if (new_value[0] == '\0')
    {
        printf("ERROR: Value cannot be empty\n");
        goto cleanup;
    }

    // Calculates the new frame header and payload sizes.
    id_size = (mp3->version == ID3V2_2) ? 3 : 4;

    frame_header_size =
        (mp3->version == ID3V2_2) ? 6 : 10;

    payload_size = 1 + (unsigned int)strlen(new_value);

    if (choice == 7)
        payload_size += 4;

    new_frame_size = frame_header_size + payload_size;

    // Allocates memory for the new metadata frame.
    new_frame = calloc(new_frame_size, 1);

    if (new_frame == NULL)
        goto cleanup;

    // Builds the new frame using the selected frame ID.
    memcpy(new_frame, selected_id, id_size);

    // Sets the frame size according to the ID3 version.
    if (mp3->version == ID3V2_2)
    {
        if (payload_size > 0xFFFFFF)
            goto cleanup;

        new_frame[3] = (unsigned char)(payload_size >> 16);
        new_frame[4] = (unsigned char)(payload_size >> 8);
        new_frame[5] = (unsigned char)payload_size;
    }
    else
    {
        put_be32(new_frame + 4, payload_size);
        new_frame[8] = 0;
        new_frame[9] = 0;
    }

    // Sets the text encoding and copies the new metadata value.
    new_frame[frame_header_size] = 0;

    if (choice == 7)
    {
        memcpy(new_frame + frame_header_size + 1, "eng", 3);
        new_frame[frame_header_size + 4] = 0;

        memcpy(new_frame + frame_header_size + 5,
               new_value, strlen(new_value));
    }
    else
    {
        memcpy(new_frame + frame_header_size + 1,
               new_value, strlen(new_value));
    }

    // Allocates memory for the updated ID3v2 tag.
    new_capacity = old_tag_size + new_frame_size + 16;

    new_tag = malloc(new_capacity);

    if (new_tag == NULL)
        goto cleanup;

    // Reads existing frames and copies unchanged frames.
    while (position + frame_header_size <= old_tag_size)
    {
        unsigned char *frame = old_tag + position;

        char current_id[5] = {0};

        unsigned int current_size;
        unsigned int total_size;

        if (frame[0] == 0)
            break;

        memcpy(current_id, frame, id_size);

        if (mp3->version == ID3V2_2)
            current_size = get_frame_size_v22(frame);
        else
            current_size = get_frame_size_v23(frame);

        total_size = frame_header_size + current_size;

        if (current_size == 0 ||
            total_size > old_tag_size - position)
            break;

        // Replaces the selected frame only once.
        if (strcmp(current_id, selected_id) == 0)
        {
            if (!found)
            {
                memcpy(new_tag + new_position,
                       new_frame, new_frame_size);

                new_position += new_frame_size;
                found = 1;
            }
        }
        else
        {
            memcpy(new_tag + new_position, frame, total_size);
            new_position += total_size;
        }

        position += total_size;
    }

    // Adds the new frame if the selected frame does not exist.
    if (!found)
    {
        memcpy(new_tag + new_position,
               new_frame, new_frame_size);

        new_position += new_frame_size;
    }

    // Creates a temporary file for the updated MP3 data.
    if (strlen(mp3->mp3_file) + 5 >= sizeof(temp_name))
        goto cleanup;

    snprintf(temp_name, sizeof(temp_name),
             "%s.tmp", mp3->mp3_file);

    temporary = fopen(temp_name, "wb");

    if (temporary == NULL)
        goto cleanup;

    // Updates the ID3 tag size in the header.
    put_synchsafe_size(header + 6, new_position);

    header[5] &= (unsigned char)~0x40;

    // Writes the updated header and metadata tag.
    if (fwrite(header, 1, 10, temporary) != 10 ||
        fwrite(new_tag, 1, new_position, temporary) != new_position)
        goto cleanup;

    // Copies the remaining MP3 audio data into the temporary file.
    {
        unsigned char buffer[4096];
        size_t bytes;

        while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0)
        {
            if (fwrite(buffer, 1, bytes, temporary) != bytes)
                goto cleanup;
        }

        if (ferror(source))
            goto cleanup;
    }

    if (fclose(temporary) != 0)
    {
        temporary = NULL;
        goto cleanup;
    }

    temporary = NULL;

    fclose(source);
    source = NULL;

    // Replaces the original MP3 file with the updated temporary file.
    if (rename(temp_name, mp3->mp3_file) != 0)
    {
        remove(temp_name);
        printf("ERROR: Unable to replace original MP3 file\n");
        goto cleanup;
    }

    printf("Metadata updated successfully\n");

    result = SUCCESS;

cleanup:

    // Closes files and releases allocated memory.
    if (source != NULL)
        fclose(source);

    if (temporary != NULL)
        fclose(temporary);

    free(old_tag);
    free(new_tag);
    free(new_frame);

    return result;
}

// Selects the editing function based on the ID3 version.
void edit_metadata(MP3Info *mp3)
{
    if (mp3->version == ID3V1 || mp3->version == ID3V1_1)
        edit_id3v1(mp3);

    else if (mp3->version == ID3V2_2 ||
             mp3->version == ID3V2_3)
        edit_id3v2_file(mp3, 0, NULL);

    else
        printf("ERROR: Unsupported ID3 version\n");
}

// Converts the command-line option into the corresponding metadata field.
int edit_metadata_option(MP3Info *mp3,
                         char option,
                         const char *value)
{
    int choice;

    if (value == NULL || value[0] == '\0')
    {
        printf("ERROR: New value is required\n");
        return FAILURE;
    }

    switch (option)
    {
        case 't':
            choice = 1;
            break;

        case 'a':
            choice = 2;
            break;

        case 'A':
            choice = 3;
            break;

        case 'y':
            choice = 4;
            break;

        case 'g':
            choice = 5;
            break;

        case 'T':
            choice = 6;
            break;

        case 'c':
            choice = 7;
            break;

        default:
            printf("ERROR: Invalid edit option\n");
            return FAILURE;
    }

    // Calls the appropriate ID3v1 or ID3v2 editing function.
    if (mp3->version == ID3V1 ||
        mp3->version == ID3V1_1)
        return edit_id3v1_option(mp3, option, value);

    if (mp3->version == ID3V2_2 ||
        mp3->version == ID3V2_3)
        return edit_id3v2_file(mp3, choice, value);

    return FAILURE;
}
