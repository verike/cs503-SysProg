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
void dsh_cd(char *path);
int exec_local_cmd_loop()
{
    char *cmd_buff;
    int rc = 0;
    cmd_buff_t cmd;

    cmd_buff = malloc(SH_CMD_MAX);
    if (!cmd_buff)
    {
        perror("Memory allocation failed");
        return ERR_MEMORY;
    }
    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (build_cmd_buff(cmd_buff, &cmd) != OK)
        {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        if (cmd.argc == 0)
            continue;

        if (strcmp(cmd.argv[0], EXIT_CMD) == 0)
            break;
        if (strcmp(cmd.argv[0], "cd") == 0)
        {
            dsh_cd(cmd.argv[1]);
            continue;
        }
        if (strcmp(cmd.argv[0], "rc") == 0)
        {
            printf("%d\n", rc);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(cmd.argv[0], cmd.argv);
            fprintf(stderr, "error");
            exit(127);
        }
        else if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
            // rc = WEXITSTATUS(status);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                fprintf(stderr, "error: command exited with status %d\n", WEXITSTATUS(status));
            }
        }
        else
        {
            perror("Fork failed");
        }
    }

    free(cmd_buff);
    return OK;
}
void dsh_cd(char *path)
{
    if (path == NULL || strlen(path) == 0)
        return;
    if (chdir(path) != 0)
        perror("cd failed");
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd)
{
    // char *token;
    cmd->argc = 0;

    char *token = strtok(cmd_line, " ");
    while (token != NULL)
    {
        if (token[0] == '"')
        {
            token++;
            char *end = token + strlen(token) - 1;
            size_t len = strlen(token);
            if (len > 0 && token[len - 1] == '"')
                token[len - 1] = '\0';
            if (*end == '"')
                *end = '\0';
        }

        // Remove extra spaces from token
        while (isspace((unsigned char)*token))
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace((unsigned char)*end))
            end--;
        *(end + 1) = '\0';

        cmd->argv[cmd->argc++] = strdup(token);
        token = strtok(NULL, " ");
    }

    cmd->argv[cmd->argc] = NULL;
    return (cmd->argc > 0) ? OK : WARN_NO_CMDS;
}
