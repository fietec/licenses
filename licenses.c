#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define LICENSE_MIT "mit"
#define LICENSE_UN "un"

#define LICENSE_MIT_PATH ""
#define LICENSE_UN_PATH ""

#define return_defer(value) do{result = (value); goto defer;}while(0);

void print_usage(char *program_name)
{
    printf("%s <license>\n", program_name);
    printf("Currently these licenses are available:\n");
    printf("  - MIT [%s]\n", LICENSE_MIT);
    printf("  - UNLICENSE [%s]\n", LICENSE_UN);
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
        fprintf(stderr, "[ERROR] Could not read src file %s!\n", filepath);
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
    char *program_name = shift_args(&argc, &argv);
    if (argc < 1){
        fprintf(stderr, "[ERROR] No license provided!\n");
        print_usage(program_name);
        return 1;
    }
    char *license_input = str_to_lower(shift_args(&argc, &argv));
    if (strcmp(license_input, "-h") == 0){
        print_usage(program_name);
        return 0;
    }
    else if (strcmp(license_input, LICENSE_MIT) == 0){
        return write_license(LICENSE_MIT_PATH);
    }
    else if (strcmp(license_input, LICENSE_UN) == 0){
        return write_license(LICENSE_UN_PATH);
    }
    else{
        fprintf(stderr, "[ERROR] Unknown license: \"%s\"!\n", license_input);
        print_usage(program_name);
        return 1;
    }
}