#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/**** 
 **** FOR REMOTE SHELL USE YOUR SOLUTION FROM SHELL PART 3 HERE
 **** THE MAIN FUNCTION CALLS THIS ONE AS ITS ENTRY POINT TO
 **** EXECUTE THE SHELL LOCALLY
 ****
 */

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

int exec_local_cmd_loop()
{
    char *cmd_buff = malloc(SH_CMD_MAX);
    if (!cmd_buff)
    {
        perror("Memory allocation failed");
        return ERR_MEMORY;
    }

    command_list_t clist;

    while (1)
    {
        printf("%s", SH_PROMPT);

        // Read user input
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Check for exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0)
            break;

        // Parse input into a command list
        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc == OK)
        {
            execute_pipeline(&clist);
        }
        else if (rc == WARN_NO_CMDS)
        {
            printf(CMD_WARN_NO_CMD);
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        }
    }

    free(cmd_buff);
    return OK;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    memset(clist, 0, sizeof(command_list_t));

    char *token = strtok(cmd_line, " ");
    int cmd_index = 0;

    while (token && cmd_index < CMD_MAX) {
        clist->commands[cmd_index].argv[0] = strdup(token);
        clist->commands[cmd_index].argc = 1;
        cmd_index++;

        token = strtok(NULL, " ");
    }

    clist->num = cmd_index;
    return (cmd_index > 0) ? OK : WARN_NO_CMDS;
}


int execute_pipeline(command_list_t *clist) {
    if (clist->num == 0) return WARN_NO_CMDS;

    int pipe_fds[2];
    int prev_fd = STDIN_FILENO;

    for (int i = 0; i < clist->num; i++) {
        if (i < clist->num - 1) {
            pipe(pipe_fds);
        }

        pid_t pid = fork();
        if (pid == 0) {  // Child process
            if (i > 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < clist->num - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("exec failed");
            exit(ERR_EXEC_CMD);
        } else {  // Parent process
            wait(NULL);
            if (i > 0) {
                close(prev_fd);
            }
            if (i < clist->num - 1) {
                close(pipe_fds[1]);
                prev_fd = pipe_fds[0];
            }
        }
    }
    return OK;
}

int parse_command(char *input, command_list_t *clist) {
    if (!input || strlen(input) == 0) {
        printf("parse_command: Empty command received\n");
        return WARN_NO_CMDS;
    }

    // Debugging log
    printf("parse_command: Parsing input -> %s\n", input);

    // Ensure clist is properly initialized
    memset(clist, 0, sizeof(command_list_t));

    // Tokenize input while ensuring we ignore semicolons or invalid characters
    char *token = strtok(input, " ");
    while (token != NULL && clist->num < CMD_MAX) {
        // Skip invalid tokens (e.g., semicolons)
        if (strcmp(token, ";") == 0 || strcmp(token, "") == 0) {
            token = strtok(NULL, " ");
            continue;
        }

        clist->commands[clist->num].argv[0] = strdup(token);
        clist->commands[clist->num].argv[1] = NULL;  // Ensure termination
        clist->num++;
        token = strtok(NULL, " ");
    }

    if (clist->num == 0) {
        printf("parse_command: No valid commands found\n");
        return WARN_NO_CMDS;
    }

    return OK;
}
