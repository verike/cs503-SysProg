#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define BUFFER_SZ 50

// prototypes
void usage(char *);
void print_buff(char *, int);

// prototypes for functions to handle required functionality
int count_words(char *, int, int);
// add additional prototypes here

int setup_buff(char *, char *, int);
void reverse_buffer(char *, int, int);
int word_occurrence(char *, char *, int);

int setup_buff(char *buff, char *user_str, int len)
{
    // TODO: #4:  Implement the setup buff as per the directions
        int i = 0;
    while (i < len - 1 && user_str[i] != '\0') {
        buff[i] = user_str[i];
        i++;
    }
    buff[i] = '\0'; // Null-terminate
    return i;
    return 0; // for now just so the code compiles.
}

void print_buff(char *buff, int len)
{
    printf("Buffer:  ");
    for (int i = 0; i < len; i++)
    {
        putchar(*(buff + i));
    }
    putchar('\n');
}

void usage(char *exename)
{
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len)
{
    // YOU MUST IMPLEMENT
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if ((buff[i] != ' ') && (buff[i] != '\0')) {
            if (!in_word) {
                in_word = 1;
                count++;
            }
        } else {
            in_word = 0;
        }
    }
    return count;
    // return 0;
}


// ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS
void reverse_buffer(char *buff, int len, int str_len) {
    for (int i = 0, j = str_len - 1; i < j; i++, j--) {
        char temp = buff[i];
        buff[i] = buff[j];
        buff[j] = temp;
    }
    printf("Reversed buffer: %s\n", buff);
}
int word_occurrence(char *buff, char *word, int len) {
    int count = 0;
    char *pos = buff;
    while ((pos = strstr(pos, word)) != NULL) {
        count++;
        pos += strlen(word);
    }
    return count;
}



int main(int argc, char *argv[])
{

    char *buff;         // placehoder for the internal buffer
    char *input_string; // holds the string provided by the user on cmd line
    char opt;           // used to capture user option from cmd line
    int rc;             // used for return codes
    int user_str_len;   // length of user supplied string

    // TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //       PLACE A COMMENT BLOCK HERE EXPLAINING

    // Answer: Ensures the first command-line argument starts with a '-'
    //          (indicating an option). If it doesn't, the program displays the usage and exits.
    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); // get the option flag

    // handle the help flag and then exit normally
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(0);
    }

    // WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    // TODO:  #2 Document the purpose of the if statement below
    //       PLACE A COMMENT BLOCK HERE EXPLAINING
    // Answer: to ensure that users provide the required string after the option flag
    if (argc < 3)
    {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; // capture the user input string

    // TODO:  #3 Allocate space for the buffer using malloc and
    //           handle error if malloc fails by exiting with a
    //           return code of 99
    //  CODE GOES HERE FOR #3
    buff = (char *)malloc(BUFFER_SZ * sizeof(char));
    if (buff == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ); // see todos
    if (user_str_len < 0)
    {
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt)
    {
    case 'c':
        rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
        if (rc < 0)
        {
            printf("Error counting words, rc = %d", rc);
            exit(2);
        }
        printf("Word Count: %d\n", rc);
        break;

    // TODO:  #5 Implement the other cases for 'r' and 'w' by extending
    //        the case statement options
    case 'r':
    // Example: Reverse the buffer
    reverse_buffer(buff, BUFFER_SZ, user_str_len);
    break;

case 'w':
    // Example: Count a specific word occurrence in the buffer
    rc = word_occurrence(buff, "specific_word", BUFFER_SZ);
    printf("Occurrences: %d\n", rc);
    break;
    default:
        usage(argv[0]);
        exit(1);
    }

    // TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

// TODO:  #7  Notice all of the helper functions provided in the
//           starter take both the buffer as well as the length.  Why
//           do you think providing both the pointer and the length
//           is a good practice, after all we know from main() that
//           the buff variable will have exactly 50 bytes?
//
//           PLACE YOUR ANSWER HERE
//  Answer: Brcause by pasing both buff and len ensures the function
        //  operates within the buffer's bounds, avoiding overflows or 
        // segmentation faults.
        // Also for the sake of clrity and readability, you need to 
        // explicitly declare it.