/*
   Redirección de la salida de un "programa" en un fichero.

   Compilar:
      gcc redirec.c
   Ejemplo de uso:
      ./a.out entrada.txt salida.txt sort
   
   Usa el fichero del primer argumento como entrada del programa y el
   segundo fichero como salida del programa en sustitucion de las
   entradas y salidas estandar. No hace ningún control de errores
   (no comprueba que exista el programa ni los permisos de ejecución)
*/

#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	FILE *infile, *outfile;
	int fnum1,fnum2;

        //Requiere 4 argumentos incluido el nombre del propio ejecutable
        if (4>argc){
		printf("\tError: Uso: %s ls /tmp/salida.txt\n", argv[0]);
		return(-1);
	}

	argv++;
	if (NULL==(infile=fopen(argv[0],"r")))
	{
		printf("\tError: abriendo: %s\n",argv[0]);
		return(-1);
	} 

	fnum1=fileno(infile);
	fnum2=fileno(stdin);
	dup2(fnum1,fnum2);

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

	fclose(infile); // solo si exec falla
	fclose(outfile);
	return 0;
}

