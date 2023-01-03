// args should point to start of the arguments, which are joined by the null
// terminator i.e. args = location("\0".join(arguments)) len should be the
// length of the args string (including all the null terminator) Example:
// argc=3, len = **12**, args = location("abc\0def\0ghi\0") int
// create_proccess(const char* file_path, int argc, int len, char* args);
int create_proccess(const char *file_path, int argc, int len, char *args);