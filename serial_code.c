#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>

#define MAX_FILES 1000      // Maximum number of files to store
#define MAX_FILENAME 512    // Maximum filename length
#define MAX_LINE 1024       // Maximum line length

// Function to search for a keyword in a file
int search_in_file(const char *filename, const char *keyword) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;  // File couldn't be opened

    char line[MAX_LINE];
    int count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, keyword)) {  // If keyword found in line
            count++;
        }
    }
    fclose(file);
    return count;
}

// Recursive function to scan directories and store file names
void scan_directory(char file_list[MAX_FILES][MAX_FILENAME], int *count, const char *dir_path, int depth, int max_depth) {
    if (max_depth != -1 && depth > max_depth) return;  // Stop recursion if depth limit reached

    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) && *count < MAX_FILES) {
        if (entry->d_type == DT_REG) {  // Regular file
            snprintf(file_list[*count], MAX_FILENAME, "%s/%s", dir_path, entry->d_name);
            (*count)++;
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {  
            // Recurse into subdirectory
            char subdir[MAX_FILENAME];
            snprintf(subdir, MAX_FILENAME, "%s/%s", dir_path, entry->d_name);
            scan_directory(file_list, count, subdir, depth + 1, max_depth);
        }
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: ./file_search_serial <directory> <keyword> <max_depth>\n");
        printf("       Use -1 for <max_depth> for unlimited recursion.\n");
        return 1;
    }

    char *dir_path = argv[1];
    char *keyword = argv[2];
    int max_depth = atoi(argv[3]);

    char file_list[MAX_FILES][MAX_FILENAME];
    int num_files = 0;

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);  // Start timer

    scan_directory(file_list, &num_files, dir_path, 0, max_depth);

    int total_count = 0;
    for (int i = 0; i < num_files; i++) {
        int occurrences = search_in_file(file_list[i], keyword);
        if (occurrences > 0) {
            printf("Found '%s' in file: %s (Occurrences: %d)\n", keyword, file_list[i], occurrences);
            total_count += occurrences;
        }
    }

    gettimeofday(&end_time, NULL);  // End timer
    double total_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("\n=== Search Results ===\n");
    printf("Total occurrences of '%s': %d\n", keyword, total_count);
    printf("Time taken: %.2f ms\n", total_time);

    return 0;
}
