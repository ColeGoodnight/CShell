/* 
 * ush - Lightweight shell implementation for unix systems built in c
 * Built for systems class
 *
 * Features:
 * - Unix commands with double quote functionality
 * - Environmental variable processing
 * - Built in commands (exit, envset, envunset, cd, shift, unshift, sstat file)
 I - Shell argument processing (calling ush)
 * - Wildcard expansion (directory)
 * - Command output expansion
 * - Pipelines
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "defn.h"

/* Gloabls */

char ** mainargs;
int mainargcount;
int shiftI;
FILE *fp;
int lastRetValue;
bool SIGINTH;

/* Prototypes */

int processline(char *line, int fd[], int flags);
char **arg_parse(char *line, int *argcptr);
char *remove_comments(char *line);
char** pipelineIdentification(char* line, int* pipes);

/* Shell main */

int main(int mainargc, char **mainargv)
{
  char buffer[LINELEN] = "";
  int len;
  bool interactive = false;
  mainargcount = mainargc;
  mainargs = mainargv;
  lastRetValue = -348523;

  // has args
  if (mainargc > 1) {
    /* try to open file, check for errors */
    fp = fopen(*(mainargv+1), "r");

    if (fp == NULL) {
      perror("could not open file");
      exit(127);
    }
    
  } else {
    // interactive mode
    fp = stdin;
    interactive = true;
  }

  while (1)
  {

    /* prompt and get line */
    if (interactive) {
      fprintf(stderr, "%% ");
    }

    if (fgets(buffer, LINELEN, fp) != buffer)
      break;

    /* Get rid of \n at end of buffer. */
    len = strlen(buffer);
    if (buffer[len - 1] == '\n')
      buffer[len - 1] = 0;

    /* remove comments from buffer */
    char* processed = remove_comments(buffer);

    /* Run it ... */
    int fd[1] = {-1};
    processline(processed, fd, EXPAND | WAIT);
  }

  if (!feof(fp))
    perror("read");

  fclose(fp);
  return 0; /* Also known as exit (0); */
}

  

/* removes comments from input line */
char * remove_comments(char *line) {
  int i = 0;
  bool flag = true;
  while (flag) {
    // check for comment marker
    if (*(line+i) == '#') {
      // check if first char in line
      if (i > 0) {
        // if not an env
        if (*(line+i-1) != '$') {
          *(line+i) = '\0';
          flag = false;
        }
      } else {
        *(line+i) = '\0';
        flag = false;
      }
    }
    // check for end of line
    if (*(line+i) == '\0') {
      flag = false;
    }

    i++;
  }

  return line;
}

char **arg_parse(char *line, int *argcptr)
{

  int args = 0;
  int i = 0;
  
  int x;
  char *temp = line;
  char *quotesChecker;
  bool flag = 1;
  bool inQuotes = 0;

  if (*temp == '\0')
  {
    *argcptr = 0;
    return NULL;
  }

  // increments through any preceeding spaces
  while (*temp == ' ')
  {
    i++;
    temp = line + i;
    // if encounters end of string, there are no arguments
    // sets the num of args to 0 and returns null
    if (*temp == '\0')
    {
      *argcptr = 0;
      return NULL;
    }
  }

  char* actualStart = temp;

  // if this point is reached there is at least one argument
  args++;

  // iterates through the remaining chars in the string
  while (flag)
  {

    // checks for quotes
    if (*temp == '"')
    {
      if (!inQuotes)
      {
        inQuotes = 1;
      }
      else
      {
        inQuotes = 0;
      }
    }

    // triggers on first space after
    // a non space char if not in quotes
    if (*temp == ' ' && !inQuotes)
    {
      
      int j = 0;

      // increments through trailing spaces
      while (*(line + j + i) == ' ')
      {
        j++;
      }

      // end of string reached - exit main loop
      if (*(line + i + j) == '\0')
      {
        flag = 0;
        *temp = '\0';
      }
      else
      { // else reached next arg
        *temp = '\0';
        temp = line + j + i;
        args++;
        i += j;
      }
    } else {
      i++;
      temp = (line + i);
    }

    // catch for no postceeding spaces
    if (*temp == 0)
    {
      flag = 0;
    }
  }

  // checks if quotes closed
  // standard error if not
  if (inQuotes)
  {
    perror("Standard Error");
    argcptr = 0;
    return NULL;
  }

  flag = 1;
  char **retArray = malloc(sizeof(char *) * (args + 2));
  i = 0;
  int j = 0;
  temp = line;



  char *start;

  // increments through string again to
  // assign pointers to args and eliminate
  // quotes so that the arguements work
  // (this would be better to do in the
  // previous loop with a dynamic data
  // structure, but time constraints)
  while (flag)
  {

    // check for preceeding spaces and quotes
    while (*temp == ' ' && *temp == '"')
    {
      i++;
      temp = (line + i);
    }

    // set start
    if (j == 0) {
      start = actualStart;
    } else {
      start = temp;
    }

    // iterate through current arg until null
    while (*temp != '\0')
    {
      //check for quote
      if (*temp == '"')
      {
        // shift everything right
        // of 'start' (inclusive)
        // to the right
        x = i;
        quotesChecker = (line + x);
        while ((quotesChecker) != start)
        {
          *quotesChecker = *(quotesChecker - 1);
          x--;
          quotesChecker = (line + x);
        }

        // reset start to truncate extra preceeding chars
        start = (quotesChecker + 1);
      }

      i++;
      temp = line + i;
    }

    *(retArray + j) = start;
    j++;

    i++;
    temp = line + i;

    // if reached num of args, no more args to assign
    if (j >= args)
    {
      flag = 0;
    }
  }

  retArray[j] = NULL;

  *argcptr = args;

  return retArray;
}

