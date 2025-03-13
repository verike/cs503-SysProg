
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

// INCLUDES for extra credit
// #include <signal.h>
// #include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

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
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server.
 *
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.
 *
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.
 */
int start_server(char *ifaces, int port, int is_threaded)
{
    int svr_socket;
    int rc;

    //
    // TODO:  If you are implementing the extra credit, please add logic
    //       to keep track of is_threaded to handle this feature
    //

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0)
    {
        int err_code = svr_socket; // server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

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
int stop_server(int svr_socket)
{
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
int boot_server(char *ifaces, int port)
{
    int svr_socket;
    struct sockaddr_in server_addr;
    int enable = 1;

    // Create socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0)
    {
        perror("socket failed");
        return ERR_RDSH_COMMUNICATION;
    }

    // Allow port reuse
    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Setup server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ifaces);
    server_addr.sin_port = htons(port);

    // Bind the socket to IP and port
    if (bind(svr_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Start listening for incoming connections
    if (listen(svr_socket, 5) < 0)
    {
        perror("listen failed");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server started on %s:%d\n", ifaces, port);
    return svr_socket;
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
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop.
 *
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket.
 *
 *  Returns:
 *
 *      OK_EXIT:  When the client sends the `stop-server` command this function
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
int process_cli_requests(int svr_socket)
{
    int cli_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1)
    {
        // Accept a client connection
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &client_len);
        if (cli_socket < 0)
        {
            perror("accept failed");
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected\n");

        // Handle client request
        int rc = exec_client_requests(cli_socket);

        // Close client socket
        close(cli_socket);

        // If client requested stop-server, terminate the server
        if (rc == OK_EXIT)
        {
            printf("Stopping server...\n");
            break;
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
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection.
 *      2.  When the client executes the `stop-server` command this function
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
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client.
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 *
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors.
 */

int exec_client_requests(int cli_socket)
{
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff)
    {
        perror("malloc failed");
        return ERR_RDSH_COMMUNICATION;
    }

    while (1)
    {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        int recv_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ, 0);

        if (recv_size <= 0)
        {
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        printf("[DEBUG] : Recieved command: %s\n", io_buff); // Debugging log

        // Check for built-in commands
        Built_In_Cmds cmd_type = rsh_match_command(io_buff);

        if (cmd_type != BI_NOT_BI)
        {
            if (cmd_type == BI_CMD_EXIT)
            {
                free(io_buff);
                return OK;
            }
            else if (cmd_type == BI_CMD_STOP_SVR)
            {
                free(io_buff);
                return OK_EXIT;
            }
            else if (cmd_type == BI_CMD_CD)
            {
                char *dir = strtok(NULL, " ");
                if (dir == NULL)
                {
                    dir = getenv("HOME"); // Default to home if no argument
                }
                if (chdir(dir) != 0)
                {
                    perror("cd failed");
                }
                send_message_eof(cli_socket);
                continue;
            }
        }

        // Execute the command and send response
        command_list_t clist;
        printf("[DEBUG] : Received command: %s\n", io_buff); //debugging log
        int rc = parse_command(io_buff, &clist);
        if (rc != OK) {
            printf("Error: parse_command() failed for input: %s\n", io_buff);
            send_message_string(cli_socket, "Failed to parse command\n");
        } else {
            int exit_code = rsh_execute_pipeline(cli_socket, &clist);
            if (exit_code != OK) {
                send_message_string(cli_socket, "Command execution failed\n");
            }
        }
        
        // Send EOF marker to indicate end of response
        send_message_eof(cli_socket);
    }
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

int send_message_eof(int cli_socket)
{
    char eof_char = RDSH_EOF_CHAR;
    int bytes_sent = send(cli_socket, &eof_char, 1, 0);

    return (bytes_sent == 1) ? OK : ERR_RDSH_COMMUNICATION;
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

int send_message_string(int cli_socket, char *buff)
{
    if (send(cli_socket, buff, strlen(buff), 0) < 0)
    {
        return ERR_RDSH_COMMUNICATION;
    }

    return send_message_eof(cli_socket);
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

int rsh_execute_pipeline(int cli_sock, command_list_t *clist)
{
    if (clist->num == 0)
        return WARN_NO_CMDS;

    int pipe_fds[2];
    int prev_fd = -1;

    for (int i = 0; i < clist->num; i++)
    {
        if (i < clist->num - 1)
        {
            if (pipe(pipe_fds) < 0)
            {
                perror("pipe failed");
                return ERR_EXEC_CMD;
            }
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            return ERR_EXEC_CMD;
        }

        if (pid == 0)
        { // Child process
            if (i == 0)
            {
                dup2(cli_sock, STDOUT_FILENO); // First command writes to client
            }
            else
            {
                dup2(prev_fd, STDIN_FILENO); // Get input from previous command
            }

            if (i < clist->num - 1)
            {
                dup2(pipe_fds[1], STDOUT_FILENO); // Send output to next command
            }
            if (i > 0)
            {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < clist->num - 1)
            {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }

            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1)
            {
                perror("execvp failed");
                send_message_string(cli_sock, "Command execution failed\n");
                exit(ERR_EXEC_CMD);
            }
        }
        else
        { // Parent process
            wait(NULL);
            if (i > 0)
            {
                close(prev_fd);
            }
            if (i < clist->num - 1)
            {
                close(pipe_fds[1]);
                prev_fd = pipe_fds[0];
            }
        }
    }
    return OK;
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
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
{
    return BI_NOT_IMPLEMENTED;
}
