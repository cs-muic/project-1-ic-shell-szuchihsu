/* ICCS227: Project 1: icsh
 * Name: Szu-Chi Hsu
 * StudentID: 6480921
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_CMD_BUFFER 255

char buffer[MAX_CMD_BUFFER];
char prevbuffer[MAX_CMD_BUFFER] = "";
char shell;
int exitcode = 0;
pid_t fg = 0;
typedef struct {
    pid_t pid;
    int jobID;
    char input[MAX_CMD_BUFFER];
    int status;
    int stop;
} Job;
int jobcount = 1;
Job jobs[255];

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
    //parsing
    int bg = 0;
    char copy[MAX_CMD_BUFFER];
    strcpy(copy, input);
    char* args[255];
    int i = 0;
    char* p = strtok(copy, " \n");
    while (p != NULL) {
        args[i++] = p;
        p = strtok(NULL, " \n");
    }
    //background command
    if (strcmp(args[i-1], "&") == 0) {
        bg = 1;
        args[i-1] = NULL;
    } else {
        args[i] = p;
    }
    //redirect
    int io = -1;
    char* in;
    char* out;
    int j = 0;
    while (args[j] != NULL) {
        if (strcmp(args[j], "<") == 0) {
            io = 1;
            in = args[j+1];
            args[j] = NULL;
        } else if (strcmp(args[j], ">") == 0) {
            io = 0;
            out = args[j+1];
            args[j] = NULL;
        }
        j++;
    }
    int saved_stdout = dup(1);
    int saved_stdin = dup(0);
    if (io == 1) {
        int infile = open(in, O_RDONLY);
        if (infile <= 0) {
            printf("Input file not found");
            exit(1);
        }
        dup2(infile, 0);
        close(infile);
    } else if (io == 0){
        int outfile = open(out, O_TRUNC | O_CREAT | O_WRONLY, 0666);
        if (outfile <= 0) {
            printf("Output file not found");
            exit(1);
        }
        dup2(outfile, 1);
        close(outfile);
    }
    //exec 
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        exit(1);
    } else {
        if (bg) {
            int jobID = jobcount++;
            jobs[jobID].pid = pid;
            jobs[jobID].jobID = jobID;
            jobs[jobID].status = 0;
            jobs[jobID].stop = 0;
            strcpy(jobs[jobID].input, input); 
        } else {
            int jobID = jobcount++;
            fg = jobID;
            jobs[jobID].pid = pid;
            jobs[jobID].jobID = jobID;
            jobs[jobID].status = 0;
            jobs[jobID].stop = 0;
            strcpy(jobs[jobID].input, input);
            waitpid(pid, &exitcode, WUNTRACED);
            if (WIFEXITED(exitcode)) {
                jobs[jobID].status = 1;
                fg = 0;
            }
        }
        dup2(saved_stdout, 1);
        dup2(saved_stdin, 0);
        close(saved_stdout);
        close(saved_stdin);
    }
}

void echo(char* text) {
    if (text == NULL) printf("\n");
    else if (strcmp(text, "$?") == 0) printf("%d\n", exitcode);
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
    } else if (strcmp(command, "jobs") == 0) {
        for (int i = 1; i < jobcount; i++) {
            if (jobs[i].stop) {
                printf("[%d]- Stopped \t%s", jobs[i].jobID, jobs[i].input);
            } else if (jobs[i].status == 0) {
                printf("[%d]- Running \t%s", jobs[i].jobID, jobs[i].input);
            }
        }
    } else if (strcmp(command, "fg") == 0) {
        int jobID = atoi(command + 4);
        printf("%s", jobs[jobID].input);
        fg = jobID;
        jobs[jobID].stop = 0;
        kill(jobs[jobID].pid, SIGCONT);
        waitpid(jobs[jobID].pid, &exitcode, 0);
        jobs[jobID].status = 1;
    } else if (strcmp(command, "bg") == 0) {
        int jobID = atoi(command + 4);
        printf("%s", jobs[jobID].input);
        jobs[jobID].stop = 0;
        kill(jobs[jobID].pid, SIGCONT);
    } else {
        strcpy(prevbuffer, buffer);
        external(input);
    }
}
void prompt() {
    shell = 1;
    while (1) {
        printf("icsh $ ");
        fgets(buffer, 255, stdin);
        execute(buffer);
    }  
}

void stophandler() {
    if (fg != 0) {
        kill(jobs[fg].pid, SIGTSTP);
        printf("[%d]+  Stopped \t%s", jobs[fg].jobID, jobs[fg].input);
        jobs[fg].stop = 1;
        prompt();
    }
}

void inthandler() {
    if (fg != 0) {
        kill(jobs[fg].pid, SIGINT);
        printf("[%d]+  Terminated \t%s", jobs[fg].jobID, jobs[fg].input);
        jobs[fg].status = 1;
    }
}

void childhandler() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        for (int i = 1; i < jobcount; i++) {
            if (jobs[i].pid == pid && i != fg) {
                if (WIFEXITED(status)) {
                    jobs[i].status = 1;
                    printf("[%d]+  Done \t%s", jobs[i].jobID, jobs[i].input);
                }
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    signal(SIGCHLD, childhandler);
    signal(SIGTSTP, stophandler);
    signal(SIGINT, inthandler);
    if (argc > 1) {
        shell = 0;
        FILE *file;
        file = fopen(argv[1], "r");
        while(1){
            fgets(buffer, 255, file);
            execute(buffer);
        } 
    } else {
        prompt();      
    }
}