
#include "fifo_processes.h"

#include "global_defs.h"
#include <malloc.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "sys/types.h"
//#include "winsock.h"

#include "analysis.h"
#include "utility.h"

void make_fifos(uint16_t processes_count, char *file_format) {
}

void erase_fifos(uint16_t processes_count, char *file_format) {
    for (int i = 0; i < processes_count; i++) {
        char fifo_name[256];
        sprintf(fifo_name, file_format, i);
        if (unlink(fifo_name) < 0) {
            perror("unlink");
            exit(EXIT_FAILURE);
        }
    }
}

pid_t *make_processes(uint16_t processes_count) {
    pid_t *pids = malloc(processes_count * sizeof(pid_t));
    if (pids == NULL) {
        perror("malloc failed");
        return NULL;
    }

    for (int i = 0; i < processes_count; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            free(pids);
            return NULL;
        }
        if (pid == 0) {
            exit(EXIT_SUCCESS);
        } else {
            pids[i] = pid;
        }
    }

    return pids;
}

int *open_fifos(uint16_t processes_count, char *file_format, int flags) {
    int* fds = malloc(processes_count * sizeof(int));
    if (fds == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < processes_count; i++) {
        char fifo_name[256];
        sprintf(fifo_name, file_format, i);
        fds[i] = open(fifo_name, flags);
        if (fds[i] < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
    }

    return fds;
}


void close_fifos(uint16_t processes_count, int *files) {
    for (int i = 0; i < processes_count; i++) {
        if (close(files[i]) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }
}


void shutdown_processes(uint16_t processes_count, int *fifos) {
    char task[256];
    memset(task, 0, sizeof(task));

    for (uint16_t i = 0; i < processes_count; i++) {
        if (write(fifos[i], task, sizeof(task)) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
            }
    }

}


int prepare_select(fd_set *fds, const int *filesdes, uint16_t nb_proc) {
    FD_ZERO(fds);// Initialize the fd_set to have no file descriptors set

    //Set all file descriptors in the fd_set
    int max_fd = 0;
    for (int i = 0; i < nb_proc; i++) {
        FD_SET(filesdes[i], fds);
        max_fd = (max_fd > filesdes[i]) ? max_fd : filesdes[i];
    }

    return max_fd;
}


void send_task(char *data_source, char *temp_files, char *dir_name, int command_fd) {

        char command[1024];
        snprintf(command, 1024, "dir %s/%s > %s/%s", data_source, dir_name, temp_files, dir_name);

        write(command_fd, command, strlen(command));

    }


void fifo_process_directory(char *data_source, char *temp_files, int *notify_fifos, int *command_fifos, uint16_t nb_proc) {
    if (data_source == NULL || temp_files == NULL || notify_fifos == NULL || command_fifos == NULL || nb_proc == 0) {
        fprintf(stderr, "Invalid parameters\n");
        return;
    }
    DIR *dir = opendir(data_source);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }
    struct dirent *entry;
    int current_proc = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char task[1024];
        snprintf(task, sizeof(task), "PROCESS_DIRECTORY %s/%s %s", data_source, entry->d_name, temp_files);
        if (write(command_fifos[current_proc], task, strlen(task)) < 0) {
            perror("Error writing to command FIFO");
        }

        char response[1024];
        if (read(notify_fifos[current_proc], response, sizeof(response)) < 0) {
            perror("Error reading from notify FIFO");
        }
        current_proc = (current_proc + 1) % nb_proc;
    }
    closedir(dir);

    for (int i = 0; i < nb_proc; i++) {
        close(notify_fifos[i]);
        close(command_fifos[i]);
    }
}


void fifo_process_files(char *data_source, char *temp_files, int *notify_fifos, int *command_fifos, uint16_t nb_proc) {
    if (data_source == NULL || temp_files == NULL || notify_fifos == NULL || command_fifos == NULL || nb_proc == 0) {
        fprintf(stderr, "Invalid parameters\n");
        return;
    }

    DIR *dir = opendir(data_source);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }
    struct dirent *entry;
    int current_proc = 0;
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        if (stat(entry->d_name, &st) < 0) {
            perror("Error getting file status");
            continue;
        }
        if (!S_ISREG(st.st_mode)) {
            continue;
        }
        char task[1024];
        snprintf(task, sizeof(task), "PROCESS_FILE %s/%s %s", data_source, entry->d_name, temp_files);
        if (write(command_fifos[current_proc], task, strlen(task)) < 0) {
            perror("Error writing to command FIFO");
        }

        char response[1024];
        if (read(notify_fifos[current_proc], response, sizeof(response)) < 0) {
            perror("Error reading from notify FIFO");
        }
        current_proc = (current_proc + 1) % nb_proc;
    }
    closedir(dir);

    for (int i = 0; i < nb_proc; i++) {
        close(notify_fifos[i]);
        close(command_fifos[i]);
    }
}
