//
// Created by flassabe on 26/10/22.
//

#include "direct_fork.h"

#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#include "analysis.h"
#include "utility.h"

/*!
 * @brief direct_fork_directories runs the directory analysis with direct calls to fork
 * @param data_source the data source directory with 150 directories to analyze (parallelize with fork)
 * @param temp_files the path to the temporary files directory
 * @param nb_proc the maximum number of simultaneous processes
 */
void direct_fork_directories(char *data_source, char *temp_files, uint16_t nb_proc) { // Le but est de faire la liste de tout les fichiers
    // 1. Check parameters OK
    if (data_source == NULL || temp_files == NULL || nb_proc == 0) {
        fprintf(stderr, "Invalid input parameters\n");
        return;
    }


    // 2. Iterate over directories (ignore . and ..)

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construit le chemin complet de l'entrée
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        if (entry->d_type == DT_DIR) {

            if (running_processes >= nb_proc) {
                wait(NULL);
                running_processes--;
            }
            running_processes++;

            // Crée un nouveau processus pour traiter le répertoire
            int pid = fork();
            if (pid == 0) {
                // Processus fils : appelle récursivement la fonction pour traiter le répertoire
                list_files(path, output_file);
                exit(0); // Termine le processus fils
            }
        } else { 
            fprintf(output_file, "%s\n", path);
        }
        while(running_processes != 0){
            wait(NULL);
            running_processes--;
        }
    }
    // 4. Cleanup
    closedir(dir);
}

/*!
 * @brief direct_fork_files runs the files analysis with direct calls to fork
 * @param data_source the data source containing the files
 * @param temp_files the temporary files to write the output (step2_output)
 * @param nb_proc the maximum number of simultaneous processes
 */
void direct_fork_files(char *data_source, char *temp_files, uint16_t nb_proc) {


// Ouvrez le fichier d'entrée en lecture
  FILE *input_file = fopen(argv[1], "r");
  if (input_file == NULL) {
    perror("Error opening input file");
    return 1;
  }

  // Créez un tableau de chaînes de caractères pour stocker les adresses email
  char **emails = malloc(sizeof(char*));
  int num_emails = 0;

  // Parcourez chaque ligne du fichier d'entrée
  char line[1024];
  while (fgets(line, sizeof(line), input_file)) {
    line[strcspn(line, "\n")] = '\0';


    if (running_processes >= nb_proc) {
        wait(NULL);
        running_processes--;
    }
    running_processes++;
    pid_t pid = fork();
    if (pid == 0) {
      // Ouvrez le fichier à analyser en lecture
      FILE *file = fopen(line, "r");
      if (file == NULL) {
        perror("Error opening file");
        return 1;
      }

      // Parcourez chaque ligne du fichier
      char email_line[1024];
      while (fgets(email_line, sizeof(email_line), file)) {
        // Vérifiez si la ligne contient un des champs From, To, Cc ou Bcc
        if (strstr(email_line, "From:") || strstr(email_line, "To:") ||
            strstr(email_line, "Cc:") || strstr(email_line, "Bcc:")) {
          // Utilisez strtok pour extraire les adresses email de la ligne
          char *token = strtok(email_line, " ,\n");
          while (token != NULL) {
            // Allouez de la mémoire pour stocker l'adresse email
            char *email = malloc(sizeof(char) * (strlen(token) + 1));
            strcpy(email, token);

            // Ajoutez l'adresse email au tableau
            emails = realloc(emails, sizeof(char*) * (num_emails + 1));
            emails[num_emails] = email;
            num_emails++;

            token = strtok(NULL, " ,\n");
          }
        }
      }


      // Fermez le fichier
      fclose(file);

      // Quittez le processus fils
      exit(0);
    }
  }

    // Fermez le fichier d'entrée
    fclose(input_file);

    // Attendre la fin de chaque processus fils
    while(running_processes != 0){
        wait(NULL);
        running_processes--;
    }

    // Ouvrez le fichier de sortie en écriture
    FILE *output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return 1;
    }

    // Écrivez chaque adresse email dans le fichier de sortie
    for (int i = 0; i < num_emails; i++) {
        fputs(emails[i], output_file);
        fputs("\n", output_file);
    }

    // Fermez le fichier de sortie
    fclose(output_file);

    // Libérez la mémoire utilisée par le tableau d'adresses email
    for (int i = 0; i < num_emails; i++) {
        free(emails[i]);
    }
    free(emails);

    return 0;
}











}

