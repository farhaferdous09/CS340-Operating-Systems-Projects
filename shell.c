// Farha Ferdous
// CSCI340 Project 1
// line for testing vscode edit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>  

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_PIPES 10
#define PROMPT "CS340Shell% "

// --declaring all the functions--
void parseCommand(char *input, char **args);
int executeCommand(char **args);
int isInternalCommand(char **args);
void executeInternalCommand(char **args);
void handleRedirection(char *input_file, char *output_file);
int processRedirection(char **args, char **input_file, char **output_file);
int hasPipes(char **args, char ***commands, int *num_commands);
void executePipedCommands(char ***commands, int num_commands, char *input_file, char *output_file);
void cleanup(char ***commands, int max_pipes);

// --main shell loop--
int main() {
    char input[MAX_INPUT_SIZE]; // buffer for user input
    char *args[MAX_ARGS];       // array for storing parsed command args

    // loops until exits or reaching end of file
    while (1) { // displays prompt and flushes output buffer
        printf("%s", PROMPT);
        fflush(stdout);
        // reads user input from stdin
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;  // exits on end of file
        }
        // remove trailing newline character
        input[strcspn(input, "\n")] = '\0';
        // skip empty commands
        if (strlen(input) == 0) {
            continue;
        }
        // parses input into command and args
        parseCommand(input, args);
        // skips if no command was entered
        if (args[0] == NULL) {
            continue;
        }
        // checks if command is either exit, cd, or time
        if (isInternalCommand(args)) {
            executeInternalCommand(args);
        } else {    // variables for i/o redirection files
            char *input_file = NULL;
            char *output_file = NULL;
            // processes any i/o redirection symbols
            if (processRedirection(args, &input_file, &output_file)) {
                continue;   // skips if redirection error occurred
            }
            // arrays for handling piped commands
            char **commands[MAX_PIPES];
            int num_commands = 0;
            // checks if command has any pipes
            if (hasPipes(args, commands, &num_commands)) {
                executePipedCommands(commands, num_commands, input_file, output_file);
            } else {
                executeCommand(args);   // executes simple command
                cleanup(commands, MAX_PIPES);
            }
        }
    }
    return 0;
}

// --handles I/O redirection for commands--
void handleRedirection(char *input_file, char *output_file) {
    // handles input redirection if required
    if (input_file != NULL) {   // opens input file in read-only mode
        int fd = open(input_file, O_RDONLY);
        if (fd < 0) {
            perror("shell: open input file");
            exit(EXIT_FAILURE);
        }
        // redirects stdin to come from the file
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("shell: dup2 stdin");
            exit(EXIT_FAILURE);
        }
        close(fd);  // closes original file descriptor
    }
    // handles output redirection if specified
    if (output_file != NULL) {  // opens output file, creates if needed, truncates if exists
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("shell: open output file");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {  // redirects stdout to go to the file
            perror("shell: dup2 stdout");
            exit(EXIT_FAILURE);
        }
        close(fd);  // closes original file descriptor
    }
}

// --processes redirection symbols in command args and modifies args array to remove redirection tokens--
int processRedirection(char **args, char **input_file, char **output_file) {
    *input_file = NULL; // initializes to no redirection
    *output_file = NULL;
    // scans thru command args
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "shell: no input file specified\n");
                return -1;
            }
            *input_file = args[i+1];    // stores input filename
            args[i] = NULL; // removes < from args
            args[i+1] = NULL;   // removes filename from args
        }
        else if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {    // output redirection found
                fprintf(stderr, "shell: no output file specified\n");
                return -1;
            }
            *output_file = args[i+1];   // stores output filename
            args[i] = NULL; // removes > from args
            args[i+1] = NULL;   // removes filename from args
        }
    }
    return 0;
}

// --executes a single command with optional I/O redirection and returns exit status of the command--
int executeCommand(char **args) {
    pid_t pid;
    int status;
    char *input_file = NULL;
    char *output_file = NULL;
    // processes any redirection symbols first
    if (processRedirection(args, &input_file, &output_file) == -1) {
        return -1;  // returns if redirection error
    }
    // forks a new process
    pid = fork();

    if (pid < 0) {
        perror("shell: fork");  // the fork failed
        return -1;
    } else if (pid == 0) {  // child process
        handleRedirection(input_file, output_file); // sets up I/O redirection
        
        // tries executing with path resolution first
        execvp(args[0], args);
        
        // tries execv with absolute path if that fails
        execv(args[0], args);
        // prints error if both failed
        perror("shell: exec");
        exit(EXIT_FAILURE);
    } else {
        waitpid(pid, &status, 0);
        return status;
    }
}

