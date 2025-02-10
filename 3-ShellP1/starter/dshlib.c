#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || strlen(cmd_line) == 0)
        return WARN_NO_CMDS;

    memset(clist, 0, sizeof(command_list_t));
    char *token = strtok(cmd_line, PIPE_STRING);

    while (token && clist->num < CMD_MAX)
    {
        while (*token == SPACE_CHAR)
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == SPACE_CHAR)
            *end-- = '\0';

        if (strlen(token) >= EXE_MAX + ARG_MAX)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }

        char *arg_start = strchr(token, SPACE_CHAR);
        if (arg_start)
        {
            *arg_start = '\0';
            strcpy(clist->commands[clist->num].exe, token);
            strcpy(clist->commands[clist->num].args, arg_start + 1);
        }
        else
        {
            strcpy(clist->commands[clist->num].exe, token);
        }

        clist->num++;
        token = strtok(NULL, PIPE_STRING);
    }

    return (clist->num > 0) ? OK : WARN_NO_CMDS;
}

// void trim_spaces(char *str)
// {
//     char *end;
//     while (isspace((unsigned char)*str))
//         str++; // Trim leading spaces
//     if (*str == 0)
//         return;
//     end = str + strlen(str) - 1;
//     while (end > str && isspace((unsigned char)*end))
//         end--; // Trim trailing spaces
//     *(end + 1) = '\0';
// }
