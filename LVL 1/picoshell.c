#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int picoshell(char **cmds[])
{
    int i = 0;
    int fd[2];
    int in_fd = 0;   // stdin par défaut (0)
    int ret = 0;
    pid_t pid;
    int status;

    while (cmds[i])
    {
        if (cmds[i + 1]) // Si ce n’est pas la dernière commande
        {
            if (pipe(fd) == -1)
                return 1;
        }
        else
        {
            fd[0] = -1;
            fd[1] = -1;
        }

        pid = fork();
        if (pid < 0)
            return 1;

        if (pid == 0) // Enfant
        {
            if (in_fd != 0)
            {
                if (dup2(in_fd, 0) == -1)
                    exit(1);
                close(in_fd);
            }
            if (fd[1] != -1)
            {
                if (dup2(fd[1], 1) == -1)
                    exit(1);
                close(fd[1]);
                close(fd[0]);
            }
            execvp(cmds[i][0], cmds[i]);
            exit(1); // execvp échoué
        }
        else // Parent
        {
            if (in_fd != 0)
                close(in_fd);
            if (fd[1] != -1)
                close(fd[1]);
            in_fd = fd[0];
            i++;
        }
    }

    while (wait(&status) > 0)
    {
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            ret = 1;
        else if (!WIFEXITED(status))
            ret = 1;
    }

    return ret;
}