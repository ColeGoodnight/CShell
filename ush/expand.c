/* CS 347
 * 
 * April 13, 2021, Cole Goodnight
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include "defn.h"

/* Globals */

extern char ** mainargs;
extern int mainargcount;
extern int shiftI;
extern int lastRetValue;
extern bool SIGINTH;

/* Expand main */

int expand (char *orig, char *new, int newsize) {
    char* temp = orig;
    bool flag = 1;

    // iteratable for new array
    int i = 0;

    // iteratable for old array
    int x = 0;

    int j = 0;

    while (flag) {

        if (i >= newsize) {
            // new array not big enough
            perror("Not enough allocated space");
            return -1;
        }

        if (*temp == '\0') {
            flag = 0;
        }

        // check for expansion
        if (*temp == '$' && *(temp+1) != '\0') 
        {
            // command expansion
            if (*(temp+1) == '(') {
                char newCommand [LINELEN]; 
                int open = 0;
                int y = 0;

                x+=2;
                temp = orig+x;

                // parse command from line
                while (1) {

                    if (*(temp) == ')' && open == 0) {
                        break;
                    }

                    // error if hit EOF
                    if (*(temp) == '\0') {
                        perror("No closing parens for command expansion");
                        return -1;
                    }

                    // keep track of parens
                    if (*(temp) == '(') {
                        open++;
                    }

                    if (*(temp) == ')') {
                        open--;
                    }

                    newCommand[y] = *(temp);
                    y++;
                    x++;
                    temp = orig + x;
                }

                x++;
                temp = orig + x;


                newCommand[y] = '\0';

                // open pipe
                int fd[2];

                if (pipe(fd) == -1) {
                    perror("pipe error occured");
                }

                // call processline on new command 
                // with fd from pipe
                // new process will be created at exec
                // result will be stored in newCommand
                processline(newCommand, fd, EXPAND | WAIT);

                int k = 0;

                // add returned value to new string
                while (newCommand[k] != '\0') { 
                    *(new+i) = newCommand[k];
                    i++;
                    k++;

                    if (i >= newsize) {
                        // new array not big enough
                        perror("Not enough allocated space");
                        return -1;
                    }
                }

                continue;
            }


            // check for exit value expand
            if (*(temp+1) == '?') {
                if (lastRetValue == -348523) {
                    perror ("No ret value to pull from");
                    return -1;
                }

                int k = 0;
                char numStr[1024]; 
                sprintf(numStr, "%d", lastRetValue);

                // add returned value to new string
                while (*(numStr+k) != '\0') { 
                    *(new+i) = *(numStr+k);
                    i++;
                    k++;

                    if (i >= newsize) {
                        // new array not big enough
                        perror("Not enough allocated space");
                        return -1;
                    }
                }

                x+=2;
                temp = orig+x;
                continue;
            }

            // check if enviromental variable call
            if(*(temp+1) == '{') {
                j = x+2;
                int k = 0;
                char command[100] = {'\0'};
                bool comFlag = 1;

                // iterate forwards in array
                while (comFlag) {

                    // encounter end of string
                    if (*(orig+j) == '\0') {
                        perror("No Closing Brace");
                        return -1;
                    }

                    // encounter end brace
                    if (*(orig+j) == '}') {
                        // set index for old array to start after command
                        x=j+1;
                        temp = orig+x;
                        comFlag = 0;
                        // continue
                    } else {
                        
                        // add current char to command array
                        strncat(command, (orig+j), 1);
                        j++;
                    }
                }

                comFlag = 1;
                char* envvar = getenv(command);

                if (envvar == NULL) {
                    perror("No variable found");
                    return -1;
                }
                
                // add returned value to new string
                while (*(envvar+k) != '\0') { 
                    *(new+i) = *(envvar+k);
                    i++;
                    k++;

                    if (i >= newsize) {
                        // new array not big enough
                        perror("Not enough allocated space");
                        return -1;
                    }
                }
                continue;
            }
        

            if (*(temp+1) == '#') {
                char str[100];
                if (mainargcount-shiftI-1 == 0) {
                    strcpy(str, "1");
                } else {
                    sprintf(str, "%d", mainargcount-shiftI-1);
                }
                
                int h = 0;
                
                while (str[h] != '\0') {
                    *(new+i) = str[h];
                    h++;
                    i++;
                }

                x += 2;
                temp = orig+x;
                continue;
            }
                
            if (*(temp+1) == '$') {
                int PID;
                PID = getpid();
                char * mypid = malloc(11);
                sprintf(mypid, "%d", PID);
                
                bool pidFlag = 1;
                int h = 0;

                while (pidFlag) {
                    if (*(mypid+h) != '\0') {
                        *(new+i) = *(mypid+h);
                        i++;
                        h++;
                    } else {
                        pidFlag = 0;
                    }

                    if (i >= newsize) {
                        // new array not big enough
                        perror("Not enough allocated space");
                        return -1;
                    }
                }
                x+=2;
                temp = orig+x;
                continue;
            }

            /* postceeded by number */
            if (*(temp+1) >= '0' && *(temp+1) <= '9') {
                char num[20];
                bool numFlag = true;
                int h = 0;

                /* catch any multidigit numbers */
                while (numFlag) {
                    if (*(temp+1+h) >= '0' && *(temp+1+h) <= '9') {
                        num[h] = *(temp+1+h);
                        h++;
                    } else {
                        numFlag = false;
                    }
                }

                int index = atoi(num);
                char envVar[1024];

                if (mainargcount < 2) {
                    if (index == 0) {
                        strcpy(envVar, "./ush");
                    } else {
                        strcpy(envVar, "");
                    }
                } else {
                    if (index == 0) {
                        index++;
                    } else {
                        index+=1+shiftI;
                    }
                    
                    if (index >= mainargcount) {
                        strcpy(envVar, "");
                    } else {
                        strcpy(envVar, mainargs[index]);
                    }  
                }
                
                numFlag = true;
                int k = 0;

                if (envVar == NULL) {
                    perror("Variable not found");
                    return -1;
                }

                while (*(envVar+k)!='\0') {
                    *(new+i) = *(envVar+k);
                    i++;
                    k++;
                }   
                temp = temp+h+1;
                x = x+h+1;
                continue;
            } 
        } 

        // wildcard expansion
        if (*temp == '*') {
            
            // check for non whitespace
            if (*(temp-1) != ' ' && *(temp-1) != '\0') {

                // backslash
                if (*(temp-1) == '\\') {
                    *(new+i-1) = '*';
                    x++;
                    temp = orig+x;
                    continue;
                } else {
                    // any other char
                    *(new+i) = *temp;
                    i++;
                    x++;
                    temp = orig+x;
                }
                continue;
                
            } else {

                // has whitespace preceeding
                // attempt to open directory
                DIR *d;
                struct dirent *dir;
                d = opendir(".");
                if (!d) {
                    perror("not a valid directory");
                    return -1;
                }
                char tempDir [100];
                bool simple = (*(temp+1) == ' ' || *(temp+1) == '\0');

                // get length of and check for / in file search
                int searchLength = 1;
                while (temp[searchLength]!=' ' && 
                       temp[searchLength]!=0) {

                    if (*(temp+searchLength) == '/') {
                        perror ("invalid file type");
                        return -1;
                    }
                    searchLength++;
                }
                searchLength--;

                bool matched = false;
                char search[searchLength];
                memcpy(search, temp+1, searchLength);


                // while there are still lines to be read
                while ((dir = readdir(d)) != NULL) {
                    strcpy(tempDir, dir->d_name);

                    // skip files starting with .
                    if (tempDir[0] == '.') {
                        continue;
                    }

                    // check for simple wildcard
                    if (!simple) {
                        

                        int k = 0;
                        while (tempDir[k] != 0) {
                            k++;
                        }

                        if (k<searchLength) {
                            continue;
                        }

                        // compare last searchLength chars of tempDir
                        char suffix[searchLength];
                        memcpy(suffix, &tempDir[k-searchLength], searchLength);

                        bool compare = true;

                        for (int f = 0; f < searchLength; f++)
                        {
                            if (suffix[f] != search[f]) {
                                compare = false;
                                break;
                            }
                        }

                        if (compare) {
                            matched = true;
                            int len = strlen(tempDir);
                            for (int j = 0; j<len; j++) {
                                *(new+i) = tempDir[j];
                                i++;
                            }

                            *(new+i) = ' ';
                            i++;
                            
                        }

                    } else {
                        matched = true;
                        int len = strlen(tempDir);
                        for (int j = 0; j<len; j++) {
                            *(new+i) = tempDir[j];
                            i++;
                        }

                        *(new + i) = ' ';
                        i++;
                    }
                }

                if (!matched) {
                    int len = strlen(temp);
                    for (int n = 0; n <= len; n++)
                    {
                        *(new+i) = *temp;
                        i++;
                        x++;
                        temp = orig+x;
                    }
                    
                }

                int y = 0;
                while (temp[y]!=' ' && 
                       temp[y]!=0) {
                    y++;
                    x++;
                }

                temp = orig+x;

                i--;
                closedir(d);
                continue;
            }

        } else {
            new[i] = *temp;
            i++;
            x++;
            temp = orig+x;
        }
        
    }

    return 0;
}