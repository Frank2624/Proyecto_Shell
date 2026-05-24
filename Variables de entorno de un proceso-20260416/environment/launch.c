// exit, EXIT_SUCCESS, EXIT_FAILURE
#include <stdlib.h>
// printf, puts,...
#include <stdio.h>
// fork, pid_t
#include <sys/types.h>
#include <unistd.h>
// wait
#include <sys/wait.h>
//

// enviroment pointer array
extern char **environ;

int print_array(char *array[])
{
	while (*array) puts(*array++);
	return 0;
}

int main(int argc, char *argv[])
{
	int wstatus;
	pid_t pid_wait, pid_fork;
	char *newvar;

	puts("==================");
	puts("Parent environment");
	puts("==================");
	clearenv();
	setenv("setenv_PARENT","1",1); // unsetenv(name);
	putenv("putenv_PARENT=2");
	print_array(environ);

	pid_fork = fork();
	switch (pid_fork) {
		case -1:
			perror("fork");
			exit(EXIT_FAILURE);
		case 0: // CHILD
			newvar="setenv_CHILD=3";
			puts("==============================================");
			puts("Child process");
			printf("New enviroment variable set to child process:\n\t%s\n",newvar);
			putenv(newvar); // unsetenv(name);
			if (argv[1]==NULL) {
				puts("==================");
				puts("Child  environment");
				puts("==================");
				argv[0]="/usr/bin/env";	// env shows environment
			} else {
				argv++;
			}
			execve(*argv,argv,environ);
			perror("execve");
			exit(EXIT_FAILURE);
		default: // PARENT
			pid_wait = wait(&wstatus);
			if (WIFEXITED(wstatus)) {
				printf("Child process %d ends with status %d\n",
						pid_wait,WEXITSTATUS(wstatus));
			} else if (WIFSIGNALED(wstatus)) {
				printf("Child process %d ends due to signal %d\n",
						pid_wait,WTERMSIG(wstatus));
			}
	}
	exit(EXIT_SUCCESS);
}
