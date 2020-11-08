#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/// breaks the address of the pointer pointing to the next character into bytes and stores them in the 8 bytes left in the malloc result
void break_address_into_bytes(u_char *ptr, long ptr_address) {
    u_char *current = ptr + 8;
    for (int i = 0; i < 8; i++) {
        *current = (u_char) (ptr_address >> (8 * i) & 0xff);
        current--;
    }
}

/// frees the allocated space after dumping the data into fixed arrays
void sequential_deletion(u_char *head, size_t i) {
    u_char *ptr = NULL;
    u_char *delete = NULL;
    while (i--) {
        long buffer = 0;
        for (int j = 1; j < sizeof(char) + sizeof(head); j++) {
            long x = (long) *(head + j);
            buffer += x;
            buffer = buffer << ((sizeof(char) + sizeof(head) - 1) * (j != (sizeof(char) + sizeof(head) - 1)));
        }
        ptr = (u_char *) buffer;
        delete = head;
        head = ptr;
        free(delete);
    }
}

/// dumps the input from the allocated memory spaces into char arrays
void dump_data(const u_char *head, char command_string[], char input[], size_t command_length, size_t input_length) {
    command_string[0] = input[0] = *head;
    u_char *ptr = NULL;
    for (int i = 1; i < input_length; i++) {
        long buffer = 0;
        /// adds the next address byte by byte and shifts the buffer one byte each time except when j = 8
        for (int j = 1; j < sizeof(char) + sizeof(head); j++) {
            long x = (long) *(head + j);
            buffer += x;
            buffer = buffer << ((sizeof(char) + sizeof(head) - 1) * (j != (sizeof(char) + sizeof(head) - 1)));
        }
        ptr = (u_char *) buffer;
        input[i] = *ptr;
        if (i < command_length)
            command_string[i] = *ptr;
        head = ptr;
    }
}

int main() {
    u_char c;
    c = (u_char) getchar();
    /// one byte for the char and 8 for the address of the next pointer (see: break_address_into_bytes)
    u_char *head = (u_char *) malloc(sizeof(char) + sizeof(head)), *prev = NULL, *current = NULL;
    *head = c;
    current = head;
    bool leading_spaces = true; /// used to check if there a spaces before the command
    bool command_ended = false; /// used to check if the command has ended to differentiate between spaces between command and argument and spaces in the argument
    size_t command_length = 0; /// used to count the characters in the command for later use (stay tuned :D)
    size_t input_length = 0;
    for (;; input_length++) {
        if (leading_spaces && (c == ' ' || c == '\t'))
            continue;
        leading_spaces = false;
        if (!command_ended && c == ' ') {
            command_length = input_length;
            command_ended = true;
        }
        if (current != head) {
            *current = c;
            break_address_into_bytes(prev, (long) current);
        }
        c = (u_char) getchar();
        if (c == '\n')
            break;
        prev = current;
        current = (u_char *) malloc(sizeof(char) + sizeof(head));
    }
    input_length++;
    /// adding 1 to the array sizes to add \0 later
    char command_string[command_length + 1];
    char input[input_length + 1];
    dump_data(head, command_string, input, command_length, input_length);
    command_string[command_length] = '\0';
    input[input_length] = '\0';
    sequential_deletion(head, input_length);
    /*printf("%p\n", head);
    for(int input_length=0; input_length<9; input_length++)
        printf("%02x", *(head+input_length) & 0xff);*/
    return 0;
}

