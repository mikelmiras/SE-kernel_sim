#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

char *trim_whitespace(char *str) {
    char *end;

    // Trim leading space
    while (*str == ' ' || *str == '\t') str++;

    if (*str == '\0')  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        end--;

    *(end + 1) = '\0';

    return str;
}

// Function to retrieve the value for a given key in the config file
char *get_config_value(const char *filename, const char *key) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char *line_key, *line_value;

        // Find the '=' character
        char *equals_sign = strchr(line, '=');
        if (equals_sign) {
            // Split the line into key and value
            *equals_sign = '\0';
            line_key = trim_whitespace(line);
            line_value = trim_whitespace(equals_sign + 1);

            // Check if the key matches
            if (strcmp(line_key, key) == 0) {
                fclose(file);
                return strdup(line_value);  // Return a copy of the value
            }
        }
    }

    fclose(file);
    return NULL;  // Key not found
}