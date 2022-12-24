//
// Created by flassabe on 26/10/22.
//

#include "analysis.h"

#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/file.h>

#include "utility.h"

/*!
 * @brief parse_dir parses a directory to find all files in it and its subdirs (recursive analysis of root directory)
 * All files must be output with their full path into the output file.
 * @param path the path to the object directory
 * @param output_file a pointer to an already opened file
 */
void parse_dir(char *path, FILE *output_file) {
    // 1. Check parameters
    if (path == NULL || output_file == NULL) {
        printf("Erreur : il manque des paramétres.\n");
        return;
    }
    DIR *dir = opendir(path);
    if (dir == NULL) {
        printf("Erreur : le chemin ne méne à rien");
        return;
    }


    // 2. Go through all entries: if file, write it to the output file; if a dir, call parse_dir on it

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
        }

        char entry_path[1024];
        // On utilise une chaine de caractére pour stocker le chemin vers le fichier
        strcpy(entry_path, path);
        strcat(entry_path, "/");
        strcat(entry_path, entry->d_name);

        if (stat(path, &sb) == 0) {
        fprintf(output_file, "%s\n", entry_path);
        } else{
            parse_dir(entry_path)
        }
    }

    // 3. Clear all allocated resources
    closedir(dir);
}

/*!
 * @brief clear_recipient_list clears all recipients in a recipients list
 * @param list the list to be cleared
 */
void clear_recipient_list(simple_recipient_t *list) {
    simple_recipient_t *current = list;
    while (current != NULL) {
        simple_recipient_t *next = current->next;
        current->next = NULL;
        current = next;
    }
}

/*!
 * @brief add_recipient_to_list adds a recipient to a recipients list (as a pointer to a recipient)
 * @param recipient_email the string containing the e-mail to add
 * @param list the list to add the e-mail to
 * @return a pointer to the new recipient (to update the list with)
 */
simple_recipient_t *add_recipient_to_list(char *recipient_email, simple_recipient_t *list) {
    simple_recipient_t *new_recipient = malloc(sizeof(simple_recipient_t));
    new_recipient->email = recipient_email;
    new_recipient->next = list;
    return new_recipient;
}


/*!
 * @brief extract_emails extracts all the e-mails from a buffer and put the e-mails into a recipients list
 * @param buffer the buffer containing one or more e-mails
 * @param list the resulting list
 * @return the updated list
 */
simple_recipient_t *extract_emails(char *buffer, simple_recipient_t *list) {
        // 1. Check parameters
        if (buffer == NULL || list == NULL) {
            return NULL;
        }
        // 2. Go through buffer and extract e-mails
    char *email_start = buffer;
    while (1) {
        // On cherche l'élément qui caractérise l'adresse mail
        email_start = strstr(email_start, "@");
        if (email_start == NULL) {
        break;
        }

        // a partir du début de l'adresse mail on va jusqu'au prochain espace
        char *email_end = strstr(email_start, " ");
        if (email_end == NULL) {
        // This is the last email in the buffer, set end to the end of the buffer
        email_end = buffer + strlen(buffer);
        }

        // Ici on prend les positions de début et de fin en ce qui est entre les deux est l'adresse mail
        int email_length = email_end - email_start;
        char *email = malloc(email_length + 1);
        memcpy(email, email_start, email_length);
        email[email_length] = '\0';

        // 3. Add each e-mail to list
        list = add_recipient_to_list(email, list);

        email_start = email_end + 1;
    }

        // 4. Return list
    return list;
}


/*!
* @brief extract_e_mail extracts an e-mail from a buffer
* @param buffer the buffer containing the e-mail
* @param destination the buffer into which the e-mail is copied
*/
void extract_e_mail(char buffer[], char destination[]) {
    if (buffer == NULL || destination == NULL) {
        return;
    }

    // Find the start of the email content
    char *content_start = strstr(buffer, "Content:");
    if (content_start == NULL) {
        destination[0] = '\0';
        return;
    }
    // On se déplace du début de "Content:" jusqu'a sa fin
    content_start += 8;

    // La fin de l'email, j'ai mis deux retour à la ligne n'ayant pas trouver d'exemple de fichier
    char *content_end = strstr(content_start, "\n\n");
    if (content_end == NULL) {
        destination[0] = '\0';
        return;
    }

    int content_length = content_end - content_start;
    memcpy(destination, content_start, content_length);
    destination[content_length] = '\0';

}

// Used to track status in e-mail (for multi lines To, Cc, and Bcc fields)
typedef enum {IN_DEST_FIELD, OUT_OF_DEST_FIELD} read_status_t;

