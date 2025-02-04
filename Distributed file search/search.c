#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <mpi.h>

#define MAX_PATH_LENGTH 256

// Function to search a file for a keyword
int searchFile(const char *filePath, const char *keyword) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        return 0; // Error opening file
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, keyword) != NULL) {
            fclose(file);
            return 1; // Keyword found
        }
    }

    fclose(file);
    return 0; // Keyword not found
}

// Function to get list of files in a directory
int getFilesInDirectory(const char *directory, char ***files) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("Error opening directory");
        return 0;
    }

    int count = 0;
    *files = malloc(0);
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; // Skip "." and ".."
        }
        // Allocate memory for the new file name and add to list
        *files = realloc(*files, sizeof(char*) * (count + 1));
        (*files)[count] = malloc(MAX_PATH_LENGTH);
        snprintf((*files)[count], MAX_PATH_LENGTH, "%s/%s", directory, entry->d_name);
        count++;
    }

    closedir(dir);
    return count;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 3) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <directory> <keyword>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    char *directory = argv[1];
    char *keyword = argv[2];

    if (rank == 0) { // Master process
        char **files;
        int numFiles = getFilesInDirectory(directory, &files);

        if (numFiles == 0) {
            if (rank == 0) {
                fprintf(stderr, "No files found in the directory\n");
            }
            MPI_Finalize();
            return 1;
        }

        // Distribute files to slave processes
        int filesPerProcess = numFiles / (nprocs - 1);
        int remainder = numFiles % (nprocs - 1);

        for (int i = 1; i < nprocs; ++i) {
            int start = (i - 1) * filesPerProcess;
            int end = start + filesPerProcess;
            if (i == nprocs - 1) {
                end += remainder; // Assign remaining files to last process
            }

            // Send number of files to slave
            int numFilesForSlave = end - start;
            MPI_Send(&numFilesForSlave, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            // Send file names to slave
            for (int j = start; j < end; ++j) {
                MPI_Send(files[j], strlen(files[j]) + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
        }

        // Collect results from slaves
        char filePath[MAX_PATH_LENGTH];
        for (int i = 1; i < nprocs; ++i) {
            int numResults;
            MPI_Recv(&numResults, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int j = 0; j < numResults; ++j) {
                MPI_Recv(filePath, MAX_PATH_LENGTH, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("File containing the keyword '%s': %s\n", keyword, filePath);
            }
        }

        // Free memory allocated for file paths
        for (int i = 0; i < numFiles; ++i) {
            free(files[i]);
        }
        free(files);
    } else { // Slave processes
        int numFiles;
        MPI_Recv(&numFiles, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        char filePath[MAX_PATH_LENGTH];
        int numResults = 0;
        for (int i = 0; i < numFiles; ++i) {
            MPI_Recv(filePath, MAX_PATH_LENGTH, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (searchFile(filePath, keyword)) {
                numResults++;
                MPI_Send(&numResults, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(filePath, strlen(filePath) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Finalize();
    return 0;
}

