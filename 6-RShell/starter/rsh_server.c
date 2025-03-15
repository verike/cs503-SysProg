#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

//INCLUDES for extra credit
//#include <signal.h>
//#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

typedef struct {
    int cli_socket;
} client_thread_args_t;

// Function to handle client in a separate thread
void *client_handler(void *args) {
    client_thread_args_t *client_args = (client_thread_args_t *)args;
    exec_client_requests(client_args->cli_socket);
    close(client_args->cli_socket);
    free(client_args);
    pthread_exit(NULL);
}

/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the stop-server command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
int start_server(char *ifaces, int port, int is_threaded){
    int svr_socket;
    int rc;
    //(void)is_threaded; // Suppress unused parameter warning
    //
    //TODO:  If you are implementing the extra credit, please add logic
    //       to keep track of is_threaded to handle this feature
    //



    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0){
        int err_code = svr_socket;  //server socket will carry error code
        return err_code;
    }


    printf("Server started in %s mode.\n", is_threaded ? "Multi-Threaded" : "Single-Threaded");

    rc = process_cli_requests(svr_socket, is_threaded);


    stop_server(svr_socket);


    return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket){
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    struct sockaddr_in server_addr;
    int enable = 1;

    // Step 1: Create a socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("Error creating server socket");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 2: Set socket options to allow address reuse
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Error setting socket options");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 3: Prepare server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // Convert port to network byte order

    // Convert and assign the interface IP
    if (inet_pton(AF_INET, ifaces, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 4: Bind the socket
    if (bind(svr_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Step 5: Put the socket in listening mode
    if (listen(svr_socket, SOMAXCONN) < 0) {
        perror("Error setting server to listen mode");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server listening on %s:%d\n", ifaces, port);
    return svr_socket;

    //return WARN_RDSH_NOT_IMPL;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the stop-server command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the stop-server command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */

/* 
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc;

    while (1) {
        // Step 1: Accept an incoming client connection
        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket < 0) {
            perror("Error accepting client connection");
            fprintf(stderr, CMD_ERR_RDSH_COMM);
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected\n");

        // Step 2: Handle client requests
        rc = exec_client_requests(cli_socket);

        // Step 3: Close client socket after handling requests
        close(cli_socket);
        printf(RCMD_MSG_CLIENT_EXITED);

        // Step 4: If client requested server to stop, break loop
        if (rc == OK_EXIT) {
            printf(RCMD_MSG_SVR_STOP_REQ);
            break;
        }
    }

    return OK_EXIT;

    
    //return WARN_RDSH_NOT_IMPL;
}

*/

int process_cli_requests(int svr_socket, int is_threaded) {
    int cli_socket;
    int rc;

    while (1) {
        // Step 1: Accept a client connection
        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket < 0) {
            perror("Error accepting client connection");
            fprintf(stderr, CMD_ERR_RDSH_COMM);
            return ERR_RDSH_COMMUNICATION;
        }

        //printf("Client connected\n");
        printf("Client connected (socket %d)\n", cli_socket);
        fflush(stdout);

        // Multi-threaded mode: Create a new thread for each client
        if (is_threaded) {
            pthread_t client_thread;
            client_thread_args_t *args = malloc(sizeof(client_thread_args_t));
            if (!args) {
                perror("Memory allocation failed for client thread");
                close(cli_socket);
                continue;
            }

            args->cli_socket = cli_socket;

            if (pthread_create(&client_thread, NULL, client_handler, (void *)args) != 0) {
                perror("Failed to create client thread");
                free(args);
                close(cli_socket);
                continue;
            }

            // Detach thread so it cleans up after itself
            pthread_detach(client_thread);
        } else {
            // Single-threaded mode: Handle client in main thread
            rc = exec_client_requests(cli_socket);
            close(cli_socket);
            printf(RCMD_MSG_CLIENT_EXITED);

            if (rc == OK_EXIT) {
                printf(RCMD_MSG_SVR_STOP_REQ);
                break;
            }
        }
    }

    return OK_EXIT;
}


/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the exit command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the stop-server command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the exit command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent stop-server command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
int exec_client_requests(int cli_socket) {
    char *io_buff;
    ssize_t recv_size;
    int rc;

    // Step 1: Allocate buffer for receiving client data
    io_buff = (char *)malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return ERR_RDSH_COMMUNICATION;
    }

    while (1) {
        // Send the prompt before reading input
        send_message_string(cli_socket, SH_PROMPT);
        // Step 2: Receive command from client
        recv_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        if (recv_size < 0) {
            perror("Error receiving command from client");
            fprintf(stderr, CMD_ERR_RDSH_COMM);
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        // Step 3: Check for client disconnection
        if (recv_size == 0) {
            printf("Client disconnected\n");
            free(io_buff);
            return OK;
        }

        // Ensure null termination of received data
        io_buff[recv_size] = '\0';

        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);

        // Step 4: Check for built-in commands
        if (strcmp(io_buff, "exit") == 0) {
            send_message_string(cli_socket, RCMD_MSG_CLIENT_EXITED "\n");

            free(io_buff);
            return OK;
        }

        if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "dsh4> stop-server\n" RCMD_MSG_SVR_STOP_REQ "\n");
            fflush(stdout); 
            printf("stopping ...");
            usleep(200000);
            free(io_buff);
            printf("stopping ...");
            return OK_EXIT;
        }

        // Step 5: Parse and execute the command pipeline
        command_list_t clist;
        rc = build_cmd_list(io_buff, &clist);
        if (rc != OK) {
            send_message_string(cli_socket, CMD_ERR_RDSH_EXEC);
        } else {
            rc = rsh_execute_pipeline(cli_socket, &clist);
            free_cmd_list(&clist);
        }

        // Step 6: Send EOF character to mark end of response
        send_message_eof(cli_socket);
    }

    // Cleanup before exiting
    free(io_buff);
    return OK;
    
    //return WARN_RDSH_NOT_IMPL;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket){
    
    int bytes_sent;

    // Send EOF character to client
    bytes_sent = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    if (bytes_sent < 0) {
        perror("Error sending EOF to client");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        return ERR_RDSH_COMMUNICATION;
    }

    return OK;
    
    //return WARN_RDSH_NOT_IMPL;
}

/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
int send_message_string(int cli_socket, char *buff){
    
    int bytes_sent, msg_length;

    // Validate input
    if (!buff) {
        fprintf(stderr, "Error: Null buffer passed to send_message_string()\n");
        return ERR_RDSH_COMMUNICATION;
    }

    msg_length = strlen(buff); // Get message length

    // Step 1: Send the message
    bytes_sent = send(cli_socket, buff, msg_length, 0);
    if (bytes_sent < 0) {
        perror("Error sending message to client");
        fprintf(stderr, CMD_ERR_RDSH_COMM);
        return ERR_RDSH_COMMUNICATION;
    }

    // Check for partial send
    if (bytes_sent != msg_length) {
        fprintf(stderr, CMD_ERR_RDSH_SEND, bytes_sent, msg_length);
        return ERR_RDSH_COMMUNICATION;
    }

    if (msg_length == 0) {
        return send_message_eof(cli_socket); // Ensure EOF is always sent
    }
    
    // Step 2: Send EOF character
    return send_message_eof(cli_socket);
    

    
    //return WARN_RDSH_NOT_IMPL;
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
 
    int num_cmds = clist->num;
    int prev_pipe = -1;
    int fd[2];
    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1 && pipe(fd) < 0) {
            perror("pipe");
            fprintf(stderr, CMD_ERR_RDSH_EXEC);
            return ERR_RDSH_CMD_EXEC;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            fprintf(stderr, CMD_ERR_RDSH_EXEC);
            return ERR_RDSH_CMD_EXEC;
        }

        if (pids[i] == 0) {  // Child Process
            if (i == 0) {
                // First command: Redirect stdin from the client socket
                dup2(cli_sock, STDIN_FILENO);
            } else {
                // Redirect stdin from the previous pipe
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }

            if (i < num_cmds - 1) {
                // Not the last command: Redirect stdout to the pipe
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            } else {
                // Last command: Redirect stdout and stderr to client socket
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            }

            // Close all open file descriptors
            if (i > 0) {
                close(prev_pipe);
            }
            if (i < num_cmds - 1) {
                close(fd[0]);
                close(fd[1]); // Ensure both pipe ends are closed
            }
            

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            fprintf(stderr, CMD_ERR_RDSH_EXEC);
            exit(ERR_RDSH_CMD_EXEC);
        } else { // Parent Process
            if (i > 0) close(prev_pipe);
            if (i < num_cmds - 1) {
                prev_pipe = fd[0];
                close(fd[1]);
            }
        }
    }

    // Wait for all children to finish
    int status;
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
    }

    // Send EOF to indicate command completion
    send_message_eof(cli_sock);

    return WEXITSTATUS(status);

    
    //return WARN_RDSH_NOT_IMPL;
}

/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
Built_In_Cmds rsh_match_command(const char *input)
{
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    return BI_NOT_BI; // Not a built-in command
}


/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
 Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd->argc == 0) {
        return BI_NOT_BI;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) {
        return BI_CMD_EXIT; // Signal that client should exit
    }

    if (strcmp(cmd->argv[0], "stop-server") == 0) {
        return BI_CMD_STOP_SVR; // Signal that server should shut down
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        if (cmd->argc < 2) {
            send_message_string(STDOUT_FILENO, "cd: missing argument\n");
            return BI_EXECUTED;
        }
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd failed");
            send_message_string(STDOUT_FILENO, "cd: No such directory\n");
        }
        return BI_EXECUTED; // Command was executed
    }

    if (strcmp(cmd->argv[0], "dragon") == 0) {
        send_message_string(STDOUT_FILENO, "🔥 Roar! 🔥\n");
        return BI_EXECUTED;
    }

    return BI_NOT_BI; // Not a built-in command, proceed to execvp()
}