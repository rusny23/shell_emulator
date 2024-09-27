/*
 * shell.c
 *
 *
 *     January 29 2024
 *
 * CS111 Homework 1
 * Shell
 *
 * SUMMARY: This file contains an implementation of the jumboshell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/types.h> 


void parse_input(char *token, char *s);
void free_array(char **arr, int length);
int get_length(char **arr);
void grow_args_array(char ***arr, int size, size_t *capacity);
void multiple_args_check(char ***myargs, int myargs_length);
void run_children(char ***myargs, int *start_idxs, int myargs_length);

int main(int argc, char *argv[]) {
    setenv("PS1", "jsh$ ", 1);
    size_t buffer = 0;
    char *input = NULL;
    char s[8] = " \t\n\f\r\v"; // Unwanted characters

    printf("%s", getenv("PS1"));
    if (getline(&input, &buffer, stdin) == -1) {
        fprintf(stderr, "Error reading input\n");
        return 1;
    }

    while (strncmp(input, "exit", strlen("exit")) != 0) {
        // Remove newline from input if there is
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        // Create a new buffer for modified input
        char *modified_input = malloc((len + 3) * sizeof(char)); // +3 for potential two spaces and null terminator
        size_t modified_len = 0;

        // Check for '|' character and add spaces in modified_input
        for (size_t i = 0; i < len; i++) {
            if (input[i] == '|') {
                // Add space before '|'
                if (i > 0 && input[i - 1] != ' ') {
                    modified_input[modified_len++] = ' ';
                }

                modified_input[modified_len++] = '|';

                // Add space after '|'
                if (i < len - 1 && input[i + 1] != ' ') {
                    modified_input[modified_len++] = ' ';
                }
            } else {
                modified_input[modified_len++] = input[i];
            }
        }

        modified_input[modified_len] = '\0'; // Null terminate the modified input

        char *token = strtok(modified_input, s);

        parse_input(token, s);

        free(modified_input); // Free the modified input buffer

        printf("%s", getenv("PS1"));
        if (getline(&input, &buffer, stdin) == -1) {
            fprintf(stderr, "Error reading input\n");
            break; // Break out of the loop on error
        }
    }

    free(input);
    return 0;
}


/* Function:   parse_input
 * Parameters: a pointer to the first token that is extracted from the input 
 *             string, an array of characters that need to be filtered out of 
 *             the input.
 * Purpose:    parses the user input to place each subsequent command,
 *             argument, and pipe (|) character (tokens) into a dynamic array.
 * Returns:    nothing
 */
