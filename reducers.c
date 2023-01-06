//
// Created by flassabe on 26/10/22.
//

#include "reducers.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "global_defs.h"
#include "utility.h"

#include <sys/stat.h>
#include <stdlib.h>

#include<libgen.h>
#include "configuration.h"


/*!
 * @brief add_source_to_list adds an e-mail to the sources list. If the e-mail already exists, do not add it.
 * @param list the list to update
 * @param source_email the e-mail to add as a string
 * @return a pointer to the updated beginning of the list
 */
sender_t *add_source_to_list(sender_t *list, char *source_email) {
    //On stocke l'email dans la variable "new"
    sender_t* new = (sender_t*)malloc(sizeof(sender_t));
    strcpy(new->recipient_address,source_email);
    new->head=NULL;
    new->tail=NULL;
    new->next=NULL;
    new->prev=NULL;

    if(list==NULL){
        list=new;
    }else{
        //Si on pas trouve l'email dans la liste, on l'ajoute a la fin
        if (find_source_in_list(list, source_email) == NULL) {
            sender_t* p=list;
            while(p->next!=NULL){
                p=p->next;
            }
            p->next=new;
            new->prev=p;
        }
    }

    return list;
}

/*!
 * @brief clear_sources_list clears the list of e-mail sources (therefore clearing the recipients of each source)
 * @param list a pointer to the list to clear
 */
void clear_sources_list(sender_t *list) {
    //On cherche effacer les tete de la liste de sources
    while (list->next!=0){//on parcours chaque source
        //on cherche effacer la tete de la liste des destinataires

        //on remet a 0 les chaines de characteres ie.adresse
        memset(list->recipient_address,'\0',STR_MAX_LEN);

        while(list->head->next!=0){  //on parcours la liste des destinataires

            //on remet a 0 les chaines de characteres ie.adresse
            memset(list->head->recipient_address,'\0',STR_MAX_LEN);
            list->head->occurrences=0;
            if(list->head->prev!=NULL){ //on libere la memoire de l'adresse du mail precedent
                free(list->head->prev);
            }
            list->head=list->head->next;
        }
        free(list->prev);//on libere la memoire de l'adresse du mail precedent au dernier mail recipient

        if(list->prev!=NULL){
            free(list->prev); //on libere la memoire de l'adresse du mail precedent
        }
        list=list->next;

    }
    free(list->prev);//on libere la memoire de l'adresse du mail precedent au dernier mail source
}

/*!
 * @brief find_source_in_list looks for an e-mail address in the sources list and returns a pointer to it.
 * @param list the list to look into for the e-mail
 * @param source_email the e-mail as a string to look for
 * @return a pointer to the matching source, NULL if none exists
 */
sender_t *find_source_in_list(sender_t *list, char *source_email) {
    int test_trouve=0;
    if(list==NULL){
        return list;
    }else{
        sender_t* p=list;
        while(p->next!=NULL&&test_trouve==0){ //on parcours la liste
            //Si on le trouve on envoie test_trouve=1
            if(strncmp(p->recipient_address,source_email,strlen(source_email))==0){
                test_trouve=1;
                return p;
            }
            p=p->next;
        }
        if(strcmp(p->recipient_address,source_email)==0){ //Si on trouve le mail a la fin, test_trouve=1
            test_trouve=1;
            return p;
        }

        if(test_trouve!=1){//Sinon on envoie NULL
            return NULL;
        }

    }

}

/*!
 * @brief add_recipient_to_source adds or updates a recipient in the recipients list of a source. It looks for
 * the e-mail in the recipients list: if it is found, its occurrences is incremented, else a new recipient is created
 * with its occurrences = to 1.
 * @param source a pointer to the source to add/update the recipient to
 * @param recipient_email the recipient e-mail to add/update as a string
 */
void add_recipient_to_source(sender_t *source, char *recipient_email) {
    //printf("nouveau recip\n");
    int test_trouve=0;

    recipient_t *new=(recipient_t*)malloc(sizeof(recipient_t));
    strcpy(new->recipient_address,recipient_email);
    new->next=NULL;
    new->prev=NULL;

    //Si la liste n'a pas en encore de recipients
    if(source->head==NULL){ //On l'ajoute au debut;
        new->occurrences=1;
        source->head=new;
        source->tail=new;
    }else{
        //Sinon on cherche le mail
        recipient_t* current_recipient=source->head;
        while(current_recipient!=NULL&&test_trouve==0){
            if(strcmp(current_recipient->recipient_address,recipient_email)==0){
                current_recipient->occurrences=current_recipient->occurrences+1;
                test_trouve=1;
            }
            current_recipient=current_recipient->next;
        }

        //Si on pas trouve l'email dans la liste, on l'ajoute a la fin
        if(test_trouve==0){
            new->occurrences=1;
            new->prev=source->tail;
            source->tail->next=new;
            source->tail=new;
        }
    }
}




/*!
 * @brief files_list_reducer is the first reducer. It uses concatenates all temporary files from the first step into
 * a single file. Don't forget to sync filesystem before leaving the function.
 * @param data_source the data source directory (its directories have the same names as the temp files to concatenate)
 * @param temp_files the temporary files directory, where to read files to be concatenated
 * @param output_file path to the output file (default name is step1_output, but we'll keep it as a parameter).
 */

//  files_list_reducer(config.data_path, config.temporary_directory, temp_result_name);


