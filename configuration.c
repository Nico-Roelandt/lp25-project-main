//
// Created by flassabe on 14/10/22.
//

#include "configuration.h"

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "utility.h"

/*!
 * @brief make_configuration makes the configuration from the program parameters. CLI parameters are applied after
 * file parameters. You shall keep two configuration sets: one with the default values updated by file reading (if
 * configuration file is used), a second one with CLI parameters, to overwrite the first one with its values if any.
 * @param base_configuration a pointer to the base configuration to be updated
 * @param argv the main argv
 * @param argc the main argc
 * @return the pointer to the updated base configuration
 */
configuration_t *make_configuration(configuration_t *base_configuration, char *argv[], int argc) {

    int opt = 0;
    while ((opt = getopt(argc, argv, "d:o:t:v:n:f:")) != -1) {
        switch (opt) {
            case 'd': //Parametrage data path
                if (optarg) {
                    if(directory_exists(optarg)==true){
                        strcpy(base_configuration->data_path,optarg);
                    }else{
                        base_configuration=NULL;
                    }
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    base_configuration=NULL;
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    if(directory_exists(argv[optind])==true){
                        strcpy(base_configuration->data_path,argv[optind]);
                    }else{
                        base_configuration=NULL;
                    }
                    ++optind;
                    break;
                }
                break;
            case 'o': //Parametrage output file
                if (optarg) {
                    if( path_to_file_exists(optarg)==true){
                        strcpy(base_configuration->output_file,optarg);
                    }else{
                        FILE *f=fopen(optarg, "a+"); // Si le chemin n'existe pas on le cree
                        fclose(f);
                        strcpy(base_configuration->output_file,optarg);

                    }
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    if( path_to_file_exists(optarg)==true){
                        strcpy(base_configuration->output_file,argv[optind]);
                    }else{
                        base_configuration=NULL;
                    }
                    ++optind;
                    break;
                }
                break;
            case 't': //Parametrage temporary directory
                if (optarg) {
                    if(directory_exists(optarg)==true){
                        strcpy(base_configuration->temporary_directory,optarg);
                    }else{ //sinon on cree un repertoire
                        int etat=create_directory(optarg);
                        if(etat!=0){
                            base_configuration=NULL;
                        }
                    }
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    printf("Option 'b' with optional argument=%s\n", argv[optind]);
                    if(directory_exists(argv[optind])==true){
                        strcpy(base_configuration->temporary_directory,argv[optind]);
                    }else{ //sinon on cree un repertoire
                        int etat=create_directory(argv[optind]);
                        if(etat!=0){
                            base_configuration=NULL;
                        }
                    }
                    ++optind;
                    break;
                }
                break;

            case 'v': //Parametrage verbose
                if (optarg) {
                    if(strncmp(optarg,"no",strlen("no"))==0){
                        base_configuration->is_verbose=false;
                    }else if(strncmp(optarg,"yes",strlen("yes"))==0){
                        base_configuration->is_verbose=true;
                    }else{
                        base_configuration=NULL;
                    }
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    printf("Option 'b' with no argument\n");
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    if(strncmp(argv[optind],"no",strlen("no"))==0){
                        base_configuration->is_verbose=false;
                    }else if(strncmp(argv[optind],"yes",strlen("yes"))==0){
                        base_configuration->is_verbose=true;
                    }else{
                        base_configuration=NULL;
                    }

                    ++optind;
                    break;
                }
                break;

            case 'n': //Parametrage cpu core
                if (optarg) {
                    base_configuration->cpu_core_multiplier=atoi(optarg);
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    base_configuration->cpu_core_multiplier=atoi(optarg);
                    ++optind;
                    break;
                }
            case 'f': //Parametrage  avec lecture de fichier
                if (optarg) {
                    if(path_to_file_exists(optarg)==true){
                        base_configuration=read_cfg_file(base_configuration,optarg);
                    }else{
                        base_configuration=NULL;
                    }
                    break;
                }
                if (optind == argc || (optind < argc && argv[optind][0] == '-')) {
                    base_configuration=NULL;
                    break;
                }
                if (optind < argc && argv[optind][0] != '-') {
                    if(path_to_file_exists(optarg)==true){
                        base_configuration=read_cfg_file(base_configuration,argv[optind]);

                    }else{
                        base_configuration=NULL;
                    }
                    ++optind;
                    break;
                }
                break;
            default:
                base_configuration=NULL;
                printf("Wrong option or missing argument for option\n");
        }
    }
    if (optind < argc) {
        printf("Remaining program arguments:");
        for (int i=optind; i<argc; ++i) {
            printf(" %s", argv[i]);
        }
        printf("\n");
    }

    return base_configuration;
}

