#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    int rank, size;
    char hostname[256];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Get the hostname of the machine
    gethostname(hostname, sizeof(hostname));

    // Print rank and hostname
    printf("Hello from process %d of %d on %s\n", rank, size, hostname);

    MPI_Finalize();
    return 0;
}

