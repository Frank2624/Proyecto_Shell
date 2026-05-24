/*
   Redirección de la salida de un "programa" en un fichero.

   Compilar:
      gcc redirec.c
   Ejemplo de uso:
      ./a.out /tmp/salida.txt ls -alF
   
   Usa el fichero del primer argumento como salida del programa en
   sustitucion de la salida estandar. No hace ningún control de errores
   (no comprueba que exista el programa ni los permisos de ejecución)
*/

#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *outfile;
	int fnum1,fnum2;

        //Requiere 3 argumentos incluido el nombre del propio ejecutable
        if (3>argc){
		printf("\tError: Uso: %s ls /tmp/salida.txt\n", argv[0]);
		return(-1);
	}

	argv++;
	if (NULL==(outfile=fopen(argv[0],"w")))
	{
		printf("\tError: abriendo: %s\n",argv[0]);
		return(-1);
	} 

	fnum1=fileno(outfile);
	fnum2=fileno(stdout);
	dup2(fnum1,fnum2);

	argv++;
	execvp(argv[0],argv);

	fclose(outfile);
	return 0;
}

