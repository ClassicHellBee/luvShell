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

void execute_command(char *args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void expand_wildcards(char *args[]) {
    glob_t glob_result;
    int i = 0;
    while (args[i] != NULL) {
        if (strpbrk(args[i], "*?[]") != NULL) {
            glob(args[i], GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result);
            for (int j = 0; j < glob_result.gl_pathc; j++) {
                args[i + j] = strdup(glob_result.gl_pathv[j]);
            }
            args[i + glob_result.gl_pathc] = NULL;
            globfree(&glob_result);
        }
        i++;
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

        // crear un proceso secundario para la ejecucion del comando
        char *args[MAX_ARGS];
        char *token = strtok(command, " ");
        int i = 0;
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (args[0] != NULL) {
            // verifica si se trata de un comando interno o no
            if (strcmp(args[0], "cd") == 0) {
                cd_command(args);
            } else {
                // expandir las wildcards en los argumentos
                expand_wildcards(args);
                // ejecutar comando externo
                execute_command(args);
            }
        }
    }

    return 0;
}

/*
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            char *args[] = {"/bin/sh", "-c", command, NULL};
            execvp(args[0], args);

            // si execvp retorna, hubo un perror
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
*/