/* Assignment name:    picoshell
Expected files:        picoshell.c
Allowed functions:    close, fork, wait, exit, execvp, dup2, pipe
___

Write the following function:
int    picoshell(char *cmds[]);
The goal of this function is to execute a pipeline. It must execute each
commands [sic] of cmds and connect the output of one to the input of the
next command (just like a shell).
Cmds contains a null-terminated list of valid commands. Each rows [sic]
of cmds are an argv array directly usable for a call to execvp. The first
arguments [sic] of each command is the command name or path and can be passed
directly as the first argument of execvp.
If any error occur [sic], The function must return 1 (you must of course
close all the open fds before). otherwise the function must wait all child
processes and return 0. You will find in this directory a file main.c which
contain [sic] something to help you test your function.
Examples:
./picoshell /bin/ls "|" /usr/bin/grep picoshell
picoshell
./picoshell echo 'squalala' "|" cat "|" sed 's/a/b/g'
squblblb/
*/

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// Here is a first version of the code THAT DOES NOT HANDLE errors
// so you can focus on the logic. The sentinel value -1 is used to detect
// when we are in the case of the first or last command
int	picoshell(char **cmds[])
{
    pid_t pid;
    int pipefd[2];
    int in_fd = -1;
    int status;

    int i = 0;
    while (cmds[i])
    {
        pipefd[0] = -1;
        pipefd[1] = -1;
        if (cmds[i + 1]) // if there is one more command
            pipe(pipefd); // potential error

        pid = fork(); // potential error

        // child
        if (pid == 0)
        {
            if (pipefd[1] != -1)
            {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO); // potential error
                close(pipefd[1]);
            }
            if (in_fd != -1)
            {
                dup2(in_fd, STDIN_FILENO); // potential error
                close(in_fd);
            }
            execvp(cmds[i][0], cmds[i]);
            exit(EXIT_FAILURE); // failed execvp
        }

        // parent
        if (in_fd != -1)
            close(in_fd);
        if (pipefd[1] != -1)
            close(pipefd[1]);
        in_fd = pipefd[0];
        i++;
    }

    // Waits and collects status of all remaining child processes to avoid zombie processes
    while (wait(&status) > 0)  // potential error
        ;
    return 0;
}

// When an error occurs we have to close all open fds and wait for all already
// started child processes. Here is the complete version. All error cases are
// probably not tested but I can't know which one are so GL HF
int	picoshell(char **cmds[])
{
    pid_t pid;
    int pipefd[2];
    int in_fd = -1;
    int status;

    int i = 0;
    while (cmds[i])
    {
        pipefd[0] = -1;
        pipefd[1] = -1;
        if (cmds[i + 1] && pipe(pipefd) == -1)
        {
            if (in_fd != -1)
                close(in_fd);
            while (wait(&status) > 0)
                ;
            return 1;
        }

        pid = fork();
        if (pid == -1)
        {
            if (pipefd[0] != -1)
                close(pipefd[0]);
            if (pipefd[1] != -1)
                close(pipefd[1]);
            if (in_fd != -1)
                close(in_fd);
            while (wait(&status) > 0)
                ;
            return 1;
        }

        // child
        if (pid == 0)
        {
            if (pipefd[1] != -1)
            {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) == -1)
                {
                    close(pipefd[1]);
                    if (in_fd != -1)
                        close(in_fd);
                    exit(EXIT_FAILURE);
                }
                close(pipefd[1]);
            }
            if (in_fd != -1)
            {
                if (dup2(in_fd, STDIN_FILENO) == -1)
                {
                    close(in_fd);
                    exit(EXIT_FAILURE);
                }
                close(in_fd);
            }
            execvp(cmds[i][0], cmds[i]);
            exit(EXIT_FAILURE);
        }

        // parent
        if (in_fd != -1)
            close(in_fd);
        if (pipefd[1] != -1)
            close(pipefd[1]);
        in_fd = pipefd[0];
        i++;
    }

    int ret = 0;
    while (wait(&status) > 0)
    {
        // If child exited normally but with a non 0 status we return an error
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            ret = 1;
        // If child terminates for any abnormal reason
        else if (!WIFEXITED(status))
            ret = 1;
    }
    return ret;
}
