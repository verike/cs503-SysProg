#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include <errno.h>
#include <limits.h>
#include <sys/types.h>  
#include <sys/stat.h>   




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

 int exec_local_cmd_loop() {
    char input_buffer[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input_buffer, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        input_buffer[strcspn(input_buffer, "\n")] = '\0';
        if (strlen(input_buffer) == 0) continue;

        // Parsing command list
        int rc = build_cmd_list(input_buffer, &clist);
        if (rc != OK) {
            if (rc == WARN_NO_CMDS) printf(CMD_WARN_NO_CMD);
            else if (rc == ERR_TOO_MANY_COMMANDS) printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        // Check built-in commands
        if (clist.num > 1) {
            for (int i = 0; i < clist.num; i++) {
                Built_In_Cmds bi = match_command(clist.commands[i].argv[0]);
                if (bi != BI_NOT_BI) {
                    fprintf(stderr, "error: built-in command in pipeline\n");
                    rc = ERR_CMD_ARGS_BAD;
                    break;
                }
            }
            if (rc != OK) {
                free_cmd_list(&clist);
                continue;
            }
        } else {
            cmd_buff_t *cmd = &clist.commands[0];
            Built_In_Cmds bi = match_command(cmd->argv[0]);
            if (bi != BI_NOT_BI) {
                exec_built_in_cmd(cmd);
                free_cmd_list(&clist);
                continue;
            }
        }

        // Execution pipeline
        execute_pipeline(&clist);
        free_cmd_list(&clist);
    }
    return OK;
}



/**
 * @brief Executes a single command, handling output redirection.
 */
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return ERR_EXEC_CMD;
    }

    if (pid == 0) {  // Child Process
        int fd = -1;
        char *output_file = NULL;
        int append_mode = 0;
        char *argv_clean[CMD_ARGV_MAX];
        int i, j = 0;

        // Look for > or >> in the command arguments
        for (i = 0; i < cmd->argc; i++) {
            if (strcmp(cmd->argv[i], ">") == 0 || strcmp(cmd->argv[i], ">>") == 0) {
                if (i + 1 >= cmd->argc) {
                    fprintf(stderr, "error: no output file specified\n");
                    exit(ERR_EXEC_CMD);
                }
                output_file = cmd->argv[i + 1];
                append_mode = (strcmp(cmd->argv[i], ">>") == 0);
                i++;  // Skip the filename in arguments
            } else {
                argv_clean[j++] = cmd->argv[i];  // Copy valid arguments
            }
        }
        argv_clean[j] = NULL;  // Null-terminate the cleaned-up argument list

        // Redirect output to a file if needed
        if (output_file) {
            fd = open(output_file, O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC), 0644);
            if (fd < 0) {
                perror("Failed to open output file");
                exit(ERR_EXEC_CMD);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Execute the actual command
        execvp(argv_clean[0], argv_clean);
        perror("execvp failed");
        exit(ERR_EXEC_CMD);
    } else {
        waitpid(pid, NULL, 0);
        return OK;
    }
}





// Processing pipeline execution
int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    int prev_pipe = -1;
    int fd[2];
    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1 && pipe(fd) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) { // Child
            if (i > 0) {
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            // Close all pipe ends
            if (i > 0) close(prev_pipe);
            if (i < num_cmds-1) close(fd[0]);

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(ERR_EXEC_CMD);
        } else { // Parent
            if (i > 0) close(prev_pipe);
            if (i < num_cmds - 1) {
                prev_pipe = fd[0];
                close(fd[1]);
            }
        }
    }

    // Wait for all children
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
    return OK;
}


Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (strcmp(cmd->argv[0], "exit") == 0) {
        free_cmd_buff(cmd);
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        char target_dir[CMD_MAX];

        if (cmd->argc > 1) {
            strncpy(target_dir, cmd->argv[1], CMD_MAX);
        } if (cmd->argc > 2) { // Too many arguments for cd
            fprintf(stderr, "error: cd command accepts at most 1 argument\n");
            return ERR_CMD_ARGS_BAD;
        }
           // strncpy(target_dir, "/tmp", PATH_MAX); // Default to /tmp
        

        // Ensure string is null-terminated
        target_dir[CMD_MAX - 1] = '\0';

        // Check if directory exists
        struct stat st;
        if (stat(target_dir, &st) == -1) {
            if (mkdir(target_dir, 0777) != 0) {
                //perror("mkdir failed");
                return BI_EXECUTED;
            }
        }

        // Check permissions before chdir()
        if (access(target_dir, R_OK | X_OK) != 0) {
            perror("cd failed");
            return BI_EXECUTED;
        }

        // Attempt to change directory
        if (chdir(target_dir) != 0) {
            perror("cd failed");
        } else {
            char cwd[CMD_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                //printf("%s\n", cwd);  // Print working directory only once
            } else {
                perror("getcwd failed");
            }
        }

        return BI_EXECUTED;
    }

    if (strcmp(cmd->argv[0], "rc") == 0) {
        printf("%d\n", errno);
        return BI_EXECUTED;
    }

    return BI_NOT_BI;
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "rc") == 0) return -1;
    return BI_NOT_BI;
}



int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    char *ptr = cmd_line;
    int in_quotes = 0;
    char *token_start = NULL;

    while (*ptr) {
        while (isspace(*ptr) && !in_quotes) ptr++; // Skip spaces outside quotes

        if (*ptr == '"') { // Detect start of quoted string
            in_quotes = !in_quotes;
            ptr++;
            token_start = ptr;
        } else {
            token_start = ptr;
        }

        while (*ptr && (in_quotes || !isspace(*ptr))) { // Capture argument
            if (*ptr == '"') {
                in_quotes = !in_quotes;
                *ptr = '\0';  // Properly terminate quoted string
            }
            ptr++;
        }

        if (*ptr) {
            *ptr = '\0';  // Terminate non-quoted argument
            ptr++;
        }

        cmd_buff->argv[cmd_buff->argc++] = token_start;
        //if (cmd_buff->argc >= CMD_ARGV_MAX - 1) break;
        if (cmd_buff->argc > CMD_ARGV_MAX - 1) {
            fprintf(stderr, "error: too many arguments\n");
            return ERR_CMD_ARGS_BAD;  // Return error -4
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}


// Used to split commands by pipeline
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    char *saveptr;
    char *token = strtok_r(cmd_line, "|", &saveptr);

    while (token != NULL) {
        if (clist->num >= CMD_MAX) {
            fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            return ERR_TOO_MANY_COMMANDS;
        }

        // Trim leading/trailing spaces
        while (*token == ' ') token++;
        size_t len = strlen(token);
        while (len > 0 && token[len-1] == ' ') token[--len] = '\0';

        cmd_buff_t *cmd = &clist->commands[clist->num];
        if (alloc_cmd_buff(cmd) != OK) return ERR_MEMORY;

        int rc = build_cmd_buff(token, cmd);
        if (rc != OK) {
            free_cmd_buff(cmd);
            return rc;
        }

        clist->num++;
        token = strtok_r(NULL, "|", &saveptr);
    }

    if (clist->num == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    return OK;
}

// Release command list
int free_cmd_list(command_list_t *clist) {
    for (int i = 0; i < clist->num; i++) {
        free_cmd_buff(&clist->commands[i]);
    }
    clist->num = 0;
    return OK;
}