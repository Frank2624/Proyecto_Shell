/*
   Une con un pipe dos procesos:
	> ./pipe ls sort
   es equivalente a:
	> ls | sort
*/
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int descf[2], anterior[2], i;
	pid_t pid;
	int status;

        if (2>argc){
		printf("\tError: Uso: %s prog1 prog2 ...\n", argv[0]);
		return -1;
	}

	for (i=1;i<argc;i++)
	{
		if (i<argc-1)
		{
			pipe(descf); /* se crea un pipe */
		}
		if (-1==(pid=fork()))
		{
			printf("Error: invocando fork\n");
                	return -1;
		}
		if (0==pid)
		{
			/* el proceso hijo cambia su salida estandar
			al pipe nuevo cerrando la entrada del pipe */
			if (i<argc-1) /* si no es el ultimo */
			{
				dup2(descf[1],fileno(stdout));
				close(descf[0]);
			}
			/* el proceso hijo cambia su entrada estandar
			al pipe anterior cerrando la entrada del pipe */
			if (i>1) /* si no es el primero */
			{
				dup2(anterior[0],fileno(stdin));
				close(anterior[1]);
			}
			/* ahora ejecuta el programa indicado en argv */
			execlp(argv[i],argv[i],NULL);
			/* si llega aqui es que exec fallo */
			return -1;
		}
		if (i>1)
		{
			close(anterior[0]);
			close(anterior[1]);
		}
		anterior[0]=descf[0];
		anterior[1]=descf[1];
	}
	waitpid(pid,&status,0);
	return 0;
}

