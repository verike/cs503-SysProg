1. Can you think of why we use `fork/execvp` instead of just calling `execvp` directly? What value do you think the `fork` provides?

    > **Answer**:  Fork creates a new process, allowing the parent shell to continue running while the child executes the command. This separation is essential. if execvp were called directly in the shell, the shell itself would be replaced by the new program.

2. What happens if the fork() system call fails? How does your implementation handle this scenario?

    > **Answer**:  Fork failure is due to resource limits. My implementation prints an error message using perror("Fork failed") and skips the command execution.

3. How does execvp() find the command to execute? What system environment variable plays a role in this process?

    > **Answer**: execvp() searches for the command in directories listed in the PATH environment variable.

4. What is the purpose of calling wait() in the parent process after forking? What would happen if we didn’t call it?

    > **Answer**: wait() ensures that the parent waits for the child to finish, ensuring proper cleanup.

5. In the referenced demo code we used WEXITSTATUS(). What information does this provide, and why is it important?

    > **Answer**: This extracts the exit status of the child process, useful for error handling and return code checks.

6. Describe how your implementation of build_cmd_buff() handles quoted arguments. Why is this necessary?

    > **Answer**: It treats text inside quotes as a single argument, preserving spaces within quotes.

7. What changes did you make to your parsing logic compared to the previous assignment? Were there any unexpected challenges in refactoring your old code?

    > **Answer**: Refactored to handle single commands only; managing quotes and spaces was challenging.

8. For this quesiton, you need to do some research on Linux signals. You can use [this google search](https://www.google.com/search?q=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&oq=Linux+signals+overview+site%3Aman7.org+OR+site%3Alinux.die.net+OR+site%3Atldp.org&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzc2MGowajeoAgCwAgA&sourceid=chrome&ie=UTF-8) to get started.

- What is the purpose of signals in a Linux system, and how do they differ from other forms of interprocess communication (IPC)?

    > **Answer**: Signals allow processes to handle asynchronous events like termination requests or interruptions while whle performing a process.

- Find and describe three commonly used signals (e.g., SIGKILL, SIGTERM, SIGINT). What are their typical use cases?

    > **Answer**:  
    - SIGKILL: Forces a process to terminate immediately.
    - SIGTERM: Requests a process to terminate.
    - SIGINT: Interrupts a process, usually via Ctrl+C.

- What happens when a process receives SIGSTOP? Can it be caught or ignored like SIGINT? Why or why not?

    > **Answer**: SIGSTOP halts a process and cannot be caught or ignored.