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


    DIR *dir = opendir(data_source);
    struct dirent *entry;
    char *temp_path = malloc(sizeof(temp_files)+sizeof(entry->d_name));
    char *source_path = malloc(sizeof(temp_files)+sizeof(entry->d_name));
    // 2. Iterate over directories (ignore . and ..)
    int running_processes = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        int size = strlen(entry->d_name);
        concat_path(data_source, entry->d_name, source_path);

        if (entry->d_type == DT_DIR && entry->d_name[size - 2] == '-') {

        



            if (running_processes >= nb_proc) {
                wait(NULL);
                running_processes--;
            }
            
            // Crée un nouveau processus pour traiter le répertoire
            int pid = fork();
            if (pid == 0) { 
                // Processus fils : appelle récursivement la fonction pour traiter le répertoire
                direct_fork_directories(source_path, concat_path(temp_files, entry->d_name, temp_path), nb_proc-1);
                exit(0); // Termine le processus fils
            } else {
                running_processes++;
            }
        } else { 
            if(entry->d_type == DT_DIR){
                int pid = fork();
                if (pid == 0) { 
                    // Processus fils : appelle récursivement la fonction pour traiter le répertoire
                    direct_fork_directories(source_path, temp_files, nb_proc-1);
                    exit(0); // Termine le processus fils
                } else {
                    running_processes++;
                }
            } else {
                FILE *output_file = fopen(temp_files, "a");
                fprintf(output_file, "%s\n", source_path);
                fclose(output_file);
            }
        }

    }

    // 4. Cleanup
    while(running_processes != 0){
        wait(NULL);
        running_processes--;
    }
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
    char* temp[12];
    char *temp_path = malloc(sizeof(temp_files) + sizeof(temp));
    char *source_path = malloc(sizeof(temp_files) + sizeof(temp)); // Ici on agrandit la taille du malloc pour pouvoir ajouter "step1_output"
    
    concat_path(temp_files,"step1_output",source_path);
    //Ouverture de step1_output
    FILE *input_file = fopen(source_path, "r");
    if (input_file == NULL) {
        printf("Error opening input file 1");
        return;
    }

    concat_path(temp_files,"step2_output",temp_path);
    // Ouvrez le fichier de sortie en écriture
    FILE *output_file = fopen(temp_path, "w");
    if (output_file == NULL) {
        perror("Error opening output file 2\n");
        return;
    }

    // Parcourez chaque ligne du fichier d'entrée
    char line[5000];
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
            //    printf("Error opening file :%s\n",line);
                exit(0);
            }
            // Parcourez chaque ligne du fichier
            char email_line[1024];
            while (fgets(email_line, sizeof(email_line), file)) {
                // Vérifiez si la ligne contient un des champs From, To, Cc ou Bcc
                if (strstr(email_line, "From:") || strstr(email_line, "To:") || strstr(email_line, "Cc:") || strstr(email_line, "Bcc:")) {
                    char *email_start = strstr(email_line, " ") + 1;
                    int stop = 0;
                    while (stop != 1) {
                        // a partir du début de l'adresse mail on va jusqu'au .com
                        char *email_end = strstr(email_start, ",");
                        if (email_end == NULL) {
                            // This is the last email in the buffer, set end to the end of the buffer
                            email_end = strstr(email_start, ".com");
                            email_end = email_end + 4;
                            stop = 1;
                        }

                        // Ici on prend les positions de début et de fin en ce qui est entre les deux est l'adresse mail
                        int email_length = email_end - email_start;
                        
                        if(strstr(email_start, "@")){
                            char* email = malloc(sizeof(char*) * email_length);
                            if(!strstr(email_start, " ") && !strstr(email_start, "/")){
                                memcpy(email, email_start, email_length * sizeof(char));
                                fprintf(output_file, "%s ", email);
                                //Écrivez chaque adresse email dans le fichier de sortie
                            }
                        }


                        email_start = email_end + 1;
                    }

                }
            }
            fprintf(output_file, "\n");
            // Fermez le fichier
            fclose(file);

            // Quittez le processus fils
            exit(0);
        }
    }
    printf("FIN");
    // Fermez le fichier d'entrée
    fclose(input_file);

    // Attendre la fin de chaque processus fils
    while(running_processes != 0){
        wait(NULL);
        running_processes--;
    }

    // Fermez le fichier de sortie
    fclose(output_file);
    return;
}