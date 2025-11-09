# Apptainer Quick Start Guide

Quick reference for using KooChemicalSimulation with Apptainer containers.

## Pre-built Images

### CPU-only Image

```bash
# Build the container
apptainer build koolab.sif apptainer.def

# Run interactive shell
apptainer shell koolab.sif

# Run a simulation
apptainer exec koolab.sif ./build/examples/diffusion_example

# Run tests
apptainer exec koolab.sif ctest --test-dir build
```

### GPU-enabled Image

```bash
# Build GPU container
apptainer build koolab-gpu.sif apptainer-gpu.def

# Run with NVIDIA GPU
apptainer exec --nv koolab-gpu.sif ./build/examples/gpu_example

# Run with AMD ROCm
apptainer exec --rocm koolab-gpu.sif ./build/examples/gpu_example
```

## Common Use Cases

### 1. Development Environment

```bash
# Mount your code directory
apptainer shell --bind $(pwd):/workspace koolab.sif

# Inside container
cd /workspace
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. HPC Cluster Usage

```bash
# SLURM example
#!/bin/bash
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=32
#SBATCH --time=01:00:00

module load apptainer

mpirun -np 128 apptainer exec koolab.sif \
    /opt/koolab/build/examples/mpi_diffusion_example
```

### 3. Reproducible Research

```bash
# Save exact environment
apptainer inspect koolab.sif

# Export environment description
apptainer inspect --deffile koolab.sif > my_env.def
```

## Building Custom Images

### Modify for Your Needs

Edit `apptainer.def` or `apptainer-gpu.def`:

```bash
Bootstrap: docker
From: ubuntu:22.04

%post
    # Add your custom dependencies here
    apt-get update
    apt-get install -y your-package

    # Add your custom build steps
    # ...
```

### Build with Custom Options

```bash
# Build with specific CMake options
apptainer build --build-arg CMAKE_OPTIONS="-DENABLE_MPI=ON" koolab.sif apptainer.def
```

## Performance Tips

### 1. Use --bind for Data

```bash
# Mount data directory for fast I/O
apptainer exec --bind /scratch/data:/data koolab.sif ./simulation
```

### 2. Enable SIF Cache

```bash
export APPTAINER_CACHEDIR=/scratch/$USER/apptainer-cache
```

### 3. Optimize for MPI

```bash
# Use host MPI for better performance
apptainer exec --bind /opt/mpi:/opt/mpi koolab.sif \
    mpirun -np 64 ./mpi_simulation
```

## Troubleshooting

### Permission Denied

```bash
# Run as fakeroot
apptainer build --fakeroot koolab.sif apptainer.def
```

### GPU Not Detected

```bash
# Check NVIDIA drivers
nvidia-smi

# Test GPU access
apptainer exec --nv koolab-gpu.sif nvidia-smi
```

### MPI Issues

```bash
# Use compatible MPI version
apptainer exec --bind $MPI_ROOT:/mpi koolab.sif \
    /mpi/bin/mpirun ./program
```

## Advanced Features

### Overlay Filesystem

```bash
# Create persistent overlay
apptainer overlay create --size 1024 koolab-overlay.img

# Use overlay
apptainer shell --overlay koolab-overlay.img koolab.sif
```

### Instance Management

```bash
# Start instance
apptainer instance start koolab.sif koolab-instance

# Use instance
apptainer exec instance://koolab-instance ./simulation

# Stop instance
apptainer instance stop koolab-instance
```

### Container Inspection

```bash
# View metadata
apptainer inspect koolab.sif

# View definition
apptainer inspect --deffile koolab.sif

# List apps
apptainer inspect --list-apps koolab.sif
```

## Integration with Job Schedulers

### SLURM

```bash
#!/bin/bash
#SBATCH --job-name=koolab-sim
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=128
#SBATCH --gres=gpu:4
#SBATCH --time=24:00:00

module load apptainer cuda

srun apptainer exec --nv koolab-gpu.sif \
    /opt/koolab/bin/simulation input.yaml
```

### PBS

```bash
#!/bin/bash
#PBS -N koolab-sim
#PBS -l nodes=8:ppn=32
#PBS -l walltime=24:00:00

cd $PBS_O_WORKDIR
module load apptainer

mpirun -np 256 apptainer exec koolab.sif \
    /opt/koolab/bin/simulation
```

## Best Practices

1. **Use specific tags**: `Bootstrap: docker` with specific versions
2. **Minimize layers**: Combine RUN commands
3. **Clear cache**: `%post` cleanup to reduce image size
4. **Version control**: Keep `.def` files in git
5. **Test locally**: Build and test before deploying to HPC

## More Information

- Full guide: [APPTAINER_GUIDE.md](APPTAINER_GUIDE.md)
- Official docs: https://apptainer.org/docs/
- HPC guide: [APPTAINER.md](APPTAINER.md)
