#include "systemcalls.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

    /*
    * TODO  add your code here
    *  Call the system() function with the command set in the cmd
    *   and return a boolean true if the system() call completed with success
    *   or false() if it returned a failure
    */
    return system(cmd) != -1 ? true : false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    // If no command is provided, execution cannot proceed
    if (count < 1) {
        return false;
    }

    // Declare a variable argument list object
    va_list args;

    // Initialize the variable argument list to read arguments after 'count'
    va_start(args, count);

    // Create an array of string pointers for execv()
    // count arguments + 1 NULL terminator as required by execv()
    char *command[count + 1];

    // Loop through all variable arguments
    for (int i = 0; i < count; i++) {
        // Retrieve the next argument and store it in the command array
        command[i] = va_arg(args, char *);
    }

    // Add NULL terminator so execv() knows where the argument list ends
    command[count] = NULL;

    // Clean up the variable argument list
    va_end(args);

    // Create a new child process
    pid_t pid = fork();

    // If fork() fails, no child process was created
    if (pid < 0) {
        return false;
    }

    // This block executes only in the child process
    if (pid == 0) {

        // Replace the child process image with the program specified by command[0]
        // command[0] must be an absolute path
        // command is passed as argv[] to the new program
        execv(command[0], command);

        // If execv() returns, it failed and errno is set
        // Exit immediately with a non-zero status to notify the parent
        _exit(errno);
    }

    // This block executes only in the parent process

    int status;

    // Wait for the child process to terminate and store its exit status
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    // Check if the child terminated normally (not via a signal)
    if (!WIFEXITED(status)) {
        return false;
    }

    // Check if the child exited with a non-zero exit code
    // Non-zero means the command failed
    if (WEXITSTATUS(status) != 0) {
        return false;
    }

    // All steps succeeded and the command returned 0
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    if (count < 1 || outputfile == NULL) {
        return false;
    }

    va_list args;
    va_start(args, count);

    char *command[count + 1];
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        // Child process

        // Open or create output file, truncate if it exists
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            _exit(errno);
        }

        // Redirect stdout (fd 1) to the file
        dup2(fd, STDOUT_FILENO);

        // fd is no longer needed after duplication
        close(fd);

        // Replace child process with command
        execv(command[0], command);

        // execv failed
        _exit(errno);
    }

    // Parent process
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    if (!WIFEXITED(status)) {
        return false;
    }

    if (WEXITSTATUS(status) != 0) {
        return false;
    }

    return true;
}