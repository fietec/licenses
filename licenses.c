#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONP_IMPLEMENTATION
#include "conp.h"

#define return_defer(value) do{result = (value); goto defer;}while(0)
    
#define CONFIG_FILE_NAME "licenses.config"

static char temp_buffer[FILENAME_MAX];

static ConpEntries config;

char *get_config_file_path()
{
    snprintf(temp_buffer, FILENAME_MAX, "%s/licenses/"CONFIG_FILE_NAME, getenv("APPDATA"));
    return temp_buffer;
}

char *get_config_path()
{
    snprintf(temp_buffer, FILENAME_MAX, "%s/licenses", getenv("APPDATA"));
    return temp_buffer;
}

bool isdir(const char *path)
{
    struct stat attr;
    if (stat(path, &attr) == -1) return false;
    return S_ISDIR(attr.st_mode);
}

bool isfile(const char *path)
{
    struct stat attr;
    if (stat(path, &attr) == -1) return false;
    return S_ISREG(attr.st_mode);
}

bool create_dir(const char *path)
{
    if (mkdir(path) == -1){
        fprintf(stderr, "Could not create directory '%s': %s!\n", path, strerror(errno));
        return false;
    }
    return true;
}

void print_usage(char *program_name)
{
    printf("Licenses - How to use:\n");
    printf("  %s <license>\n", program_name);
    if (config.count == 0){
        printf("  There are no licenses available.\n");
        return;
    }
    printf("  Currently these licenses are available:\n");
    for (size_t i=0; i<config.count; ++i){
        ConpToken key = config.items[i].key;
        printf("    - %.*s\n", key.len, key.start);
    }
}

char* shift_args(int *argc, char ***argv)
{
	assert(*argc > 0 && "argv: out of bounds\n");
	char *result = **argv;
	*argc -= 1;
	*argv += 1;
	return result;
}

char* str_to_lower(char *buffer)
{
    char *r = buffer;
    char c;
    while ((c = *r) != '\0'){
        if (64 < c && c < 91){
            *r = c | 0x20;
        }
        r++;
    }
    return buffer;
}

unsigned long long file_size(const char* file_path){
    struct stat file;
    if (stat(file_path, &file) == -1){
        return 0;
    }
    return (unsigned long long) file.st_size;
}

// allocate and populate a string with the file's content
char* read_entire_file(char *file_path)
{
    if (file_path == NULL) return NULL;
    FILE *file = fopen(file_path, "r");
    if (file == NULL) return NULL;
    unsigned long long size = file_size(file_path);
    char *content = (char*) calloc(size+1, sizeof(*content));
    if (!content){
        fclose(file);
        return NULL;
    }
    fread(content, 1, size, file);
    fclose(file);
    return content;
}

int write_license(char *filepath)
{
    int result = 0;
    char *content = read_entire_file(filepath);
    if (!content){
        fprintf(stderr, "[ERROR] Could not read src file '%s'!\n", filepath);
        return 1;
    }
    FILE *file = fopen("LICENSE", "w");
    if (!file){
        fprintf(stderr, "[ERROR] Could not open `LICENSE` file!\n");
        return_defer(1);
    }
    size_t len = strlen(content);
    size_t n = fwrite(content, sizeof(char), len, file);
    if (n < len){
        fprintf(stderr, "[ERROR] Could only write %d of %d bytes to %s!\n", n, len, "LICENSE");
        return_defer(1);
    }
    printf("Successfully create LICENSE!\n");
  defer:
    free(content);
    fclose(file);
    return result;
}

int main(int argc, char **argv)
{
    int result;
    // create config files
    char *config_path = get_config_path();
    if (!isdir(config_path)){
        if (!create_dir(config_path)) return 1;
    }
    config_path = get_config_file_path();
    if (!isfile(config_path)){
        FILE *f = fopen(config_path, "w");
        fclose(f);
        printf("Created config file at '%s'.\n", config_path);
    }
    
    char *config_content = read_entire_file(config_path);
    if (config_content == NULL){
        fprintf(stderr, "Failed to read config file!\n");
        return 1;
    }

    if (!conp_parse_all(&config, config_content, strlen(config_content), CONFIG_FILE_NAME)){
        fprintf(stderr, "Failed to parse config!\n");
        return_defer(1);
    }
    char *program_name = shift_args(&argc, &argv);
    if (argc < 1){
        fprintf(stderr, "[ERROR] No license provided!\n");
        print_usage(program_name);
        return_defer(1);
    }
    char *license_input = str_to_lower(shift_args(&argc, &argv));
    if (strcmp(license_input, "-h") == 0){
        print_usage(program_name);
        return_defer(0);
    }
    else if (conp_entries_iskey(&config, license_input)){
        ConpToken token;
        if (!conp_entries_get(&config, license_input, &token)){
            fprintf(stderr, "Config changed during runtime!\n");
            return_defer(1);
        }
        if (!conp_extract(&token, temp_buffer, FILENAME_MAX)) return_defer(1);
        return write_license(temp_buffer);
    }
    else{
        fprintf(stderr, "[ERROR] Unknown license: \"%s\"!\n", license_input);
        print_usage(program_name);
        return_defer(1);
    }
  defer:
    free(config_content);
    return result;
}