// --parse input string into command and arguments and uses strtok() to split on whitespace--
void parseCommand(char *input, char **args) {
    int i = 0;  // gets first token
    char *token = strtok(input, " \t");
    // continues getting tokens until max args or null
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL; // terminates the argument list
}

// --checks if the command is an internal command and returns 1 otherwise returns 0 if external--
int isInternalCommand(char **args) {
    if (args[0] == NULL) return 0;
    return (strcmp(args[0], "exit") == 0 || // exit command
            strcmp(args[0], "cd") == 0 ||   // change directory
            strcmp(args[0], "time") == 0);  // print current time
}

// --handle internal commands (exit, cd, time) which don't fork as new process--
void executeInternalCommand(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        exit(0);    // exits the shell
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {  // changes directory
            fprintf(stderr, "shell: cd: missing argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("shell: cd");
        }
    } else if (strcmp(args[0], "time") == 0) {
        time_t current_time;    // prints current time
        time(&current_time);    // gets current time
        printf("%s", ctime(&current_time)); // prints as string
    }
}

// --check for and parse pipes into subcommands--
int hasPipes(char **args, char ***commands, int *num_commands) {
    int i, j;
    *num_commands = 0;
    // allocates memory for pipe-separated commands
    for (i = 0; i < MAX_PIPES; i++) {
        commands[i] = malloc(MAX_ARGS * sizeof(char *));
        if (commands[i] == NULL) {
            perror("shell: malloc");
            exit(EXIT_FAILURE);
        }
        for (j = 0; j < MAX_ARGS; j++) {    // initializes to null
            commands[i][j] = NULL;
        }
    }
    // checks if any pipes exist in command
    int pipe_found = 0;
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_found = 1;
            break;
        }
    }
    
    if (!pipe_found) {
        *num_commands = 0;
        return 0;   // no pipes found
    }
    // splits command at pipe symbols
    int cmd_idx = 0;
    int arg_idx = 0;
    
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            commands[cmd_idx][arg_idx] = NULL;  // terminates current command
            cmd_idx++;  // moves to next command
            arg_idx = 0;    // reset argument index
        } else {    // adds argument to current command
            commands[cmd_idx][arg_idx++] = args[i];
        }
    }
    // terminates the last command
    commands[cmd_idx][arg_idx] = NULL;
    *num_commands = cmd_idx + 1;
    
    return (*num_commands > 1); // returns true if multiple commands
}

// --execute commands connected by pipes and handles I/O redirection for first and last commands--
void executePipedCommands(char ***commands, int num_commands, char *input_file, char *output_file) {
    int i;
    int pipes[MAX_PIPES][2];    // pipe file descriptors
    pid_t pid;
    // create all needed pipes
    for (i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("shell: pipe");
            exit(EXIT_FAILURE);
        }
    }
    // fork a process for each command
    for (i = 0; i < num_commands; i++) {
        pid = fork();
        if (pid < 0) {
            perror("shell: fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // cild process
            if (i == 0 && input_file != NULL) { // first command can have input redirection
                int fd = open(input_file, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr, "shell: cannot open input file '%s'\n", input_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            // middle commands read from previous pipe
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            // middle commands write to next pipe
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            // last command can have output redirection
            if (i == num_commands - 1 && output_file != NULL) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    fprintf(stderr, "shell: cannot create output file '%s'\n", output_file);
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            // closes all pipe file descriptors in child
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            // execute the command
            execvp(commands[i][0], commands[i]);
            execv(commands[i][0], commands[i]);
            fprintf(stderr, "shell: command not found: %s\n", commands[i][0]);
            exit(EXIT_FAILURE);
        }
    }
    // parent closes all pipe file descriptors
    for (i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    // parent waits for all children to finish
    for (i = 0; i < num_commands; i++) {
        wait(NULL);
    }
    // free allocated memory
    cleanup(commands, MAX_PIPES);
}

// --helper function to clean up allocated memory for piped commands--
void cleanup(char ***commands, int max_pipes) {
    for (int i = 0; i < max_pipes; i++) {
        if (commands[i] != NULL) {
            free(commands[i]);
            commands[i] = NULL;
        }
    }
}