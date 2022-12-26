//
// Created by flassabe on 27/10/22.
//

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
#include "winsock.h"

#include "analysis.h"
#include "utility.h"

/*!
 * @brief make_fifos creates FIFOs for processes to communicate with their parent
 * @param processes_count the number of FIFOs to create
 * @param file_format the filename format, e.g. fifo-out-%d, used to name the FIFOs
 */
void make_fifos(uint16_t processes_count, char *file_format) {

    for (int i = 0; i < processes_count; i++) {
        char fifo_name[256];
        sprintf(fifo_name, file_format, i);
        if (mkfifo(fifo_name, 0666) < 0) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
}

/*!
 * @brief erase_fifos erases FIFOs used for processes communications with the parent
 * @param processes_count the number of FIFOs to destroy
 * @param file_format the filename format, e.g. fifo-out-%d, used to name the FIFOs
 */
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

/*!
 * @brief make_processes creates processes and starts their code (waiting for commands)
 * @param processes_count the number of processes to create
 * @return a malloc'ed array with the PIDs of the created processes
 */
pid_t *make_processes(uint16_t processes_count) {
    // 1. Create PIDs array
    // 2. Loop over processes_count to fork
    // 2 bis. in fork child part, open reading and writing FIFOs, and start listening on reading FIFO
    // 3. Upon reception, apply task
    // 3 bis. If task has a NULL callback, terminate process (don't forget cleanup).
    return NULL;
}

/*!
 * @brief open_fifos opens FIFO from the parent's side
 * @param processes_count the number of FIFOs to open (must be created before)
 * @param file_format the name pattern of the FIFOs
 * @param flags the opening mode for the FIFOs
 * @return a malloc'ed array of opened FIFOs file descriptors
 */
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

/*!
 * @brief close_fifos closes FIFOs opened by the parent
 * @param processes_count the number of FIFOs to close
 * @param files the array of opened FIFOs as file descriptors
 */
void close_fifos(uint16_t processes_count, int *files) {
    for (int i = 0; i < processes_count; i++) {
        if (close(files[i]) < 0) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }
}

/*!
 * @brief shutdown_processes terminates all worker processes by sending a task with a NULL callback
 * @param processes_count the number of processes to terminate
 * @param fifos the array to the output FIFOs (used to command the processes) file descriptors
 */
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

/*!
 * @brief prepare_select prepares fd_set for select with all file descriptors to look at
 * @param fds the fd_set to initialize
 * @param filesdes the array of file descriptors
 * @param nb_proc the number of processes (elements in the array)
 * @return the maximum file descriptor value (as used in select)
 */
int prepare_select(fd_set *fds, const int *filesdes, uint16_t nb_proc) {
    FD_ZERO(fds); // Initialize the fd_set to have no file descriptors set

    // Set all file descriptors in the fd_set
    int max_fd = 0;
    for (int i = 0; i < nb_proc; i++) {
        FD_SET(filesdes[i], fds);
        max_fd = (max_fd > filesdes[i]) ? max_fd : filesdes[i];
    }

    return max_fd;
}

/*!
 * @brief send_task sends a directory task to a child process. Must send a directory command on object directory
 * data_source/dir_name, to write the result in temp_files/dir_name. Sends on FIFO with FD command_fd
 * @param data_source the data source with directories to analyze
 * @param temp_files the temporary output files directory
 * @param dir_name the current dir name to analyze
 * @param command_fd the child process command FIFO file descriptor
 */
void send_task(char *data_source, char *temp_files, char *dir_name, int command_fd) {

        char command[1024];
        snprintf(command, 1024, "dir %s/%s > %s/%s", data_source, dir_name, temp_files, dir_name);

        write(command_fd, command, strlen(command));

    }

/*!
 * @brief fifo_process_directory is the main function to distribute directory analysis to worker processes.
 * @param data_source the data source with the directories to analyze
 * @param temp_files the temporary files directory
 * @param notify_fifos the FIFOs on which to read for workers to notify end of tasks
 * @param command_fifos the FIFOs on which to send tasks to workers
 * @param nb_proc the maximum number of simultaneous tasks, = to number of workers
 * Uses @see send_task
 */
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


/*!
 * @brief fifo_process_files is the main function to distribute files analysis to worker processes.
 * @param data_source the data source with the files to analyze
 * @param temp_files the temporary files directory (step1_output is here)
 * @param notify_fifos the FIFOs on which to read for workers to notify end of tasks
 * @param command_fifos the FIFOs on which to send tasks to workers
 * @param nb_proc  the maximum number of simultaneous tasks, = to number of workers
 */
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
