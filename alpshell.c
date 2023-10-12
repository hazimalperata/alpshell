#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// COLORS
#define K_NRM  "\x1B[0m"
#define K_GRN  "\x1B[32m"
#define K_YEL  "\x1B[33m"
#define K_BLU  "\x1B[34m"
#define K_MAG  "\x1B[35m"
#define K_CYN  "\x1B[36m"
#define K_WHT  "\x1B[37m"
#define K_ERR  "\x1B[41m"

// MAX VALUES OF INPUTS/VARIABLES
#define PATH_MAX 100
#define MAX_CHARACTER 100
#define MAX_ARGUMENTS 10
#define MAX_HISTORY_COMMANDS 10


//GLOBAL VARIABLES
char PATH[PATH_MAX];
char *USER;
char *HOME_ENV;
char commands[MAX_HISTORY_COMMANDS][MAX_CHARACTER] = {0};


// THIS IS A FUNCTION FOR DELETING HOME ENV FROM PATH.
char *tilda_replacer() {
    char *tilda = strstr(PATH, HOME_ENV);                                                   // Has tilda sign
    if (tilda) {
        char *tempPath = calloc(1, sizeof PATH);
        memcpy(tempPath, &tilda[strlen(HOME_ENV)],
               strlen(PATH) -
               strlen(HOME_ENV));                                                    // Copying from path without HOME env
        char *newPath = calloc(1, sizeof PATH + 1);
        for (int i = 1; i < sizeof PATH + 1; i++) {
            newPath[i] = tempPath[i - 1];
        }
        newPath[0] = '~';
        free(tempPath);                                                                             // Adding tilda sign
        return newPath;
    } else {
        return PATH;
    }
}

// FUNCTIONS FOR HISTORY
void insert_command(char *command) {
    for (int i = MAX_HISTORY_COMMANDS - 1; i > 0; i--) {
        strcpy(commands[i], commands[i - 1]);                           // Shift operation
    }
    strcpy(commands[0], command);                                       // Insert the last command
}

void print_command_queue() {
    for (int i = 0; i < MAX_HISTORY_COMMANDS && commands[i][0] != 0; i++) {
        printf(K_WHT"[%d] %s\n", i + 1, commands[i]);                     // Classic print way of an array
    }
    printf(K_NRM);
}

// FUNCTIONS FOR BEAUTY SHELL
void print_shell() {
    printf(K_BLU "alpshell");
    printf(K_YEL "^%s:", USER);
    printf(K_MAG "%s", tilda_replacer());
    printf(K_GRN ">>> " K_NRM);
}

void print_exit() {
    printf(K_CYN "\n\t  Goodbye!\n");
    printf(K_CYN "(＾ω＾)づ\n");
    sleep(1);
}

void init_shell() {
    char *welcome_text = ("██     ██ ███████ ██       ██████  ██████  ███    ███ ███████     ████████  ██████       █████  ██      ██████  ███████ ██   ██ ███████ ██      ██      ██ \n"
                          "██     ██ ██      ██      ██      ██    ██ ████  ████ ██             ██    ██    ██     ██   ██ ██      ██   ██ ██      ██   ██ ██      ██      ██      ██ \n"
                          "██  █  ██ █████   ██      ██      ██    ██ ██ ████ ██ █████          ██    ██    ██     ███████ ██      ██████  ███████ ███████ █████   ██      ██      ██ \n"
                          "██ ███ ██ ██      ██      ██      ██    ██ ██  ██  ██ ██             ██    ██    ██     ██   ██ ██      ██           ██ ██   ██ ██      ██      ██         \n"
                          " ███ ███  ███████ ███████  ██████  ██████  ██      ██ ███████        ██     ██████      ██   ██ ███████ ██      ███████ ██   ██ ███████ ███████ ███████ ██ \n");
    printf(K_BLU "%s\n", welcome_text);
    sleep(1);
}

// CHANGE DIRECTORY FUNCTION
void change_directory(char **parsed) {
    if (parsed[1] ==
        NULL) {                                                // If empty cd is entered, we set the pwd to home
        chdir(HOME_ENV);
        setenv("PWD", HOME_ENV, 1);
        strcpy(PATH, HOME_ENV);
    } else {
        char *temp_path = calloc(1, sizeof PATH);
        strcpy(temp_path, PATH);
        strcpy(PATH, parsed[1]);
        if (chdir(PATH) == 0) {                                             // Valid PATH
            getcwd(PATH, sizeof PATH);
            setenv("PWD", PATH, 1);
        } else {
            strcpy(PATH, temp_path);                                // Invalid PATH
            printf(K_ERR"Invalid PATH entered..."K_NRM"\n");
        }
        free(temp_path);
    }
}

