#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>

#define MAX_FILES 100  // Maximum files per node
#define MAX_FILENAME 256  // Maximum filename length
#define MAX_LINE 1024  // Maximum line length in files

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

// Function to scan directory and store file names
int get_files(char file_list[MAX_FILES][MAX_FILENAME], const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return 0;

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) && count < MAX_FILES) {
        if (entry->d_type == DT_REG) {  // Only regular files
            snprintf(file_list[count], MAX_FILENAME, "%s/%s", dir_path, entry->d_name);
            count++;
        }
    }
    closedir(dir);
    return count;
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) {
            printf("Usage: mpirun -np <num_processes> ./file_search_mpi <directory> <keyword>\n");
        }
        MPI_Finalize();
        return 1;
    }

    char *dir_path = argv[1];
    char *keyword = argv[2];

    char file_list[MAX_FILES][MAX_FILENAME];
    int num_files = get_files(file_list, dir_path);

    // Distribute file list among nodes
    int files_per_proc = num_files / size;
    int start = rank * files_per_proc;
    int end = (rank == size - 1) ? num_files : start + files_per_proc;

    int local_count = 0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);  // Start timer

    for (int i = start; i < end; i++) {
        int occurrences = search_in_file(file_list[i], keyword);
        if (occurrences > 0) {
            printf("Process %d found '%s' in file: %s (Occurrences: %d)\n", rank, keyword, file_list[i], occurrences);
            local_count += occurrences;
        }
    }

    gettimeofday(&end_time, NULL);  // End timer
    double local_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Gather results at master node
    int total_count;
    MPI_Reduce(&local_count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n=== Search Results ===\n");
        printf("Total occurrences of '%s': %d\n", keyword, total_count);
        printf("Time taken: %.2f ms\n", max_time);
    }

    MPI_Finalize();
    return 0;
}
