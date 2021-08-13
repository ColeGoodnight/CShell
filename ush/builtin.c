/* CS 347
 * 
 * April 17, 2021, Cole Goodnight
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "defn.h"


/* Globals */
extern int shiftI;
extern char ** mainargs;
extern int mainargcount;
extern int lastRetValue;

/* Prototypes */
int exitShell(char **line, int numInArgs);
int setEnv(char **line, int numInArgs);
int unsetEnv(char **line, int numInArgs);
int changeDir(char **line, int numInArgs);
int unshift(char ** line, int numInArgs);
int shift(char ** line,  int numInArgs);
int sstat(char ** line, int numInArgs);
void exitBuiltIn(int num);

/* checks if the provided command is built in */
int builtIn(char **line, int numInArgs) {
    const int numArgs = 7;
    char args[][9] = {
        "exit",
        "envset",
        "envunset",
        "cd",
        "shift",
        "unshift",
        "sstat"
    };

    int (*funcs[])(char** line, int numInArgs) = {
        exitShell,
        setEnv,
        unsetEnv,
        changeDir,
        shift,
        unshift,
        sstat
    };

    for (int i = 0; i < numArgs; i++)
    {
        if (strcmp(args[i], line[0]) == 0) {
            (*funcs[i])(line, numInArgs);
            return 0;
        }
    }

    exitBuiltIn(1);
    return -1;
}

// exits
int exitShell(char **line, int numInArgs) {
    if (numInArgs>1) {
        int exitNum = (int) strtol(line[1], (char **)NULL, 10);
        lastRetValue = 0;
        exit(exitNum);
        return exitNum;
    }
    lastRetValue = 1;
    exit(0);
    return 0;
}

// shifts args over by n
int shift(char **line, int numInArgs) {
    if (numInArgs > 2) {
        perror("too many shift args");
        exitBuiltIn(1);
        return 1;
    }

    int shiftNum = 1;
    if (line[1] != NULL) {
        shiftNum = atoi(line[1]);
    }

    if (mainargcount-1-shiftNum < 1) {
        perror ("invalid shift value");
        exitBuiltIn(1);
        return 1;
    }

    if (shiftI + shiftNum > mainargcount-2) {
        perror("invalid shift");
        exitBuiltIn(1);
        return 1;
    }

    shiftI += shiftNum;
    exitBuiltIn(0);
    return 0;

}

// unshifts args, either by n or back to original order
int unshift(char **line, int numInArgs) {
    if (numInArgs > 2) {
        perror("too many unshift args");
        exitBuiltIn(1);
        return 1;
    }

    if (numInArgs > 1) {

        if (line[1][0] < '0' || line[1][0] > '9') {
            perror ("no unshift value");
            exitBuiltIn(1);
            return 1;
        }

        int shiftNum = atoi(line[1]);

        if (shiftNum > shiftI) {
            perror("invalid unshift value");
            exitBuiltIn(1);
            return 1;
        }

        if (shiftI-shiftNum < 1) {
            perror("invalid unshift");
            exitBuiltIn(1);
            return 1;
        }

        shiftI -= shiftNum;        
    } else {
        shiftI = 0;
    }

    exitBuiltIn(0);
    return 0;
}

// prints info from stat(2) call
int sstat (char **line, int numInArgs) {
    if (numInArgs < 2) {
        perror("Not enough commands");
        exitBuiltIn(1);
        return 1;
    }

    int i = 1;
    struct stat fileStats;
    struct passwd *userPass;
    struct group *groupPass;
    struct timespec lastMod;
    char userName[128];
    char groupName[128];

    while (line[i] != NULL) {
        stat(line[i], &fileStats);
        userPass = getpwuid(fileStats.st_uid);
        groupPass = getgrgid(fileStats.st_gid);
        lastMod = fileStats.st_mtim;
        time_t lastModTime = lastMod.tv_sec;
        strcpy(groupName, groupPass->gr_name);
        strcpy(userName, userPass->pw_name);
        char permissions[16];

        dprintf(STDOUT_FILENO,"%s ", line[i]);
        

        if (userName == NULL) {
            dprintf(STDOUT_FILENO,"%u ", fileStats.st_uid);
            
        } else {
            dprintf(STDOUT_FILENO,"%s ", userName);
            
        }

        if (groupName == NULL) {
            dprintf(STDOUT_FILENO,"%u ", fileStats.st_gid);
            
        } else {
            dprintf(STDOUT_FILENO,"%s ", groupName);
            
        }
        
        strmode(fileStats.st_mode, permissions);
        dprintf(STDOUT_FILENO,"%s ", permissions);
        

        dprintf(STDOUT_FILENO,"%ld ", fileStats.st_nlink);
        
        dprintf(STDOUT_FILENO,"%ld ", fileStats.st_size);
        
        dprintf(STDOUT_FILENO,"%s", asctime(localtime(&lastModTime)));
        

        i++;
    }

    exitBuiltIn(0);
    return 0;
}

// sets an enviromental variable to a value
int setEnv(char **line, int numInArgs) {
    if (numInArgs<3) {
        perror("Not enough args");
        exitBuiltIn(1);
        return 1;
    }
    if(setenv(line[1], line[2], 1) != 0) {
        perror("setenv error");
        exitBuiltIn(1);
        return 1;
    } else {
        exitBuiltIn(0);
        return 0;
    }
}

// unsets an enviromental variable
int unsetEnv(char **line, int numInArgs) {
    if (numInArgs<2) {
        perror("Not enough args");
        exitBuiltIn(1);
        return 1;
    }

    if(unsetenv(line[1]) != 0) {
        perror("setenv error");
        exitBuiltIn(1);
        return 1;
    } else {
        exitBuiltIn(0);
        return 0;
    }
}

// changes the working directory
int changeDir(char **line, int numInArgs) {
    if (numInArgs<2) {
        chdir(getenv("HOME"));
        exitBuiltIn(0);
        return 0;
    } else if (chdir(line[1]) != '\0') {
        perror("Directory error");
        exitBuiltIn(1);
        return 1;
    } else {
        exitBuiltIn(0);
        return 0;
    }
}

void exitBuiltIn(int exit) {
    if (exit == 0) {
        lastRetValue = 0;
        return;
    } else {
        lastRetValue = 1;
        return;
    }
}