int processline(char *line, int fd[], int flags)
{
  pid_t cpid;
  int status;
  int numArgsPtr;
  char newLine[LINELEN] = {'\0'};
  int numPipes = 0;
  char ** pipeCommands;


  
  if (expand(line, newLine, LINELEN) == -1) {
    return -1;
  }
    
  pipeCommands = pipelineIdentification(newLine, &numPipes);
  
  // if there are pipes
  if (numPipes > 0) {

    char comp [LINELEN]; 
    char comp2 [LINELEN];
    //int saved_stdout = dup(1);
    int fd2[2];
    pipe(fd2);
    // for each command open a new pipe and call processline 
    // change fds to stdin and stdout
    // read data from pipe to new command
    for (int h = 0; h < numPipes+1; h++)
    {
      // store previous pipe output for next iteration
      strcpy(comp2, comp);

      // copy command into long string for command return
      strcpy(comp, pipeCommands[h]);

      // if first command has occured
      // copy previous output to end of command
      if (h>0) {
        int o = 0;
        int p = 0;
        bool flag = false;

        // cycle to end of command
        while (1) {

          // when hit end of command add space and start transfer
          if (comp[o] == '\0' && !flag) {
            flag = true;
            comp[o] = ' ';
            o++;
          }

          // take info from previous and add to comp
          if (flag) {
            comp[o] = comp2[p];
            p++;
            if (comp2[p] == '\0') {
              o++;
              comp[o] = comp2[p];
              break;
            }
          }
          o++;
        }
      }

      // if on last command, restore stdout and write to it
      // otherwise write to pipe
      if (numPipes == h) {
        int fd3[2] = {-1};
        processline(comp, fd3, WAIT | NOEXPAND);
      } else {
        processline(comp, fd2, WAIT | NOEXPAND);
      }
    }
    return 0;
  } 

  char **args = arg_parse(newLine, &numArgsPtr);

  if (numArgsPtr < 1)
  {
    return -1;
  }

  int builtI = builtIn(args, numArgsPtr);

  if(builtI == 0) {
    free(args);
    // close pipe write if in pipe
    if (fd[0] != -1) {
      close(fd[0]);
    }
    // return for built in command - 0 and up are taken by PIDs
    return -2;
  }

  /* Start a new process to do the job. */
  cpid = fork();

  if (cpid < 0)
  {
    free(args);
    /* Fork wasn't successful */
    perror("fork");
    // close pipe write if in pipe
    if (fd[0] != -1) {
      close(fd[0]);
      close(fd[1]);
    }
    // fork was not successful
    return -1;
  }

  /* Check for who we are! */
  if (cpid == 0)
  {
    /* We are the child! */
    // if made it to this point, ready to execute commands
    // change STDOUT to pipe write if valid fd
    if (fd[0] != -1) {
      // close stdout
      close(STDOUT_FILENO);
      // replace stdout with pipe write
      dup(fd[1]);
      // close pipe read
      close(fd[0]);
    }

    execvp(args[0], args);
    /* execvp reurned, wasn't successful */
    perror("exec");

    // close pipe write if in pipe
    if (fd[0] != -1) {
      close(fd[0]);
    }

    free(args); 
    fclose(fp); // avoid a linux stdio bug
    lastRetValue = 127;
    exit(127);
  } else if (fd[0] != -1) {
    // we are the parent, pipe is open
    close(fd[1]);
    int h = 0;
    char readF = ' ';
    bool flag = true;
    int nLcounter = 0;

    // keep reading until encoutner a long series on newline that signifies
    // end of write from other end
    while (flag) {
      read(fd[0], &readF, sizeof(char));
      if (readF == '\n') {
        nLcounter++;
        line[h] = ' ';
      } else {
        nLcounter = 0;
        line[h] = readF;
      }

      if (nLcounter > 4) {
        break;
      }
      
      h++;
    }
    line[h-4] = '\0';
  }

  if ((flags & WAIT)) {
    if (wait(&status) < 0) {
      /* Wait wasn't successful */
      perror("wait");
    }
  }


  free(args);
  

  // check for signal
  if (WIFSIGNALED(status)) {
      lastRetValue = 128 + WTERMSIG(status);

      // check for non SIGINT signal
      if (lastRetValue != 128+SIGINT) {
        printf("%s", strsignal(lastRetValue-128));
      }

      // check for core dump
      if (WCOREDUMP(status)) {
        printf(" (core dumped)");
      }
      return cpid;

    } else {
      lastRetValue = WEXITSTATUS(status);
      return cpid;
    }
}

// identifies any pipelines in line
// adds null terminators and returns number of pipes
char** pipelineIdentification(char* line, int *pipes) {
  char* start = line;
  *pipes = 0;
  while (*(line) != '\0') {
    if (*(line) == '|') {
      *pipes+=1;
      *(line) = '\0';
    }
    line = line+1;
  }

  char** ret = malloc(sizeof(char*)*(*(pipes)+1));
  *ret = start;
  int i = 1;

  while (i <= *pipes) {
    if (*(start) == '\0') {
      *(ret+i) = (start+1);
      i++;
    }
    start = start+1;
  }

  return ret;
}
