//
// ./launch2
// ./launch2 ./entorno
// ./launch2 ./arguments
// ./launch2 ./arguments @@ '@?' @putenv_PARENT # use '@?' to prevent the subtitution of the wildcard (?)
//


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
	int last_value = 126;
	char parent_pid_str[16];
	char **argo, **argd;

	sprintf(parent_pid_str,"%d",getpid());

	puts("==================");
	puts("Parent environment");
	puts("==================");
	//clearenv();
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
				// advance argv pointer & replace arguments
				argv++; // the program name
				for (argd=argo=argv; *argo; argo++) {
					if ((*argo)[0] == '@') {
						if ((*argo)[1] == '@') {
							*argd++ = parent_pid_str;
						} else if ((*argo)[1] == '?') {
							*argd=malloc(16);
							sprintf(*argd++,"%d",last_value);
						} else { // skip undefined variables
							if ((newvar=getenv(&(*argo)[1])) != NULL) {
								*argd++=newvar;
							}
						}
					} else {
						*argd++=*argo;
					}
				}
				*argd=*argo;
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
