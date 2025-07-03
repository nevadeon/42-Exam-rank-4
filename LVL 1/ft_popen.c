#include <unistd.h>
#include <stdlib.h>

int ft_popen(const char *file, char *const av[], int type)
{
    if (!file || !av || (type != 'r' && type != 'w'))
        return (-1);

    int fd[2];

    if(pipe(fd) == -1)
        return -1;
    if (type == 'r')
    {
        if (fork() == 0)
        {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            execvp(file, av);
            exit(-1);
        }
        close(fd[1]);
        return (fd[0]);
    }
    if (type == 'w')
    {
        if (fork() == 0)
        {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);
            execvp(file, av);
            exit(-1);
        }
        close(fd[0]);
        return (fd[1]);
    }
    return (-1);
}


/*#include <stdio.h>
#include <string.h>
int main()
{
	//int fd = open("texte", O_RDONLY);
	int fd = ft_popen("lss", (char *const[]){"ls", NULL}, 'r');

	char buf[1];
	while(read(fd, buf, 1))
		write(1, buf, 1);

	close(fd);
	return (0);
}*/ 