#!/bin/bash
#SBATCH --job-name=job_a              
#SBATCH --output=job_a_output.txt     
#SBATCH --error=job_a_error.txt       
#SBATCH --nodes=1                     
#BATCH --ntasks=1                    
#SBATCH --cpus-per-task=2            
#SBATCH --time=00:10:00              
#SBATCH --mem=1G                     

# Load any necessary modules (if required)
# module load gcc      # Uncomment if you're using a specific compiler or environment module

# Print a start message
echo "Starting Job A..."

# Compile the C program (Job A) - only needed once before running the script
gcc job_a.c -o job_a_program

# Run the compiled C program (Job A)
./job_a_program

# Print a completion message
echo "Job A completed."

