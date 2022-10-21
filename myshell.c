#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "parser.h"

void redireccion(int tipo);
int mandatoEspecial();
void manejador(int senal);

tline *linea; // global al utilizarlo en varias funciones
int **fds;
int numeroMandatos;

int main(void)
{
	char buff[1024];
	int i, j, k;

	printf("\nmsh> "); // promt

	// comprabar las dos señales y hacer la funcion manejador
	signal(SIGINT, manejador);	// para el CRT + C
	signal(SIGQUIT, manejador); // para el CTR + /

	while (fgets(buff, 1024, stdin)){

		linea = tokenize(buff);
		numeroMandatos = linea->ncommands;

		if (linea == NULL)	continue;

		else if (linea->ncommands == 1){ // hay un unico mandato

			// funcion por si es uno de los mandatos especialess
			if (!(strcmp(linea->commands[0].argv[0], "jobs")) || !(strcmp(linea->commands[0].argv[0], "fg")) || !(strcmp(linea->commands[0].argv[0], "cd")))
			{
				mandatoEspecial();
			}

			pid_t pid = fork();

			if (pid == -1){ // error
				fprintf(stderr, "Fallo en el fork()\n%s\n", strerror(errno));
				exit(1);
				break;
			}else if (pid == 0){ // proceso hijo

				if (linea->redirect_input != NULL)  redireccion(1); // redireccion de entrada
				if (linea->redirect_output != NULL)	redireccion(2); // redireccion de salida
				if (linea->redirect_error != NULL)	redireccion(3); // redireccion de error

				if (linea->background) 				printf("Comando a ejecutarse en background\n");

				execvp(linea->commands[0].filename, linea->commands[0].argv); // ejcecucion del primer y unico argumento pasado
				// si llega aqui es que ha dado error
				printf("%s:No se encuentra el mandato\n", linea->commands[0].argv[0]);
				exit(1);
			}
			else{	// proceso Padre
				wait(NULL); // espero al hijo
			}
		}
		else{ // varios mandatos

			if (numeroMandatos > 1){//siempre que se hayan introducido varios mandatos

				for (i = 0; i < numeroMandatos; i++){ // ninguno de los mandatos es de los especiales

					if (!(strcmp(linea->commands[i].argv[0], "jobs")) || !(strcmp(linea->commands[i].argv[0], "fg")) || !(strcmp(linea->commands[i].argv[0], "cd")))
					{
						printf("Uno de los comandos no es valido");
						exit(2);
					}
				}

				int pids[numeroMandatos]; // espacio para tantos hijos como comandos

				fds = (int **)malloc((numeroMandatos - 1) * sizeof(int *)); // matriz que hará tantas tuberias como mandatos menos uno, y CAD>
				if (fds == NULL){
					fprintf(stderr, "Error en la creacion de la matriz\n%s\n", strerror(errno));
					exit(2);
				}
				for (i = 0; i < numeroMandatos; i++){

					fds[i] = (int *)malloc(2 * sizeof(int));
					if (fds[i] == NULL){
						fprintf(stderr, "Error en la creacion del array\n%s\n", strerror(errno));
						exit(2);
					}
				}

				for (i = 0; i < numeroMandatos; i++){
					pipe(fds[i]);

					pids[i] = fork();
					if (pids[i] < 0){

						fprintf(stderr, "Fallo en el fork() numero %i\n%s\n", i, strerror(errno));
						exit(1);
					}
					else if (pids[i] == 0){ // proceso hijo
						if (i == 0){ // 1º mandato; el hijo no lee
							close(fds[0][0]);

							if (linea->redirect_input != NULL)	redireccion(1); //comprobacion de redireccion de entrada

							dup2(fds[0][1], STDOUT_FILENO);
							close(fds[0][1]);

							execvp(linea->commands[i].filename, linea->commands[0].argv);
							// si llega aqui es que ha dado error
							fprintf(stderr, "%s:No se encuentra el mandato\n", linea->commands[0].argv[0]);
							exit(2);
						}else if (i == (numeroMandatos)-1){ // ultimo mandato, el hijo no escribe

							close(fds[i - 1][1]);
							for (j = 0; j < (i - 1); j++){ // se cierran los anteriores
								for (k = 0; k < 2; k++){
									close(fds[j][k]);
								}
							}

							if (linea->redirect_output != NULL) redireccion(2); // redireccion de salida
							if (linea->redirect_error  != NULL) redireccion(3); // redireccion de error


							dup2(fds[i - 1][0], STDIN_FILENO); // entrada
							close(fds[i - 1][0]);

							execvp(linea->commands[i].filename, linea->commands[i].argv);
							// si llega aqui es que ha dado error
							fprintf(stderr, "%s: No se encuentra el mandato\n", linea->commands[0].argv[0]);
							exit(2);
						}else{ // mandato intermedio

							close(fds[i - 1][1]);
							for (j = 0; j < (i - 1); j++){ // se cierran los anteriores
								for (k = 0; k < 2; k++){
									close(fds[j][k]);
								}
							}

							dup2(fds[i - 1][0], STDIN_FILENO); // entrada
							dup2(fds[i][1], STDOUT_FILENO);
							close(fds[i - 1][0]);
							close(fds[i][1]);

							execvp(linea->commands[i].filename, linea->commands[i].argv);
							// si llega aqui es que ha dado error
							fprintf(stderr, "%s: No se encuentra el mandato\n", linea->commands[0].argv[0]);
							exit(2);
						}
					}
				}
				for (i = 0; i < numeroMandatos; i++){ // se finaliza cerrando todas las tuberias
					for (j = 0; j < 2; j++){
						close(fds[i][j]);
					}
				}
				for (i = 0; i < numeroMandatos; i++)
				{ // hay que esperar a que acaben los hijos
					wait(NULL);
				}
				for (i = 0; i < numeroMandatos; i++)	free(fds[i]); // liberacion de la memoria dinamica
				free(fds);
			}
		}
		printf("\nmsh> "); // promt, para que vuelva a aparecer
	}

	return 0;
}

