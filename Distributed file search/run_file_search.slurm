#!/bin/bash
#SBATCH --job-name=file_search         # Job name
#SBATCH --nodes=2                      # Number of nodes (2 nodes)
#SBATCH --ntasks-per-node=2            # Number of MPI tasks per node (2 tasks per node)
#SBATCH --cpus-per-task=1              # Number of OpenMP threads per task (1 thread per task)
#SBATCH --time=00:10:00                # Time limit
#SBATCH --output=file_search_%j.out    # Output file
#SBATCH --error=file_search_%j.err     # Error file

# Load necessary modules (if required)
module load mpi/openmpi-x86_64

# Set OpenMP environment variable
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# Run the program
mpirun -np $SLURM_NTASKS ./file_search /home/master1/Distributed\ file\ search/
 keyword
