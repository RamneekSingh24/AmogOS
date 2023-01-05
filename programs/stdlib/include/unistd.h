// args should point to start of the arguments, which are joined by the null
// terminator i.e. args = location("\0".join(arguments)) len should be the
// length of the args string (including all the null terminator) Example:
// argc=3, len = **12**, args = location("abc\0def\0ghi\0") int
// create_proccess(const char* file_path, int argc, int len, char* args);
int create_proccess(const char *file_path, int argc, int len, char *args);

// TODO add support for getting the status
// int waitpid(int pid, int*status)
// or waitpid is block and returns the status

// Non blocking waitpid
// returns 0 if the process exited
// -ve error code otherwise
int waitpid(int pid);