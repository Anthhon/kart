#ifndef FILE_H_
#define FILE_H_

void file_append_into(char* dir_path, char* dest);
bool file_exists(const char* path);
int file_write(const char *path, const char *fmt, ...);
int file_new(const char *path);

#endif // FILE_H_
