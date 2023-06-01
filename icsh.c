/* ICCS227: Project 1: icsh
 * Name: Szu-Chi Hsu
 * StudentID: 6480921
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CMD_BUFFER 255

char buffer[MAX_CMD_BUFFER];
char prevbuffer[MAX_CMD_BUFFER] = "";
char shell;

int isWhitespaceOrNewline(const char* str) {
    while (*str != '\0') {
        if (!isspace(*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}
void external(char* input) {
    if (fork() == 0) {
        char copy[MAX_CMD_BUFFER];
        strcpy(copy, input);
        char* args[255];
        int i = 0;
        char* p = strtok(copy, " \n");
        while (p != NULL) {
            args[i++] = p;
            p = strtok(NULL, " \n");
        }
        args[i] = p;
        execvp(args[0], args);
    } else {
        wait(NULL);
    }
}

void echo(char* text) {
    if (text == NULL) printf("\n");
    else printf("%s\n", text);
}
void execute(char* input) {
    if (isWhitespaceOrNewline(input)) return;
    char copy[MAX_CMD_BUFFER];
    strcpy(copy, input);
    char *command = strtok(copy, " \n");
    if (strcmp(command, "echo") == 0) {
        strcpy(prevbuffer, buffer);
        echo(strtok(NULL, "\n"));
    } else if (strcmp(command, "!!") == 0) {
        if (strcmp(prevbuffer, "") != 0) {
            strcpy(buffer, prevbuffer);
            if (shell) printf("%s", prevbuffer);
            execute(prevbuffer);
        } else {
            return;
        }
    } else if (strcmp(command, "exit") == 0) {
        if (shell) printf("Goodbye\n");
        char* status = strtok(NULL, "\n");
        if (status != NULL) {
            exit((unsigned char)(0xFF & atoi(status)));
        } else {
            exit(0);
        }
    } else {
        strcpy(prevbuffer, buffer);
        external(input);
    }
}
int main(int argc, char *argv[]) {
    if (argc > 1) {
        shell = 0;
        FILE *file;
        file = fopen(argv[1], "r");
        while(1){
            fgets(buffer, 255, file);
            execute(buffer);
        } 
    } else {
        shell = 1;
        while (1) {
            printf("icsh $ ");
            fgets(buffer, 255, stdin);
            execute(buffer);
        }        
    }
}
