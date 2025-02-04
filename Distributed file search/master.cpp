#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <mpi.h>
#include <dirent.h>

using namespace std;

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

        // Check if there are any files
        if (numFiles == 0) {
            cerr << "Error: No files found in the directory!" << endl;
            MPI_Finalize();
            return 1;
        }

        // Check if there are enough processes
        if (nprocs <= 1) {
            cerr << "Error: Insufficient number of processes. You need at least 2 processes!" << endl;
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
    }

    MPI_Finalize();
    return 0;
}

