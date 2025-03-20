#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

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

    // THIS CODE SHOULD BE THE SAME AS PRIOR ASSIGNMENTS
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

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (!cmd_line || strlen(cmd_line) == 0)
        return WARN_NO_CMDS;

    memset(clist, 0, sizeof(command_list_t));

    char *pipe_pos = strchr(cmd_line, '|'); // Finding the first pipe `|`

    while (pipe_pos && clist->num < CMD_MAX)
    {
        *pipe_pos = '\0';
        char *current_command = cmd_line;  // first part of the command
        char *next_command = pipe_pos + 1; // everything after '|'

        while (*current_command == SPACE_CHAR)
            current_command++;

        char *end = current_command + strlen(current_command) - 1;
        while (end > current_command && *end == SPACE_CHAR)
            *end-- = '\0';

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
        clist->num++;

        // Move to the next command
        cmd_line = next_command;

        pipe_pos = strchr(cmd_line, '|'); // find the next '|'
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

        pids[i] = fork();

        if (pids[i] == 0) // Child process
        {

            // Redirect input from previous pipe if not the first command
            if (i > 0)
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("[ERROR] Failed to redirect STDIN");
                    exit(EXIT_FAILURE);
                }
            }

            // Redirect output to next pipe if not the last command
            else if (i < num_cmds - 1)
            {
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
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) return ERR_MEMORY;
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

int free_cmd_list(command_list_t *cmd_lst) {
    if (cmd_lst == NULL) {
        return ERR_MEMORY; // Return an error if NULL is passed
    }

    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]); // Free each command buffer
    }

    cmd_lst->num = 0; // Reset the number of commands in the list
    return OK;
}

void print_dragon(){
    // TODO implement 
    printf("             ______________ \n");
    printf("          ,-' \\     :     / '-.\n");
    printf("        ,'     \\    :    /     `,\n");
    printf("       /        \\___:___/        \\\n");
    printf("      :      ,---'     `---,      :\n");
    printf("      :     /             \\     :\n");
    printf("       \\   :               :   /\n");
    printf("        `._ \\             / _,'\n");
    printf("           `--._________.--'\n");
    printf("              /         \\\n");
    printf("             :           :\n");
    printf("             :           :\n");
    printf("              \\         /\n");
    printf("               `-.   .-'\n");
    printf("                  `-`\n");
  }
  