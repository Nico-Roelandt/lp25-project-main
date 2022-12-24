//
// Created by flassabe on 26/10/22.
//

#include "utility.h"

#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include<libgen.h>
#include <sys/stat.h>

#include "global_defs.h"

/*!
 * @brief cat_path concatenates two file system paths into a result. It adds the separation /  if required.
 * @param prefix first part of the complete path
 * @param suffix second part of the complete path
 * @param full_path resulting path
 * @return pointer to full_path if operation succeeded, NULL else
 */
char *concat_path(char *prefix, char *suffix, char *full_path) {
    if(prefix!=NULL&&suffix!=NULL){
        strcpy(full_path,prefix);
        strcat(full_path,suffix);
    }
    else{
        strcpy(full_path,NULL);
    }


    return full_path;
}

/*!
 * @brief directory_exists tests if directory located at path exists
 * @param path the path whose existence to test
 * @return true if directory exists, false else
 */
bool directory_exists(char *path) {
    if (opendir(path)==NULL) // Si une
    {
        //printf("%s ",strerror(errno))
        return false;
    }
    else{
        //printf("exte %s",path);
        return true;
    }
    return false;
}

/*!
 * @brief path_to_file_exists tests if a path leading to a file exists. It separates the path to the file from the
 * file name itself. For instance, path_to_file_exists("/var/log/Xorg.0.log") will test if /var/log exists and is a
 * directory.
 * @param path the path to the file
 * @return true if path to file exists, false else
 */
bool path_to_file_exists(char *path) {
    FILE *f=fopen(path, "a+");
    if(f==NULL){
        //printf("%s ",strerror(errno));
        return false;
    }else{

        //printf("chemin trouve");
        return true;

    }
    return false;
}

/*!
 * @brief sync_temporary_files waits for filesystem syncing for a path
 * @param temp_dir the path to the directory to wait for
 * Use fsync and dirfd
 */
void sync_temporary_files(char *temp_dir) {


}

/*!
 * @brief next_dir returns the next directory entry that is not . or ..
 * @param entry a pointer to the current struct dirent in caller
 * @param dir a pointer to the already opened directory
 * @return a pointer to the next not . or .. directory, NULL if none remain
 */
struct dirent *next_dir(struct dirent *entry, DIR *dir) { //PAS ENCORE FINIE
    int stat_change_directory=-1;
    int test_first_dir=0;
    //char path[STR_MAX_LEN];
    char *path=(char*)malloc(STR_MAX_LEN*sizeof(char));

    if (!dir) {
        printf("is not a valid path\n");

    } else {
        while ((entry = readdir(dir)) != NULL&&test_first_dir==0){

            struct stat info;
            stat(entry->d_name, &info);
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")&&S_ISDIR(info.st_mode)) {
                printf("%s is a directory\n", entry->d_name);
                //realpath(entry->d_name, path);
                puts(path);
                stat_change_directory=chdir(path);
                if(stat_change_directory==0){
                    printf("change of directory successful");
                }else{
                    printf("change of directory unsuccessful");
                }
                test_first_dir=1;
            }
        }
        closedir(dir);
        return entry;
    }
    return NULL;
}

/*!
 * @brief Creates a new repertory with th
 * @param path: chemin absolue contenant repertoire a creer et repertoire parent
 * @return 1 si le repertoire a ete cree et NULL sinon
 */
int create_directory(char*path){
    int stat_change_directory=-1;
    int stat_create_directory=-1;
    int lg_lastdir,lg_path,lg_parentdir;

    char last_dir[STR_MAX_LEN];
    char parent_dir[STR_MAX_LEN];

    strcpy(last_dir, basename(path));   // On extrait le nom du repertoire a creer
    lg_lastdir=strlen(last_dir);
    lg_path=strlen(path);
    lg_parentdir=lg_path-lg_lastdir;
    strncpy(parent_dir, path,lg_parentdir-1); // On extrait le chemin des repertoires parents


    if(directory_exists(parent_dir)){
        stat_change_directory=chdir(parent_dir);
        if(stat_change_directory==0){ //Change of direcctory succesful
            stat_create_directory=mkdir(last_dir, 0777);
            if ( stat_create_directory== 0) { //Directory created successfully
                //printf("Directory created successfully\n");
            } else {
                //perror("mkdir");
                return 0;
            }
        }else{
            return 0;
        }

    }else{
        return 0;
    }


    return 1;
}








