1. In this assignment I suggested you use `fgets()` to get user input in the main while loop. Why is `fgets()` a good choice for this application?

    > **Answer**:  fgets() is a good choice because it safely reads input from the user while preventing buffer overflows. fgets() allows us to specify a maximum input size, ensuring that user input does not exceed allocated memory. Also, fgets() reads until a newline character is encountered or the buffer is full, making it well-suited for handling command-line input.

2. You needed to use `malloc()` to allocte memory for `cmd_buff` in `dsh_cli.c`. Can you explain why you needed to do that, instead of allocating a fixed-size array?

    > **Answer**:   malloc() allows dynamic allocation of memory at runtime, which provides flexibility compared to using a fixed-size array. Allocating memory dynamically ensures that we use memory efficiently and avoid unnecessary memory consumption, especially in environments where memory is a constraint.


3. In `dshlib.c`, the function `build_cmd_list(`)` must trim leading and trailing spaces from each command before storing it. Why is this necessary? If we didn't trim spaces, what kind of issues might arise when executing commands in our shell?

    > **Answer**: If spaces are not trimmed:

    * The shell might misinterpret commands, causing them to fail.

    * Commands could be stored incorrectly in the structure, leading to unexpected behavior.

    * Extra spaces might cause incorrect argument parsing, especially when commands are tokenized.

    * User experience may be negatively impacted due to inconsistent handling of whitespace.



4. For this question you need to do some research on STDIN, STDOUT, and STDERR in Linux. We've learned this week that shells are "robust brokers of input and output". Google _"linux shell stdin stdout stderr explained"_ to get started.

- One topic you should have found information on is "redirection". Please provide at least 3 redirection examples that we should implement in our custom shell, and explain what challenges we might have implementing them.

    > **Answer**:  Output Redirection (> and >>)

    * ls > output.txt (writes output to a file)

    Challenge: We need to open the specified file for writing and redirect the command's output.

    Input Redirection (<)

    * sort < file.txt (reads input from a file instead of stdin)

    Challenge: The shell must properly replace stdin with the file input.

    * Error Redirection (2> and 2>>)

    gcc program.c 2> errors.log (redirects errors to a file)

    Challenge: Properly distinguishing stderr from stdout while executing commands.


- You should have also learned about "pipes". Redirection and piping both involve controlling input and output in the shell, but they serve different purposes. Explain the key differences between redirection and piping.

    > **Answer**: Redirection is used to send input/output to or from files, while piping is used to pass output from one command directly as input to another.

- STDERR is often used for error messages, while STDOUT is for regular output. Why is it important to keep these separate in a shell?

    > **Answer**:  STDERR separate from STDOUT is important because it allows the user to distinguish between normal output and error messages. This separation enables better debugging, logging, and redirection of errors without affecting standard output. 

- How should our custom shell handle errors from commands that fail? Consider cases where a command outputs both STDOUT and STDERR. Should we provide a way to merge them, and if so, how?

    > **Answer**:   Our custom shell should:

    * Detect and display errors properly by capturing stderr separately.
    * Allow users to redirect errors using 2> for logging.
    * Provide an option to merge stdout and stderr using 2>&1 when needed.
    * Return appropriate exit codes to indicate success or failure.