void parse_input(char *token, char *s){
    size_t capacity = 10; //initial capacity of myargs
    size_t index = 0; //current size of myargs

    //array to store user input as separate strings
    char **myargs = (char **)malloc(sizeof( char *) * capacity); //TRACK

    //check for proper malloc
    if(myargs == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    while(token != NULL){

        grow_args_array(&myargs, index, &capacity);

        myargs[index] = (char *)malloc(strlen(token) + 1); //TRACK

        if(myargs[index] == NULL){
            fprintf(stderr, "Memory allocation failed for the string\n");
            free_array(myargs, index);
            return;
        }

        strcpy(myargs[index], token);

        index++;
        token = strtok(NULL, s);
    }
    
    grow_args_array(&myargs, index, &capacity);
    myargs[index] = NULL;

    //plus one to account for NULL terminator
    int myargs_length = index + 1;

    multiple_args_check(&myargs, myargs_length);
}


/* Function:   multiple_args_check
 * Parameters: a pointer to an array (myargs) of commands, (possibly) pipe 
 *             lines, and arguments. And the length of the array.
 * Purpose:    parses through myargs and replaces any pipe character with
 *             a NULL pointer. Also creates an array that contains all of
 *             the indices in myargs that hold a command word (ex. wc, ls, etc).
 * Returns:    nothing
 */
void multiple_args_check(char ***myargs, int myargs_length){
    int command_start_idx[myargs_length];
    command_start_idx[0] = 0;
    int curr_idx = 0;
    int my_args_curr_idx = 0;

    while((*myargs)[my_args_curr_idx] != NULL){
        if(strcmp((*myargs)[my_args_curr_idx], "|") == 0){
            free((*myargs)[my_args_curr_idx]);
            (*myargs)[my_args_curr_idx] = NULL;
            if(my_args_curr_idx + 1 != myargs_length){
                command_start_idx[++curr_idx] = my_args_curr_idx + 1;
            }
        }
        my_args_curr_idx++;
    }
    command_start_idx[++curr_idx] = -1;

    //this will have the char** array prepped with a list of command start 
    //indices
    run_children(myargs, command_start_idx, myargs_length);
}


/* Function:   run_children
 * Parameters: a pointer to an array (myargs) of commands, an array holding the 
 *             indices of all the command words in myargs. And the length of the
 *             array.
 * Purpose:    Forks and runs child processes that are stored in myargs using 
 *             execvp. If there is piping, each command pipes their output to 
 *             the the input of the next command and so forth. The last 
 *             command's output and status are printed.
 * Returns:    nothing
 */
void run_children(char ***myargs, int *start_idxs, int myargs_length) {
    int i = 0;
    int prev_pipe[2] = {-1, -1};
    pid_t child_pids[myargs_length]; // Array to store child PIDs
    int num_children = 0; // Counter for the number of child processes

    while (start_idxs[i] != -1) {
        int current_pipe[2]; // Pipe for the current child process
        if (pipe(current_pipe) < 0) {
            perror("pipe");
            exit(127);
        }

        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork");
            free_array(*myargs, myargs_length);
            exit(127);
        } else if (child_pid == 0) {
            // Child process
            if (prev_pipe[0] != -1) {
                // Setup input from previous pipe
                close(prev_pipe[1]); // Close write end of previous pipe
                if (dup2(prev_pipe[0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(127);
                }
                close(prev_pipe[0]); // Close read end of previous pipe
            }

            if (start_idxs[i + 1] != -1) { // If not the last command
                close(current_pipe[0]); // Close read end of current pipe
                if (dup2(current_pipe[1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(127);
                }
            }
            close(current_pipe[1]); // Close write end of current pipe

            execvp((*myargs)[start_idxs[i]], *myargs + start_idxs[i]);
            perror("execvp");
            free_array(*myargs, get_length(*myargs));
            exit(127);
        } else {
            // Parent process
            child_pids[num_children++] = child_pid; // Store child PID
            if (prev_pipe[0] != -1) {
                // Close previous pipe
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }
            prev_pipe[0] = current_pipe[0];
            prev_pipe[1] = current_pipe[1];
            if (start_idxs[i + 1] == -1) { // If last command
                close(current_pipe[1]); // Close write end of the last pipe
            }
        }

        i++;
    }

    // Close the last read end outside the loop
    if (prev_pipe[0] != -1) {
        close(prev_pipe[0]);
    }

    // Wait for all child processes
    for (i = 0; i < num_children; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (i == num_children - 1) { // For the last child process
            if (WIFEXITED(status)) {
                printf("jsh status: %d\n", WEXITSTATUS(status));
            }
        }
    }

    free_array(*myargs, myargs_length);
}



/* Function:   free_array
 * Parameters: a string array and the length of the array
 * Purpose:    free the dynamic array and all of its elements
 * Returns:    nothing
 */
void free_array(char **arr, int length)
{
    assert(arr != NULL);

    for (int i = 0; i < length; i++) {
        if (arr[i] != NULL) {
            free(arr[i]);
            arr[i] = NULL;
        }
    }

    free(arr);
}


/* Function:   get_length
 * Parameters: a string array 
 * Purpose:    calculate the length of the string array
 * Returns:    the length of the string array
 */
int get_length(char **arr){
    int length = 0;
    while (arr[length] != NULL) {
        length++;
    }
    return length;
}


/* Function:   grow_args_array
 * Parameters: a pointer to a string array, the current size of the array, and
 *             its current capacity.
 * Purpose:    grows the dynamic array once the size of the array has reached
 *             the capacity. Doubles the capacity as well.
 * Returns:    nothing
 */
void grow_args_array(char ***arr, int size, size_t *capacity){
    if(size > ((*capacity) - 1)){
        //double the capacity
        char **new_myargs = NULL;

        (*capacity) *= 2;
        new_myargs = (char **)realloc(*arr, sizeof(char *) * (*capacity));

        if(new_myargs == NULL){
            fprintf(stderr, "Memory allocation failed to grow myargs array\n");
            free_array(*arr, size);
        }

        *arr = new_myargs;
    }
}




