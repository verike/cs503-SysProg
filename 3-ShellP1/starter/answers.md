/* questions.md */
1. **Why is `fgets()` a good choice?**
   - `fgets()` is safer than `gets()` because it prevents buffer overflows by limiting input size.
   - It handles standard input and allows EOF checking for headless execution.

2. **Why use `malloc()` for `cmd_buff` instead of a fixed-size array?**
   - Dynamically allocating memory prevents unnecessary stack space usage and allows flexibility for longer input strings.

3. **Why trim spaces in `build_cmd_list()`?**
   - If spaces aren’t trimmed, empty commands may be processed incorrectly.
   - Prevents issues with command execution where arguments could have unexpected leading spaces.

4. **Three examples of redirection & challenges:**
   - `ls > output.txt`: Redirects STDOUT to a file. Challenge: Need to handle file I/O.
   - `cat < input.txt`: Reads from a file instead of STDIN. Challenge: Open and manage file input properly.
   - `grep error log.txt 2> errors.txt`: Redirects STDERR to a file. Challenge: Separate STDERR from STDOUT.

5. **Difference between redirection and piping:**
   - Redirection sends input/output to a file, while piping (`|`) connects commands by sending one’s output as another’s input.

6. **Why keep STDERR separate from STDOUT?**
   - Keeping them separate allows error messages to be seen even if output is redirected.

7. **How should our shell handle errors?**
   - Print errors to STDERR using `fprintf(stderr, ...)`.
   - Allow merging of STDERR and STDOUT with `2>&1` for debugging.