#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

/// breaks the address of the pointer pointing to the next character into bytes
/// and stores them in the 8 bytes left in the malloc result
void break_address_into_bytes(u_char *ptr, long ptr_address) {
    u_char *current = ptr + 8;
    for (int i = 0; i < 8; i++) {
        *current = (u_char) (ptr_address >> (8 * i) & 0xff);
        current--;
    }
}

/// dumps the input from the allocated memory spaces into char arrays
void
dump_data(const u_char *head, char command_string[], char input[], size_t command_length, size_t input_length) {
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

/// counts the number of words in the input to prepare for args pointers
int count_input_strings(char input[], size_t input_length, bool *wait) {
    int input_string_count = 0;
    int i;
    for (i = 0; i <= input_length; i++) {
        /// a word is always followed by either a space or a \0 and its last character is neither a space nor a \0
        if ((input[i] == ' ' || input[i] == '\0') && input[i - 1] != ' ' && input[i - 1] != '\0') {
            input_string_count++;
            input[i] = '\0';
        } else if (input[i] == '&') {
            *wait = false;
            input[i--] = ' '; /// in order to capture the last word
        }
    }
    return input_string_count;
}

/// assigns a pointer to the first character of each word in the input
void break_down_input(char input[], size_t input_length, char *args[], int input_strings_count) {
    args[0] = &input[0];
    for (int i = 1, j = 1; i < input_length && j < input_strings_count; i++) {
        /// a word is always preceded by a space or a \0 (after count_input_strings)
        /// except for the first word which was added before the loop and its first character is not a space
        if ((input[i] == '\0' || input[i] == ' ') && input[i + 1] != ' ')
            args[j++] = &input[i + 1];
    }
}

/// creates a child process and executes the given command in this process
void fork_and_execute(char *command, char *args[], bool wait_flag) {
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        printf("fork failed :(");
    } else if (pid == 0) {
        execvp(command, args);
        exit(0);
    } else {
        if (wait_flag)
            wait(NULL);
    }
}

/// called when the child process terminates to log the termination to a file
void log_child_completion() {
    FILE *log_file_ptr;
    log_file_ptr = fopen("log.txt", "a");
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log_file_ptr, "[%02d/%02d/%d %02d:%02d:%02d %cM] Child process was terminated\n", tm.tm_mday,
            tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour - 12, tm.tm_min, tm.tm_sec, tm.tm_hour > 12 ? 'P' : 'A');
    fclose(log_file_ptr);
}

int main() {
    signal(SIGCHLD, log_child_completion); /// handles the signal of child termination
    while (true) {
        u_char c;
        do {
            c = (u_char) getchar();
        } while (c == ' ' || c == '\t');
        /// one byte for the char and 8 for the address of the next pointer (see: break_address_into_bytes)
        u_char *head = (u_char *) malloc(sizeof(char) + sizeof(head)), *prev = NULL, *current = NULL;
        *head = c;
        current = head;
        /// command_ended: used to check if the command has ended to differentiate
        /// between spaces between command and argument and spaces in the argument
        bool command_ended = false;
        size_t command_length = 0; /// used to count the characters in the command for later use (stay tuned :D)
        size_t input_length = 0;
        for (;; input_length++) {
            if (!command_ended && (c == ' ' || c == '&')) {
                command_length = input_length;
                command_ended = true;
            }
            if (current != head) {
                *current = c;
                break_address_into_bytes(prev, (long) current);
            }
            c = (u_char) getchar();
            if (c == '\t') /// special case to handle tabs :)
                c = ' ';
            if (c == '\n')
                break;
            prev = current;
            current = (u_char *) malloc(sizeof(char) + sizeof(head));
        }
        input_length++;
        /// special case for commands with no arguments :)
        command_length = command_ended ? command_length : input_length;
        /// adding 1 to the array sizes to add \0 later
        char command_string[command_length + 1];
        char input[input_length + 1];
        dump_data(head, command_string, input, command_length, input_length);
        command_string[command_length] = '\0';
        input[input_length] = '\0';
        sequential_deletion(head, input_length);
        char *command_ptr = &command_string[0];
        bool wait = true;
        int input_strings_count = count_input_strings(input, input_length, &wait);
        char *args[input_strings_count + 1];
        break_down_input(input, input_length, args, input_strings_count);
        args[input_strings_count] = NULL;
        if (strcmp(command_string, "exit") == 0) {
            exit(0);
        }
        fork_and_execute(command_ptr, args, wait);
    }
}

