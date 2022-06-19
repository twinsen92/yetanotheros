#include <stdio.h>
#include <unistd.h>

FILE *stdin = (FILE *)STDIN_FILENO;
FILE *stdout = (FILE *)STDOUT_FILENO;
FILE *stderr = (FILE *)STDERR_FILENO;