void files_list_reducer(char *data_source, char *temp_files, char *output_file) {
    int stat_change_directory;
    char last_dir[STR_MAX_LEN];
    char string[STR_MAX_LEN];
    char **noms_utilisateurs;
    noms_utilisateurs=malloc(150*sizeof(char*));
    DIR *dir;
    int it=0;
    if(directory_exists(data_source)==true&&directory_exists(temp_files)==true){ //On teste l'existance des repertoires
        //On stocke les noms des utilisateurs du repertoire
        dir=opendir(data_source);
        struct dirent *current_entry;
        while ((current_entry = readdir(dir)) != NULL){
            struct stat info;
            stat(current_entry->d_name, &info);
            if (strcmp(current_entry->d_name, ".") && strcmp(current_entry->d_name, "..")) {
                noms_utilisateurs[it] = malloc(sizeof(char) * strlen(current_entry->d_name));
                strcpy(noms_utilisateurs[it],current_entry->d_name);
                it++;
            }
        }

        strcpy(last_dir, basename(output_file));
        stat_change_directory=chdir(temp_files);
        //On ouvre le fichier resultat
        FILE *file_output = fopen(last_dir, "a+");
        if(stat_change_directory==0){ //Change of direcctory succesful
            //On ouvre chaque fichier "utilisateur"
            for(int i=0;i<it;i++){
                FILE *f = fopen(noms_utilisateurs[i], "r");
                if(f!=NULL){
                    //On parcours ligne par ligne et on la copie dans le fichier resultar
                    while (fgets(string, STR_MAX_LEN, f)) {
                        fprintf( file_output, "%s",string );
                    }
                    fclose(f);
                }
            }
            fclose(file_output);
        }else{
            //printf("");
        }
        for(int i=0;i<it;i++){
            free(noms_utilisateurs[i]);
        }
        free(noms_utilisateurs);

    } //Sinon on envoie un message d'erreur
    else{
        printf("Repertoires inexistants\n");
    }
    sync_temporary_files(temp_files);
}


/*!
 * @brief files_reducer opens the second temporary output file (default step2_output) and collates all sender/recipient
 * information as defined in the project instructions. Stores data in a double level linked list (list of source e-mails
 * containing each a list of recipients with their occurrences).
 * @param temp_file path to temp output file
 * @param output_file final output file to be written by your function
 */
void files_reducer(char *temp_file, char *output_file) {
    sender_t *list_of_source_emails=NULL; //
    char string[STR_MAX_LEN];
    char sender[STR_MAX_LEN];
    memset(sender,'\0',STR_MAX_LEN);
    if(path_to_file_exists(temp_file)==true) {
        FILE *f = fopen(temp_file, "r");
        while (fgets(string, STR_MAX_LEN, f)) {
            //On extrait le sender et on l'joute a la liste
            get_word(string,sender);
            list_of_source_emails=add_source_to_list(list_of_source_emails,sender);
            //On coupe le string complet pour extraire les "recipients"
            strcpy(string,string+strlen(sender));
            char recipient[STR_MAX_LEN];
            //On ajoute des recipients tant que qu'il y en ait
            while(string[0]!='\n'){
                //On reinitialise un nouveau "recipient"
                memset(recipient,'\0',STR_MAX_LEN);
                //On extrait le nouveau recipient
                get_word(string,recipient);
                //Et on ajoute le recipient au sender correspondant
                add_recipient_to_source(find_source_in_list(list_of_source_emails,sender),recipient);
                strcpy(string,string+strlen(recipient)+1);
            }
            memset(sender,'\0',STR_MAX_LEN);
            memset(recipient,'\0',STR_MAX_LEN);
        }
    }

    affiche_liste(list_of_source_emails);
    // Maintenant on copie, nos resultats dans un fichier
    char *fichier_resultat=malloc(sizeof(char)*STR_MAX_LEN);
    char *parent_dir=malloc(sizeof(char)*STR_MAX_LEN);
    //On extrait le nom du fichier a creer
    strcpy(fichier_resultat, basename(output_file));
    puts(fichier_resultat);
    // On extrait le chemin du repertoires parent
    int lg_fichier_resultat=strlen(fichier_resultat);
    int lg_path=strlen(output_file);
    int lg_parentdir=lg_path-lg_fichier_resultat;
    strncpy(parent_dir, output_file,lg_parentdir-1);

    //On change de repertoire pour creer un fichier dans le repertoire parent qui lui correspond
    if(directory_exists(parent_dir)){
        int stat_change_directory=chdir(parent_dir);
        if(stat_change_directory==0){ //Change of direcctory succesful
            sender_t* p=list_of_source_emails;
            while (p!=NULL){ // On parcours la liste
                FILE *file_output = fopen(fichier_resultat, "a+");// on ouvre le fichier resultat
                fprintf(file_output,"%s ", p->recipient_address); //on copie la liste des sources avec destinataire
                recipient_t* current_recipient=p->head;
                while(current_recipient!=NULL){  // Tant qu'il aient des  recipients, on les imprime
                    fprintf(file_output,"(%d) %s ", current_recipient->occurrences,current_recipient->recipient_address);
                    current_recipient=current_recipient->next;
                }
                fprintf(file_output,"\n");
                p=p->next;
                fclose(file_output);// A chaque fois on ferme le fichier resultat
            }
        }
    }
    free(fichier_resultat);
    free(parent_dir);
}

void affiche_liste(sender_t* l){ //Fonction pas utilise mais algorithme utilise pour imprimer dans le fichier resultat
    if( l==NULL ){
        printf("Liste vide\n");
    }
    else{
        sender_t* p=l;
        while (p !=NULL){
            printf("%s ", p->recipient_address);
            recipient_t* current_recipient=p->head;
            while(current_recipient!=NULL){
                printf("(%d) %s ", current_recipient->occurrences,current_recipient->recipient_address);
                current_recipient=current_recipient->next;
            }
            printf("\n");
            p=p->next;
        }
        //printf (" %s ]\n",p->recipient_address);
    }


}