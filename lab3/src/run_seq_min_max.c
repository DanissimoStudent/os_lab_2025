#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        // Ошибка при создании процесса
        printf("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Код выполняется в дочернем процессе
        char *new_arguments[] = {"./sequential_min_max", argv[1], argv[2], NULL};

        execvp(new_arguments[0], new_arguments);

        // Если execvp вернул управление, значит произошла ошибка
        printf("execvp failed");
        return 1;
    }
    
    else {
        // Код выполняется в родительском процессе

        // Переменная хранит код завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        }
        
        else {
            printf("Child process failed\n");
        }
    }

    return 0;
}