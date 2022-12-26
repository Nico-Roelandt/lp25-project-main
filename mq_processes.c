//
// Created by flassabe on 10/11/22.
//

#include "mq_processes.h"

#include <sys/msg.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <bits/types/sig_atomic_t.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdbool.h>

#include "utility.h"
#include "analysis.h"

// Structure for tasks received from the parent process
struct task {
  void (*callback)(void *arg); // Pointer to callback function
  void *arg; // Argument to pass to callback function
};
// Custom task argument structure
struct task_arg {
  char *data_source; // Data source directory
  char *temp_files; // Temporary files directory
  char *target_file; // Target file
};

// Structure for messages sent between parent and child processes
struct message {
  long type; // Message type (used for message queue communication)
  struct task task; // Task received from parent process
};
// Custom configuration structure (assumed to contain a processes_count field)
struct configuration {
  int processes_count;
  // Other fields as needed...
};

// Type alias for the configuration structure
typedef struct configuration configuration_t;

/*!
 * @brief make_message_queue creates the message queue used for communications between parent and worker processes
 * @return the file descriptor of the message queue
 */
int make_message_queue() {
     key_t key;
  int message_queue_id;

  // Generate a unique key for the message queue
  key = ftok("/tmp", 'M');
  if (key == -1) {
    perror("Error generating key for message queue");
    return -1;
  }

  // Create the message queue
  message_queue_id = msgget(key, 0666 | IPC_CREAT);
  if (message_queue_id == -1) {
    perror("Error creating message queue");
    return -1;
  }

  return message_queue_id;
}

/*!
 * @brief close_message_queue closes a message queue
 * @param mq the descriptor of the MQ to close
 */
void close_message_queue(int mq) { 
     // Remove the message queue
  if (msgctl(mq, IPC_RMID, NULL) == -1) {
    perror("Error removing message queue");
  }
}

/*!
 * @brief child_process is the function handling code for a child
 * @param mq message queue descriptor used to communicate with the parent
 */
void child_process(int mq) {
     struct message message;
  bool running = true; // Flag to control the child process's loop

  while (running) {
    // Receive a task from the parent process
    if (msgrcv(mq, &message, sizeof(struct task), 0, 0) == -1) {
      perror("Error receiving message from message queue");
      break;
    }

    // Check if the task's callback function is NULL
    if (message.task.callback == NULL) {
      running = false; // Leave the loop if the callback is NULL
    } else {
      // Execute the task's callback function with the given argument
      message.task.callback(message.task.arg);

      // Notify the parent process that the task has been completed
      message.type = 1; // Set the message type to 1 for the parent process
      if (msgsnd(mq, &message, sizeof(struct task), 0) == -1) {
        perror("Error sending message to message queue");
        break;
      }
    }
  }

  // Clean up the message queue
  close_message_queue(mq);
}





/*!
 * @brief mq_make_processes makes a processes pool used for tasks execution
 * @param config a pointer to the program configuration (with all parameters, inc. processes count)
 * @param mq the identifier of the message queue used to communicate between parent and children (workers)
 * @return a malloc'ed array with all children PIDs
 */
pid_t *mq_make_processes(configuration_t *config, int mq) {
     pid_t *children_pids;
  int i;

  // Allocate memory for the array of children PIDs
  children_pids = malloc(config->processes_count * sizeof(pid_t));
  if (children_pids == NULL) {
    perror("Error allocating memory for children PIDs");
    return NULL;
  }

  // Fork the child processes
  for (i = 0; i < config->processes_count; i++) {
    pid_t child_pid = fork();
    if (child_pid == -1) {
      perror("Error creating child process");
      free(children_pids);
      return NULL;
    }
    if (child_pid == 0) {
      // Child process: start handling tasks
      child_process(mq);
      exit(EXIT_SUCCESS);
    } else {
      // Parent process: store the child's PID in the array
      children_pids[i] = child_pid;
    }
  }

  return children_pids;
}



/*!
 * @brief close_processes commands all workers to terminate
 * @param config a pointer to the configuration
 * @param mq the message queue to communicate with the workers
 * @param children the array of children's PIDs
 */
void close_processes(configuration_t *config, int mq, pid_t children[]) {
     struct message message;
  int i;

  // Send a command to each child process to terminate
  for (i = 0; i < config->processes_count; i++) {
    message.type = children[i]; // Set the message type to the child's PID
    message.task.callback = NULL; // Set the callback function to NULL to signal termination
    if (msgsnd(mq, &message, sizeof(struct task), 0) == -1) {
      perror("Error sending message to message queue");
      break;
    }
  }

  // Wait for all child processes to terminate
  for (i = 0; i < config->processes_count; i++) {
    waitpid(children[i], NULL, 0);
  }

  // Clean up the message queue
  close_message_queue(mq);
}



