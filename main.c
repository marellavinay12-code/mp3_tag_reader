
#include "read.h"
#include "display.h"
#include "edit.h"

// Display the help menu
void display_help(void)
{
    printf("Options:\n");
    printf("  -t    Modifies a Title tag\n");
    printf("  -T    Modifies a Track tag\n");
    printf("  -a    Modifies an Artist tag\n");
    printf("  -A    Modifies an Album tag\n");
    printf("  -y    Modifies a Year tag\n");
    printf("  -c    Modifies a Comment tag\n");
    printf("  -g    Modifies a Genre tag\n");
    printf("  -h    Displays this help info");
}

// Command line arguments
int main(int argc, char *argv[])
{
    MP3Info mp3 = {0};
    int option = 0;
    char *file_name;

    // Check help option
    if (argc == 2 && strcmp(argv[1], "-h") == 0)
    {
        display_help();
        return SUCCESS;
    }

    // Check view option
    if (argc == 3 && strcmp(argv[1], "-v") == 0)
    {
        file_name = argv[2];
    }
    // Check edit option
    else if (argc == 4)
    {
        if (argv[1][0] != '-' || strlen(argv[1]) != 2)
        {
            printf("ERROR: Invalid option!\n");
            printf("Example: ./a.out -t \"New Title\" sample.mp3\n");
            return FAILURE;
        }

        option = argv[1][1];

        if (option != 't' && option != 'T' &&
            option != 'a' && option != 'A' &&
            option != 'y' && option != 'c' &&
            option != 'g')
        {
            printf("ERROR: Invalid option!\n");
            printf("Example: ./a.out -t \"New Title\" sample.mp3\n");
            return FAILURE;
        }

        file_name = argv[3];
    }
    else
    {
        printf("ERROR: Invalid arguments!\n");
        printf("Examples:\n");
        printf("  View metadata : ./a.out -v sample.mp3\n");
        printf("  Edit title    : ./a.out -t \"New Title\" sample.mp3\n");
        printf("  Help          : ./a.out -h\n");
        return FAILURE;
    }

    // Check file name length
    if (strlen(file_name) >= sizeof(mp3.mp3_file))
    {
        printf("ERROR: File name is too long!\n");
        return FAILURE;
    }

    strcpy(mp3.mp3_file, file_name);

    // Validate the MP3 file
    if (validate_Mp3file(&mp3) == FAILURE)
        return FAILURE;

    // Identify the ID3 version
    if (version_identify(&mp3) == FAILURE)
        return FAILURE;

    // Read metadata
    if (read_metadata(&mp3) == FAILURE)
        return FAILURE;

    // Display or edit metadata
    if (option != 0)
    {
        return edit_metadata_option(&mp3, option, argv[2]);
    }
    else
    {
        display_metadata(&mp3);
    }

    return SUCCESS;
}
