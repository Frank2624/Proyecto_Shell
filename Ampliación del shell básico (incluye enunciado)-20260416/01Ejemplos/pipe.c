/*
   Une con un pipe dos procesos:
	> ./pipe ls sort
   es equivalente a:
	> ls | sort
*/

#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int descf[2], fno;

        //Requiere 3 argumentos incluido el nombre del propio ejecutable
        if (3>argc){
		printf("\tError: Uso: %s prog1 prog2\n", argv[0]);
		return(-1);
	}

	pipe(descf); /* se crea un pipe */
        if (fork())
	{
		/* el proceso padre ejecuta el primer programa y cambia su
		salida estandar al pipe cerrando la entrada del pipe */
		fno=fileno(stdout);
		dup2(descf[1],fno);
		close(descf[0]);
		execlp(argv[1],argv[1],NULL);
	}
	else
	{
		/* el proceso hijo tiene una copia del pipe del padre,
		en el fork, ejecuta el segundo programa y cambia su
		entrada estandar por el pipe cerrando la salida del pipe */
		fno=fileno(stdin);
		dup2(descf[0],fno);
		close(descf[1]);
		execlp(argv[2],argv[2],NULL);
	}
	return 0;
}

