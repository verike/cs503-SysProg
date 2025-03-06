#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

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
/*
 * Implements the shell command loop, handling user input.
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

/*
 * Parses a command line into a list of commands separated by pipes.
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (!cmd_line || strlen(cmd_line) == 0)
        return WARN_NO_CMDS;

    // printf("[DEBUG] Full input command line: %s\n", cmd_line);

    memset(clist, 0, sizeof(command_list_t));

    char *pipe_pos = strchr(cmd_line, '|'); // Finding the first pipe `|`

    // char *token = strtok(cmd_line, PIPE_STRING);

    while (pipe_pos && clist->num < CMD_MAX)
    {
        // printf("[DEBUG] Splitting command on '|': %s\n", token);

        // Trim leading spaces
        // while (*token == SPACE_CHAR)
        //     token++;

        *pipe_pos = '\0';
        char *current_command  = cmd_line;  // first part of the command
        char *next_command = pipe_pos + 1;  // everything after '|'


        // Trim trailing spaces
        // char *end = token + strlen(token) - 1;

        while (*current_command == SPACE_CHAR)
            current_command++;

        char *end = current_command + strlen(current_command) - 1;
        while (end > current_command && *end == SPACE_CHAR)
        // while (end > token && *end == SPACE_CHAR)
            *end-- = '\0';

        // Debug raw token before processing
        // printf("[DEBUG] Raw token: %s\n", current_command);

        // Validate command size
        if (strlen(current_command) >= EXE_MAX + ARG_MAX)
            return ERR_CMD_OR_ARGS_TOO_BIG;

        // Allocate memory for command buffer
        cmd_buff_t *cmd = &clist->commands[clist->num];
        cmd->_cmd_buffer = strdup(current_command); // Store the full command line

        // Tokenize into arguments
        cmd->argc = 0;
        char *arg_token = strtok(cmd->_cmd_buffer, " ");
        while (arg_token && cmd->argc < CMD_ARGV_MAX - 1)
        {
            cmd->argv[cmd->argc++] = arg_token;
            arg_token = strtok(NULL, " ");
        }
        cmd->argv[cmd->argc] = NULL; // Null-terminate argument list

        // debug print to confirm command is added
        // printf("[DEBUG] Command %d parsed as: %s\n", clist->num, cmd->argv[0]);

        clist->num++;

        // Move to the next command
        cmd_line = next_command;

        pipe_pos = strchr(cmd_line, '|');  // find the next '|'
    }

    // Process the last command (if there's no more `|`)
    if (strlen(cmd_line) > 0 && clist->num < CMD_MAX)
    {
        // Trim leading and trailing spaces for the last command
        while (*cmd_line == SPACE_CHAR)
            cmd_line++;
        char *end = cmd_line + strlen(cmd_line) - 1;
        while (end > cmd_line && *end == SPACE_CHAR)
            *end-- = '\0';

        // printf("[DEBUG] Raw token (Last Command): %s\n", cmd_line);

        cmd_buff_t *cmd = &clist->commands[clist->num];
        cmd->_cmd_buffer = strdup(cmd_line);

        // Tokenize into arguments
        cmd->argc = 0;
        char *arg_token = strtok(cmd->_cmd_buffer, " ");
        while (arg_token && cmd->argc < CMD_ARGV_MAX - 1)
        {
            cmd->argv[cmd->argc++] = arg_token;
            arg_token = strtok(NULL, " ");
        }
        cmd->argv[cmd->argc] = NULL;

        // printf("[DEBUG] Command %d parsed as: %s\n", clist->num, cmd->argv[0]);

        clist->num++;
    }

    return (clist->num > 0) ? OK : WARN_NO_CMDS;
}

/*
 * Executes a sequence of piped commands.
 */

int execute_pipeline(command_list_t *clist)
{
    int num_cmds = clist->num;
    int pipes[CMD_MAX - 1][2]; // Pipes for communication
    pid_t pids[CMD_MAX];       // Store process IDs

    // printf("[DEBUG] Number of commands: %d\n", num_cmds);

    
    // Create pipes
    for (int i = 0; i < num_cmds - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Pipe creation failed");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < num_cmds; i++)
    {
        // printf("[DEBUG] Forking process for command %d: %s\n", i, clist->commands[i].argv[0]);

        pids[i] = fork();

        if (pids[i] == 0) // Child process
        {
            // printf("[DEBUG] Inside child process %d\n", i);

            // Redirect input from previous pipe if not the first command
            if (i > 0)
            {
                // printf("[DEBUG] Redirecting input: Reading from pipe[%d][0]\n", i - 1);
                // fflush(stdout);
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("[ERROR] Failed to redirect STDIN");
                    exit(EXIT_FAILURE);
                }
                // dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // Redirect output to next pipe if not the last command
            else if (i < num_cmds - 1)
            {
                // printf("[DEBUG] Redirecting output: Writing to pipe[%d][1]\n", i);
                // fflush(stdout);
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                {
                    perror("[ERROR] Failed to redirect STDOUT");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipes in child
            for (int j = 0; j < num_cmds - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Debug print command being executed
            // printf("[DEBUG] Executing command: %s\n", clist->commands[i].argv[0]);
            // fflush(stdout);

            // Execute command using arguments
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv))
            {
                perror("execvp failed");
                exit(EXIT_FAILURE);
            }
        }
        else if (pids[i] < 0)
        {
            perror("Fork failed");
            return ERR_EXEC_CMD;
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < num_cmds; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    return OK;

    // int pipefd[2];
    // pid_t pid1, pid2;

    // if (pipe(pipefd) == -1)
    // {
    //     perror("Pipe creation failed");
    //     return ERR_EXEC_CMD;
    // }

    // pid1 = fork();
    // if (pid1 == 0) // First child (ls)
    // {
    //     dup2(pipefd[1], STDOUT_FILENO); // Redirect output to pipe
    //     close(pipefd[0]);
    //     close(pipefd[1]);
    //     execlp("ls", "ls", NULL);
    //     perror("exec ls failed");
    //     exit(EXIT_FAILURE);
    // }

    // pid2 = fork();
    // if (pid2 == 0) // Second child (grep dshlib.c)
    // {
    //     dup2(pipefd[0], STDIN_FILENO); // Redirect input from pipe
    //     close(pipefd[0]);
    //     close(pipefd[1]);
    //     execlp("grep", "grep", "dshlib.c", NULL);
    //     perror("exec grep failed");
    //     exit(EXIT_FAILURE);
    // }

    // // Close pipe in parent
    // close(pipefd[0]);
    // close(pipefd[1]);

    // // Wait for child processes
    // waitpid(pid1, NULL, 0);
    // waitpid(pid2, NULL, 0);

    // return OK;
}