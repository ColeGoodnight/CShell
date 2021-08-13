#ifndef _defn_h
#define _defn_h

int expand (char *orig, char *new, int newsize);
int builtIn(char **line, int numInArgs);
void strmode(mode_t mode, char* p);
int processline(char* line, int fd[], int flags);

#define EXPAND 1
#define NOEXPAND 0
#define WAIT 2
#define NOWAIT 0

#define LINELEN 200000

#endif