// EXECUTE COMMANDS
void execArgs(char **parsed, const char *hasBackgroundSign) {
    char *background = malloc(sizeof hasBackgroundSign);
    if (hasBackgroundSign) {
        strcpy(background, hasBackgroundSign);
        parsed[0][strcspn(parsed[0], "&")] = '\0';
    }
    pid_t pid = fork();
    if (pid == -1) {
        printf(K_ERR "Failed forking child..."K_NRM"\n");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf(K_ERR "Could not execute:%s..."K_NRM"\n", parsed[0]);       // Execvp has error. So we can't execute.
        }
        exit(0);
    } else {
        if ((int) background[0] != '&') {
            waitpid(pid, 0,
                    0);                                         // In background operations we need to wait the result of command
        }
        free(background);
        return;
    }
}

void execArgsPiped(char **parsed, char **parsedpipe) {
    int pipefd[2];
    pid_t p1_ID, p2_ID;

    if (pipe(pipefd) < 0) {
        printf(K_ERR "Pipe could not be initialized..."K_NRM"\n");
        return;
    }
    p1_ID = fork();
    if (p1_ID < 0) {
        printf(K_ERR "Could not fork\n..."K_NRM);
        return;
    } else if (p1_ID == 0) {                                                        // Child 1 process
        close(pipefd[0]);                                                       // Close read-end because we don't need
        dup2(pipefd[1],
             STDOUT_FILENO);                                     // We duplicate the pipe. So we can get the result of pipe2
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {                               // Execute the command
            printf(K_ERR "Could not execute in 1:%s..."K_NRM"\n", parsed[0]);        // We get an error
            exit(0);
        }
    } else {
        p2_ID = fork();

        if (p2_ID < 0) {
            printf(K_ERR "Could not fork..."K_NRM"\n");
            return;
        }
        if (p2_ID == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf(K_ERR "Could not execute in 2:%s..."K_NRM"\n", parsed[0]);
                exit(0);
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
        }
    }
}

// THE FUNCTION FOR `WHAT WILL WE DO?` DECISION.
int command_decider(char **parsed) {
    int i, switchOwnArg = 0;
    char *build_in_command_list[] = {"cd", "dir", "history", "bye"};
    int number_of_built_in_codes = 4;

    for (i = 0; i <
                number_of_built_in_codes; i++) {                                            // We compare two strings and we take its index.
        if (strcmp(parsed[0], build_in_command_list[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }

    switch (switchOwnArg) {
        case 1:
            change_directory(parsed);
            return 1;
        case 2:
            printf(K_MAG "%s\n", getcwd(PATH, sizeof(PATH)));                   // Print directory.
            return 1;
        case 3:
            print_command_queue();
            return 1;
        case 4:
            print_exit();
            exit(0);
        default:
            break;
    }
    return 0;
}

// GET INPUT FROM USER
int takeInput(char *str) {
    char buf[MAX_CHARACTER];
    print_shell();
    fgets(buf, MAX_CHARACTER, stdin);
    buf[strcspn(buf,
                "\n")] = '\0';                         // Unfortunately fgets function takes enter(\n) but we don't need to this. I fixed it that replace with (\0) end character.
    if (strlen(buf) != 0) {
        insert_command(buf);                                // Add history
        strcpy(str, buf);                                   // We get the clean command or garbage text ¯\_(ツ)_/¯
        return 0;
    } else {
        return 1;
    }
}


// FUNCTION FOR PARSING PIPE OPERATION
int parsePipe(char *str, char **strpiped) {
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }

    if (strpiped[1] ==
        NULL)                                                // We need to know is there a pipe. So there is return.
        return 0;
    else {
        return 1;
    }
}

// FUNCTION FOR PARSING SPACE OPERATION (SAME METHOD WITH PIPE)
void parseSpace(char *str, char **parsed) {
    for (int i = 0; i < MAX_ARGUMENTS; i++) {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL) {
            break;
        }
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

// THIS FUNCTION DECIDE THE COMMAND IS PIPED OR NOT. THEN IT'S PREPARING IT/THEM.
int processString(char *str, char **parsed, char **parsedpipe) {
    char *strpiped[2];

    int piped = parsePipe(str, strpiped);

    if (piped) {
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);

    } else {
        parseSpace(str, parsed);
    }

    return piped;
}

// MAIN FUNCTION
int main() {
    USER = getenv("USER");
    int execute_number;
    getcwd(PATH, sizeof(PATH));
    init_shell();
    HOME_ENV = getenv("HOME");
    while (1) {
        char inputString[MAX_CHARACTER], *parsedArgs[MAX_ARGUMENTS];
        char *parsedArgsPiped[MAX_ARGUMENTS];                                               // IDEs give warning for here because of infinite loop, but we have an exit way.
        if (takeInput(inputString)) {
            continue;
        }
        char *background_sign = strstr(inputString, "&");
        execute_number = processString(inputString, parsedArgs, parsedArgsPiped);

        if (command_decider(parsedArgs)) {
            continue;
        }
        if (!execute_number) {
            execArgs(parsedArgs, background_sign);
        } else {
            execArgsPiped(parsedArgs, parsedArgsPiped);
        }
    }
    return 0;
}