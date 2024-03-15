#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <glob.h>
#include <stdbool.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

// funcion para designar el uso de (Ctrl+C)
void sigint_handler(int signum) {
    printf("\n"); // imprimir nueva linea
    fflush(stdout); // limpia el buffer de salida
}

void cd_command(char *args[]) {
    if (args[1] == NULL) {
        // si no se proporciona un argumento, cambiar al de usuario
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[1]) == -1) {
            perror("cd");
        }
    }
}

void export_command(char *args[]) {
    if (args[1] != NULL && args[2] != NULL) {
        setenv(args[1], args[2], 1);
    } else {
        printf("[!]: export VARIABLE\n");
    }
}

void execute_command(char *args[], bool background) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // ignora la regla SIGINT (Ctrl+C) en el proceso
        signal(SIGINT, SIG_IGN);

        // ejecuta el comando
        execvp(args[0], args);

        // si execvp devuelve, hay un error
        perror("execvp");
        exit(EXIT_FAILURE);
        /*
        // redireccion de entrada y salida si es necesario
        int in_fd, out_fd;
        if (args[2] != NULL && strcmp(args[2], "<") == 0) {
            in_fd = open(args[3], O_RDONLY);
            if (in_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (args[4] != NULL && strcmp(args[4], ">") == 0) {
            out_fd = open(args[5], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
*/
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("[%d] %s running in background\n", pid, args[0]);
        }
    }
}

int main() {
    char command[MAX_COMMAND_LENGTH];

    // manejo de la regla (Ctrl+C)
    signal(SIGINT, sigint_handler);

    while(1) {
        printf("luvShell$: ");

        // leer la entrada del usuario
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // tokenizar la entrada del usuario
        char *commands[MAX_ARGS];
        char *token = strtok(command, "|");
        int num_commands = 0;
        while (token != NULL && num_commands < MAX_ARGS - 1) {
            commands[num_commands++] = token;
            token = strtok(NULL, "|");
        }
        commands[num_commands] = NULL;

        // ejecucion de comandos en tuberia
        int fd[2];
        int in_fd = STDIN_FILENO;
        for (int i = 0; i < num_commands; i++) {
            char *args[MAX_ARGS];
            token = strtok(commands[i], " ");
            int j = 0;
            bool background = false;
            while (token != NULL && j < MAX_ARGS - 1) {
                if (strcmp(token, "&") == 0) {
                    background = true;
                    break;
                }
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;

            if (strcmp(args[0], "cd") == 0) {
                cd_command(args);
                continue;
            } else if (strcmp(args[0], "export") == 0) {
                export_command(args);
                continue;
            }

            pipe(fd);

            pid_t pid = fork();
            if(pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                dup2(in_fd, STDIN_FILENO);
                close(fd[0]);
                if (i < num_commands -1) {
                    dup2(fd[1], STDOUT_FILENO);
                }
                close(fd[1]);

                // ejecuta el comando
                execute_command(args, background);

                // si execute_command devuelve, hay un error
                exit(EXIT_FAILURE);
            } else {
                close(fd[1]);
                if (in_fd != STDIN_FILENO) {
                    close(in_fd);
                }
                in_fd = fd[0];
                if (!background) {
                    int status;
                    waitpid(pid, &status, 0);
                } else {
                    printf("[%d] %s running in background\n", pid, args[0]);
                }
            }
        }

        // esperar a q terminen todos los procesos en segundo plano
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    return 0;
}
/*

int main() {
    char command[MAX_COMMAND_LENGTH];

    // manejo de la regla (Ctrl+C)
    signal(SIGINT, sigint_handler);

    while(1) {
        printf("luvShell$: ");

        // leer la entrada del usuario
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // crear un proceso secundario para la ejecucion del comando
        char *args[MAX_ARGS];
        char *token = strtok(command, " ");
        int i = 0;
        bool background = false;
        while (token != NULL && i < MAX_ARGS - 1) {
            if (strcmp(token, "&") == 0){
                background = true;
                token = strtok(NULL, " ");
                continue;
            }
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] != NULL) {
            // verifica si se trata de un comando interno o no
            if (strcmp(args[0], "cd") == 0) {
                cd_command(args);
            } else if (strcmp(args[0], "export") == 0) {
                export_command(args);
            } else {
                // ejecutar comando externo
                execute_command(args, background);
            }
        }
    }

    return 0;
}

*/