/*!
 * @brief skip_spaces advances a string pointer to the first non-space character
 * @param str the pointer to advance in a string
 * @return a pointer to the first non-space character in str
 */
char *skip_spaces(char *str) {
    int i=0;
    while(str[i]==' '){
        i++;
    }
    str=&str[i];

    return str;
}

/*!
 * @brief check_equal looks for an optional sequence of spaces, an equal '=' sign, then a second optional sequence
 * of spaces
 * @param str the string to analyze
 * @return a pointer to the first non-space character after the =, NULL if no equal was found
 */
char *check_equal(char *str) {
    int i=0;


    if(str[0]==' '){ // Pointer to the first non espace
        str=skip_spaces(str);
    }

    //if(skip_spaces)
    while(str[i]!='='&& str[i]!='\0'){ //looks for an equal '=' sign
        i++;
    }

    if(strlen(str)==i+1){  //no equal was found
        str=NULL;

    }else{
        str=&str[i]; // Pointer to the first '=' if an equal was found
        i=0;
        if(str[i+1]==' '){ //looks for an optional sequence of spaces after the equal
            str=&str[i+1];// Pointer to the first ' ' if found after an equal
            str=skip_spaces(str); // Pointer to the first char autre que " "

        }else{
            str=&str[i+1];
        }


    }



    return str;
}

/*!
 * @brief get_word extracts a word (a sequence of non-space characters) from the source
 * @param source the source string, where to find the word
 * @param target the target string, where to copy the word
 * @return a pointer to the character after the end of the extracted word
 */
char *get_word(char *source, char *target) {
    int i=0;

    if(source[0]==' '){ //looks for an optional sequence of spaces at the beginning
        source=skip_spaces(source);
    }
    while(source[i]!=' '&&source[i]!='\0'){ //
        i++;
    }

    if(source[i-1]=='\n'){  // si le derniere caractere est un saut de page
        strncpy(target,source,i-1);

    }else{
        strncpy(target,source,i);
    }



    return source;
}

/*!
 * @brief read_cfg_file reads a configuration file (with key = value lines) and extracts all key/values for
 * configuring the program (data_path, output_file, temporary_directory, is_verbose, cpu_core_multiplier)
 * @param base_configuration a pointer to the configuration to update and return
 * @param path_to_cfg_file the path to the configuration file
 * @return a pointer to the base configuration after update, NULL is reading failed.
 */
