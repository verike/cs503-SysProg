1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

_answer here_ : My implementation would utilize the waitpid() function that tells the parent process to hold on for the child process to complete it's execution before carrying out any parent process. 

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

_answer here_ : The reason unused pipe input and output ends should be closed is to avoid the process waiting to expect data flow from those pipe write and read ends.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

_answer here_ : Running cd in a child process would not affect the parent shell, ``cd`` changes the parent shell's working directory, which cannot be done from an external process.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

_answer here_ : 

`a.` Using dynamic memory allocation for command storage and pipes instead of a fixed array. 

`b.` Increased complexity and memory management, for better scalability.