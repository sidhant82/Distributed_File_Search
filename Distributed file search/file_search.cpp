#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mpi.h>
#include <omp.h>
#include <dirent.h>

using namespace std;

// Function to search a file for a keyword
bool searchFile(const string& filePath, const string& keyword) {
    ifstream file(filePath);
    string line;
    while (getline(file, line)) {
        if (line.find(keyword) != string::npos) {
            return true; // Keyword found
        }
    }
    return false; // Keyword not found
}

// Function to get list of files in a directory
vector<string> getFilesInDirectory(const string& directory) {
    vector<string> files;
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(directory.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string fileName = ent->d_name;
            if (fileName != "." && fileName != "..") {
                files.push_back(directory + "/" + fileName);
            }
        }
        closedir(dir);
    } else {
        cerr << "Error: Could not open directory " << directory << endl;
    }
    return files;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 3) {
        if (rank == 0) {
            cerr << "Usage: " << argv[0] << " <directory> <keyword>" << endl;
        }
        MPI_Finalize();
        return 1;
    }

    string directory = argv[1];
    string keyword = argv[2];

    if (rank == 0) { // Master process
        // Get list of files in the directory
        vector<string> files = getFilesInDirectory(directory);
        int numFiles = files.size();

        // Distribute files to slave processes
        int filesPerProcess = numFiles / (nprocs - 1);
        int remainder = numFiles % (nprocs - 1);

        for (int i = 1; i < nprocs; ++i) {
            int start = (i - 1) * filesPerProcess;
            int end = start + filesPerProcess;
            if (i == nprocs - 1) {
                end += remainder; // Assign remaining files to the last process
            }

            // Send number of files to the slave
            int numFilesForSlave = end - start;
            MPI_Send(&numFilesForSlave, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            // Send file names to the slave
            for (int j = start; j < end; ++j) {
                MPI_Send(files[j].c_str(), files[j].size() + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
            }
        }

        // Collect results from slaves
        vector<string> results;
        for (int i = 1; i < nprocs; ++i) {
            int numResults;
            MPI_Recv(&numResults, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int j = 0; j < numResults; ++j) {
                char filePath[256];
                MPI_Recv(filePath, 256, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                results.push_back(filePath);
            }
        }

        // Display results
        cout << "Files containing the keyword '" << keyword << "':" << endl;
        for (const string& filePath : results) {
            cout << filePath << endl;
        }
    } else { // Slave processes
        // Receive number of files to process
        int numFiles;
        MPI_Recv(&numFiles, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive file names
        vector<string> files(numFiles);
        for (int i = 0; i < numFiles; ++i) {
            char filePath[256];
            MPI_Recv(filePath, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            files[i] = filePath;
        }

        // Search files in parallel using OpenMP
        vector<string> results;
        #pragma omp parallel for
        for (int i = 0; i < numFiles; ++i) {
            if (searchFile(files[i], keyword)) {
                #pragma omp critical
                results.push_back(files[i]);
            }
        }

        // Send results back to master
        int numResults = results.size();
        MPI_Send(&numResults, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (const string& filePath : results) {
            MPI_Send(filePath.c_str(), filePath.size() + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
