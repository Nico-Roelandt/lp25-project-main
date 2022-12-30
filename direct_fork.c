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
    // 1. Check parameters
    if (directory_exists(data_source) != true) {
        fprintf(stderr, "Data source n'est pas valide\n");
        return;
    }
    if (directory_exists(temp_files) != true) {
        fprintf(stderr, "Temp file n'est pas valide\n");
        return;
    }
    FILE *output_file = fopen(temp_files, "w");
    DIR *dir = opendir(data_source);
    struct dirent *entry;
    char *path = malloc(sizeof(data_source));
    // 2. Iterate over directories (ignore . and ..)
    int running_processes = 0;
    while ((entry = readdir(dir)) != NULL) {
        printf("Debug 2\n");
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construit le chemin complet de l'entrée
        snprintf(path, sizeof(data_source), "%s/%s", temp_files, entry->d_name);

        if (entry->d_type == DT_DIR) {
            if (running_processes >= nb_proc) {
                wait(NULL);
                running_processes--;
            }
            
            printf("Debug 3\n");
            // Crée un nouveau processus pour traiter le répertoire
            int pid = fork();
            if (pid == 0) { 
                // Processus fils : appelle récursivement la fonction pour traiter le répertoire
                direct_fork_directories(data_source, temp_files, nb_proc-1);
                exit(0); // Termine le processus fils
            } else {
                printf("Debug 4\n");
                running_processes++;
            }
        } else {  
            printf("Ajout de : %s", path);
            fprintf(output_file, "%s\n", path);
        }

    }

    // 4. Cleanup
    while(running_processes != 0){
        wait(NULL);
        running_processes--;
    }
    printf("Debug 1.8\n");
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
    FILE *input_file = fopen(data_source, "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return;
    }

    // Créez un tableau de chaînes de caractères pour stocker les adresses email
    char **emails = malloc(sizeof(char*));
    int num_emails = 0;

    // Parcourez chaque ligne du fichier d'entrée
    char line[1024];
    int running_processes = 0;
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
            return;
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
    FILE *output_file = fopen(temp_files, "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return;
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
}