void redireccion(int tipo){
	FILE *archivo;

	switch (tipo){ // tipo de redireccion que llega

		case 1: // tipo de lectura
			printf("Redireccion de entrada: %s\n", linea->redirect_input);
			archivo = fopen(linea->redirect_input, "r");
			if (archivo == NULL){
				fprintf(stderr, "fichero: Error\n%s\n", strerror(errno));
				exit(1);
			}
			dup2(fileno(archivo), STDIN_FILENO);
			break;
		case 2: // redireccion de escritura
			printf("Redireccion de salida: %s\n", linea->redirect_output);
			archivo = fopen(linea->redirect_output, "w+");
			if (archivo == NULL){
				fprintf(stderr, "fichero: Error.\n%s\n", strerror(errno));
				exit(1);
			}
			dup2(fileno(archivo), STDOUT_FILENO);

			break;
		case 3: // redireccion de error
			printf("Redireccion de error: %s\n", linea->redirect_error);
			archivo = fopen(linea->redirect_error, "w+");
			if (archivo == NULL){
				fprintf(stderr, "fichero: Error.\n%s\n", strerror(errno));
				exit(1);
			}
			dup2(fileno(archivo), STDERR_FILENO);
			break;
		default:
			printf("Redireccion no definida");
			break;
	}
	fclose(archivo);
}

int mandatoEspecial(){
	if (!(strcmp(linea->commands[0].argv[0], "cd"))){
		char miCwd[250];//current working directory
		if (linea->commands[0].argc == 2){
			chdir(linea->commands[0].argv[1]); // ir a la directorio indicado por teclado
			if(getcwd(miCwd, sizeof(miCwd))){//!=0
				printf("dir: %s",miCwd);
			}else	printf("Fallo al cambiar de directorio");
			return 0;
		}
		else if (linea->commands[0].argc == 1){
			chdir(getenv("HOME")); // con un parametro vamos a HOME (no funciona sin el getenv)
			if(getcwd(miCwd, sizeof(miCwd))){//!=0
                                printf("dir: %s",miCwd);
                        }else   printf("Fallo al cambiar de directorio");
                        return 0;
		}
		else{
			printf("Numero de parametros invalido");
			return (-1);
		}
	}
	return 1;
}

void manejador(int senal){ // IGN ignora la señal y DFL hace la señal por defecto
	if (senal){ //==1
		signal(SIGINT, SIG_IGN);
		signal(SIGINT, SIG_IGN);
	}
	else{
		signal(SIGINT, SIG_DFL);
		signal(SIGINT, SIG_DFL);
	}
}