/*!
 * @brief parse_file parses mail file at filepath location and writes the result to
 * file whose location is on path output
 * @param filepath name of the e-mail file to analyze
 * @param output path to output file
 * Uses previous utility functions: extract_email, extract_emails, add_recipient_to_list,
 * and clear_recipient_list
 */
void parse_file(char *filepath, char *output) {



    // 1. Check parameters
    if (filepath == NULL || output == NULL) {
        return;
    }

    FILE *email_file = fopen(filepath, "r");
    if (email_file == NULL) {
        printf("Erreur lors de l'ouverture d'un fichier");
        return;
    }

    // On prend le contenue du fichier pour le mettre dans un buffer
    fseek(email_file, 0, SEEK_END);
    long email_size = ftell(email_file);
    fseek(email_file, 0, SEEK_SET);
    char *email_buffer = malloc(email_size + 1);
    fread(email_buffer, 1, email_size, email_file);
    email_buffer[email_size] = '\0';

    fclose(email_file);

    // 2. Go through e-mail and extract From: address into a buffer
    char from_buffer[256];
    extract_e_mail(email_buffer, from_buffer);

    // 3. Extract recipients (To, Cc, Bcc fields) and put it to a recipients list.
    simple_recipient_t *recipients = NULL;
    recipients = extract_emails(email_buffer, recipients);

    FILE *output_file = fopen(output, "w");
    if (output_file == NULL) {
        printf("Erreur : fichier de sortie n'est pas valide");
        free(email_buffer);
        clear_recipient_list(recipients);
        return;
    }

    // 4. Lock output file
    flock(fileno(output_file), LOCK_EX);


    // 5. Write to output file according to project instructions
    simple_recipient_t *current = recipients;
    while (current != NULL) {
        // On fait la liste de tout les emails
        fprintf(output_file, "%s ", current->email);
        current = current->next;
    }

    // 6. Unlock file
    flock(fileno(output_file), LOCK_UN);

    // 7. Close file
    fclose(output_file);

    // 8. Clear all allocated resources
    free(email_buffer);
    clear_recipient_list(recipients);





}

/*!
 * @brief process_directory goes recursively into directory pointed by its task parameter object_directory
 * and lists all of its files (with complete path) into the file defined by task parameter temporary_directory/name of
 * object directory
 * @param task the task to execute: it is a directory_task_t that shall be cast from task pointer
 * Use parse_dir.
 */
void process_directory(task_t *task) {
    // 1. Check parameters
    if (task == NULL) {
        printf("Erreur : la tache est vide");
        return;
    }
    directory_task_t *directory_task = (directory_task_t *)task->argument;
    if (directory_task->object_directory == NULL || directory_task->temporary_directory == NULL) {
        printf("Erreur : la tache est n'as pas tout les éléments nécessaire");
        return;
    }

    char *output_path = malloc(strlen(directory_task->temporary_directory) + strlen(directory_task->object_directory) + 2);
    sprintf(output_path, "%s/%s", directory_task->temporary_directory, directory_task->object_directory);

    FILE *output_file = fopen(output_path, "w");
    if (output_file == NULL) {
        printf("Erreur : le fichier de sortie n'as pas pu étre ouvert");
        free(output_path);
        return;
    }
    flock(fileno(output_file), LOCK_EX);

    // 2. Go through dir tree and find all regular files
    // 3. Write all file names into output file
    parse_dir(directory_task->object_directory, output_file);


    // 4. Clear all allocated resources
    flock(fileno(output_file), LOCK_UN);
    fclose(output_file);
    free(output_path);

}

/*!
 * @brief process_file processes one e-mail file.
 * @param task a file_task_t as a pointer to a task (you shall cast it to the proper type)
 * Uses parse_file
 */
void process_file(task_t *task) {
    // 1. Check parameters
    if (task == NULL) {
        printf("Erreur : la tache est vide");
        return;
    }
    file_task_t *file_task = (file_task_t *)task;
    if (file_task->mail_directory == NULL || file_task->temporary_directory == NULL || file_task->file_name == NULL) {
        printf("Erreur : la tache est n'as pas tous les éléments nécessaire");
        return;
    }
    



    //2.1 Build the full path to the input file
    char *input_path = malloc(strlen(file_task->mail_directory) + strlen(file_task->file_name) + 2);
    sprintf(input_path, "%s/%s", file_task->mail_directory, file_task->file_name);

    //2.2 Build the full path to the output file
    char *output_path = malloc(strlen(file_task->temporary_directory) + strlen(file_task->file_name) + 2);
    sprintf(output_path, "%s/%s", file_task->temporary_directory, file_task->file_name);

    // 3. Call parse_file
    parse_file(input_path, output_path);

    // 4. Free allocated resources
    free(input_path);
    free(output_path);


}