/*!
 * @brief send_task_to_mq sends a directory task to a worker through the message queue. Directory task's object is
 * data_source/target_dir, temp output file is temp_files/target_dir. Task is sent through MQ with topic equal to
 * the worker's PID
 * @param data_source the data source directory
 * @param temp_files the temporary files directory
 * @param target_dir the name of the target directory
 * @param mq the MQ descriptor
 * @param worker_pid the worker PID
 */
void send_task_to_mq(char data_source[], char temp_files[], char target_dir[], int mq, pid_t worker_pid) {
     struct message message;
  struct task_arg *task_arg;

  // Allocate memory for the task argument
  task_arg = malloc(sizeof(struct task_arg));
  if (task_arg == NULL) {
    perror("Error allocating memory for task argument");
    return;
  }

  // Set the fields of the task argument
  task_arg->data_source = strdup(data_source);
  task_arg->temp_files = strdup(temp_files);
  task_arg->target_dir = strdup(target_dir);

  // Set the fields of the message
  message.type = worker_pid; // Set the message type to the worker's PID
  message.task.callback = handle_task; // Set the callback function for the task
  message.task.arg = task_arg; // Set the task argument

  // Send the message to the message queue
  if (msgsnd(mq, &message, sizeof(struct task), 0) == -1) {
    perror("Error sending message to message queue");
  }
}



/*!
 * @brief send_file_task_to_mq sends a file task to a worker. It operates similarly to @see send_task_to_mq
 * @param data_source the data source directory
 * @param temp_files the temporary files directory
 * @param target_file the target filename
 * @param mq the MQ descriptor
 * @param worker_pid the worker's PID
 */ 
//Alors là y'a que target file que j'ai changé par rapport a la fonction d'avant ça me parait douteux mais bon il y'a marqué qu'elle marche pareil que la fonction d'avant (mais normalement ça fonctionne comme ça je pense)
void send_file_task_to_mq(char data_source[], char temp_files[], char target_file[], int mq, pid_t worker_pid) {
  struct message message;
  struct task_arg *task_arg;

  // Allocate memory for the task argument
  task_arg = malloc(sizeof(struct task_arg));
  if (task_arg == NULL) {
    perror("Error allocating memory for task argument");
    return;
  }

  // Set the fields of the task argument
  task_arg->data_source = strdup(data_source);
  task_arg->temp_files = strdup(temp_files);
  task_arg->target_file = strdup(target_file);

  // Set the fields of the message
  message.type = worker_pid; // Set the message type to the worker's PID
  message.task.callback = handle_task; // Set the callback function for the task
  message.task.arg = task_arg; // Set the task argument

  // Send the message to the message queue
  if (msgsnd(mq, &message, sizeof(struct task), 0) == -1) {
    perror("Error sending message to message queue");
  }

}

/*!
 * @brief mq_process_directory root function for parallelizing directory analysis over workers. Must keep track of the
 * tasks count to ensure every worker handles one and only one task. Relies on two steps: one to fill all workers with
 * a task each, then, waiting for a child to finish its task before sending a new one.
 * @param config a pointer to the configuration with all relevant path and values
 * @param mq the MQ descriptor
 * @param children the children's PIDs used as MQ topics number
 */
void mq_process_directory(configuration_t *config, int mq, pid_t children[]) {
    // 1. Check parameters
    // 2. Iterate over children and provide one directory to each
    // 3. Loop while there are directories to process, and while all workers are processing
    // 3 bis. For each worker finishing its task: send a new one if any task is left, keep track of running workers else
    // 4. Cleanup
}

/*!
 * @brief mq_process_files root function for parallelizing files analysis over workers. Operates as
 * @see mq_process_directory to limit tasks to one on each worker.
 * @param config a pointer to the configuration with all relevant path and values
 * @param mq the MQ descriptor
 * @param children the children's PIDs used as MQ topics number
 */
void mq_process_files(configuration_t *config, int mq, pid_t children[]) {
    // 1. Check parameters
    // 2. Iterate over children and provide one file to each
    // 3. Loop while there are files to process, and while all workers are processing
    // 3 bis. For each worker finishing its task: send a new one if any task is left, keep track of running workers else
    // 4. Cleanup
}
