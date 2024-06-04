#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100

char prompt[50] = "hello: ";
int last_status = 0;

void execute_command(char **args, int background, char *output_file, int append, char *error_file) {
    pid_t pid = fork();
    if (pid == 0) {  // Child process
        if (output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= append ? O_APPEND : O_TRUNC;
            int fd = open(output_file, flags, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (error_file) {
            int fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {  // Fork failed
        perror("fork");
    } else {  // Parent process
        if (!background) {
            waitpid(pid, &last_status, 0);
        }
    }
}

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char *output_file = NULL;
    char *error_file = NULL;
    int background = 0;
    int append = 0;

    while (1) {
        printf("%s", prompt);
        if (!fgets(input, MAX_INPUT, stdin)) break;

        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        int i = 0;
        args[i] = strtok(input, " ");
        while (args[i] != NULL) {
            i++;
            args[i] = strtok(NULL, " ");
        }

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "quit") == 0) {
            break;
        }

        if (strcmp(args[0], "prompt") == 0 && args[1] != NULL && strcmp(args[1], "=") == 0 && args[2] != NULL) {
            strcpy(prompt, args[2]);
            strcat(prompt, ": ");
            continue;
        }

        if (strcmp(args[0], "echo") == 0) {
            if (args[1] != NULL && strcmp(args[1], "$?") == 0) {
                printf("%d\n", WEXITSTATUS(last_status));
            } else {
                for (int j = 1; args[j] != NULL; j++) {
                    printf("%s ", args[j]);
                }
                printf("\n");
            }
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL || chdir(args[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        // Check for background execution
        background = (strcmp(args[i - 1], "&") == 0);
        if (background) {
            args[i - 1] = NULL;
        }

        // Check for output redirection
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], ">") == 0) {
                output_file = args[j + 1];
                args[j] = NULL;
                append = 0;
            } else if (strcmp(args[j], ">>") == 0) {
                output_file = args[j + 1];
                args[j] = NULL;
                append = 1;
            } else if (strcmp(args[j], "2>") == 0) {
                error_file = args[j + 1];
                args[j] = NULL;
            }
        }

        execute_command(args, background, output_file, append, error_file);
        output_file = NULL;
        error_file = NULL;
        append = 0;
    }
    return 0;
}