configuration_t *read_cfg_file(configuration_t *base_configuration, char *path_to_cfg_file) {
    int test_next_ligne=0;
    char string[STR_MAX_LEN];
    char *source;
    char extract_string[STR_MAX_LEN];
    if(path_to_file_exists(path_to_cfg_file)==true) { //On teste si le chemin au fichier de configuration existe

        FILE *f = fopen(path_to_cfg_file, "r");
        while (fgets(string, STR_MAX_LEN, f)) {


            if(test_next_ligne==1){  //Saut de ligne pour le parametrage de temporary file
                test_next_ligne=0;
                get_word(string,extract_string);
                if(directory_exists(extract_string)==true){
                    strcpy(base_configuration->temporary_directory,extract_string);
                }
                else{ //sinon on cree un repertoire
                    int etat=create_directory(extract_string);
                    if(etat!=0){
                        base_configuration=NULL;
                    }
                }
            }


            if(strncmp(string,"is_verbose",strlen("is_verbose"))==0){   //Parametrage verbose

                if(check_equal(string)!=0){

                    if(strncmp(check_equal(string),"no",strlen("no"))==0){
                        base_configuration->is_verbose=false;
                    }else if(strncmp(check_equal(string),"yes",strlen("yes"))==0){
                        base_configuration->is_verbose=true;
                    }else{
                        base_configuration=NULL;
                    }

                }
            }

            if(strncmp(string,"data_path",strlen("data_path"))==0){ //Parametrage data path
                source=check_equal(string);
                if(source!=0){
                    get_word(source,extract_string);
                    if(directory_exists(extract_string)==true){
                        strcpy(base_configuration->data_path,extract_string);
                    }else{
                        base_configuration=NULL;
                    }

                }else{
                    base_configuration=NULL;
                }
            }
            if(strncmp(string,"temporary_directory",strlen("temporary_directory"))==0){ //Parametrage temporary directory
                source=check_equal(string);
                if(source!=0){
                    if(strcmp(source,"\n")==0){
                        test_next_ligne=1;
                    }else{
                        get_word(source,extract_string);
                        if(directory_exists(extract_string)==true){
                            strcpy(base_configuration->temporary_directory,extract_string);
                        }
                        else{ //sinon on cree un repertoire
                            int etat=create_directory(extract_string);
                            if(etat!=0){
                                base_configuration=NULL;
                            }
                        }

                    }

                }else{
                    base_configuration=NULL;
                }
            }
            if(strncmp(string,"output_file",strlen("output_file"))==0){ //Parametrage output file
                source=check_equal(string);
                if(source!=0){
                    get_word(source,extract_string);
                    if( path_to_file_exists(extract_string)==true){
                        strcpy(base_configuration->output_file,extract_string);
                    }else{
                        FILE *f=fopen(extract_string, "a+"); // Si le chemin n'existe pas on le cree
                        fclose(f);
                        strcpy(base_configuration->output_file,extract_string);
                    }

                }else{
                    base_configuration=NULL;
                }
            }
            if(strncmp(string,"cpu_core_multiplier",strlen("cpu_core_multiplier"))==0){
                source=check_equal(string);
                if(source!=0){
                    get_word(source,extract_string);
                    base_configuration->cpu_core_multiplier=atoi(extract_string);
                }else{
                    base_configuration=NULL;
                }

            }
            memset(extract_string,'\0',STR_MAX_LEN);
            memset(string,'\0',STR_MAX_LEN);
        }
    }
    /**
    else{
        base_configuration=NULL;
    }
*/

    return base_configuration;
}

/*!
 * @brief display_configuration displays the content of a configuration
 * @param configuration a pointer to the configuration to print
 */
void display_configuration(configuration_t *configuration) {
    printf("Current configuration:\n");
    printf("\tData source: %s\n", configuration->data_path);
    printf("\tTemporary directory: %s\n", configuration->temporary_directory);
    printf("\tOutput file: %s\n", configuration->output_file);
    printf("\tVerbose mode is %s\n", configuration->is_verbose?"on":"off");
    printf("\tCPU multiplier is %d\n", configuration->cpu_core_multiplier);
    printf("\tProcess count is %d\n", configuration->process_count);
    printf("End configuration\n");
}

/*!
 * @brief is_configuration_valid tests a configuration to check if it is executable (i.e. data directory and temporary
 * directory both exist, and path to output file exists @see directory_exists and path_to_file_exists in utility.c)
 * @param configuration the configuration to be tested
 * @return true if configuration is valid, false else
 */
bool is_configuration_valid(configuration_t *configuration) {
    if(configuration->cpu_core_multiplier!=0&&
       directory_exists(configuration->data_path)==true&&
       path_to_file_exists(configuration->output_file)==true&&(
               configuration->is_verbose==true||configuration->is_verbose==false)&&
       directory_exists(configuration->temporary_directory)==true)
    {
        return true;
    }else{
        return false;
    }
}



