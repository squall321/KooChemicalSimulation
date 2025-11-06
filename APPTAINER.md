# Apptainer/Singularity Container Guide

KooChemicalSimulation v6.0.0-alpha2
Phase 60: Container Distribution

This guide explains how to build and use KooChemicalSimulation containers with Apptainer (formerly Singularity).

## What is Apptainer?

Apptainer (Singularity) is a container platform designed for HPC environments. It provides:
- **No root daemon** - Runs as regular user
- **HPC-friendly** - Works with MPI, GPU, InfiniBand
- **Secure** - No privilege escalation
- **Portable** - Single-file containers

## Prerequisites

Install Apptainer:
```bash
# Ubuntu/Debian
sudo apt-get install -y apptainer

# Or from source
# See: https://apptainer.org/docs/admin/main/installation.html
```

## Building Containers

### Option 1: Build from Definition File

**CPU-only version:**
```bash
cd KooChemicalSimulation
sudo apptainer build koolab.sif apptainer.def
```

**GPU-enabled version:**
```bash
sudo apptainer build koolab-gpu.sif apptainer-gpu.def
```

### Option 2: Build from Docker Hub (Future)

```bash
# Once published
apptainer pull docker://koochemsim/koolab:latest
apptainer pull docker://koochemsim/koolab:gpu-latest
```

## Using Containers

### Basic Usage

```bash
# Run Python script
apptainer run koolab.sif myscript.py

# Interactive shell
apptainer shell koolab.sif

# Execute specific command
apptainer exec koolab.sif python -c "import koolab; koolab.hello()"
```

### With GPU Support

```bash
# Enable NVIDIA GPU with --nv flag
apptainer run --nv koolab-gpu.sif gpu_script.py

# Check GPU
apptainer exec --nv koolab-gpu.sif nvidia-smi

# Test GPU detection
apptainer exec --nv koolab-gpu.sif python -c "import koolab as koo; print(f'GPUs: {koo.gpu.device_count()}')"
```

### With MPI

```bash
# Run MPI application
mpirun -np 4 apptainer exec koolab.sif python mpi_script.py

# Hybrid MPI + GPU
mpirun -np 4 apptainer exec --nv koolab-gpu.sif python hybrid_mpi_gpu.py
```

### Binding Directories

```bash
# Bind current directory
apptainer run --bind $PWD:/workspace koolab.sif

# Bind multiple directories
apptainer run --bind /data:/data,/scratch:/scratch koolab.sif

# Set working directory
apptainer run --pwd /workspace --bind $PWD:/workspace koolab.sif
```

## Examples

### Example 1: Simple Simulation

```bash
# Create script
cat > example.py <<EOF
import koolab as koo
import numpy as np

print(f"KooLab version: {koo.__version__}")

# Create mesh
mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
print(f"Mesh: {mesh.num_nodes()} nodes, {mesh.num_elements()} elements")

# Create reaction
rxn = koo.parse_reaction("H2 + O2 => H2O")
print(f"Reaction: {rxn}")
EOF

# Run in container
apptainer run koolab.sif python example.py
```

### Example 2: GPU Acceleration

```bash
cat > gpu_example.py <<EOF
import koolab as koo

# Check GPU
info = koo.gpu.get_gpu_info()
print(f"GPU count: {info['device_count']}")
print(f"Runtime: {info['runtime']}")

if info['device_count'] > 0:
    dev = koo.Device.get_device(0)
    props = dev.get_properties()
    print(f"GPU: {props.name}")
    print(f"Memory: {props.total_memory / 1e9:.2f} GB")
EOF

# Run with GPU
apptainer run --nv koolab-gpu.sif python gpu_example.py
```

### Example 3: Jupyter Notebook

```bash
# Start Jupyter in container
apptainer exec koolab.sif jupyter notebook --ip=0.0.0.0 --port=8888 --no-browser

# Or with GPU
apptainer exec --nv koolab-gpu.sif jupyter notebook --ip=0.0.0.0 --port=8888 --no-browser

# Access at http://localhost:8888
```

### Example 4: Batch Job on HPC

```bash
#!/bin/bash
#SBATCH --job-name=koolab
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:2
#SBATCH --time=01:00:00

module load apptainer
module load mpi

# Run hybrid MPI + GPU simulation
mpirun -np 8 apptainer exec --nv koolab-gpu.sif python simulation.py
```

## Advanced Usage

### Environment Variables

```bash
# Set environment variables
apptainer run --env KOOLAB_DEBUG=1 koolab.sif script.py

# From file
apptainer run --env-file env.txt koolab.sif script.py
```

### Writable Overlay

```bash
# Create overlay for temporary files
apptainer run --overlay overlay.img koolab.sif script.py
```

### Instance Mode

```bash
# Start persistent instance
apptainer instance start koolab.sif koolab_instance

# Execute in instance
apptainer exec instance://koolab_instance python script.py

# Stop instance
apptainer instance stop koolab_instance
```

## Building Custom Containers

### Modify Definition File

```singularity
Bootstrap: docker
From: ubuntu:22.04

%post
    # Add custom software
    apt-get install -y mypackage

    # Install additional Python packages
    pip3 install mypackage

    # Custom KooLab configuration
    echo "export KOOLAB_CONFIG=/config" >> $SINGULARITY_ENVIRONMENT

%files
    # Copy your files
    myconfig.yaml /config/
```

### Build

```bash
sudo apptainer build custom-koolab.sif custom.def
```

## Performance Tips

1. **Bind fast scratch space** for I/O-intensive workloads
   ```bash
   apptainer run --bind /scratch:/scratch koolab.sif
   ```

2. **Use --nv for GPU** to enable NVIDIA support
   ```bash
   apptainer run --nv koolab-gpu.sif
   ```

3. **Enable MPI** for distributed computing
   ```bash
   mpirun -np 16 apptainer exec koolab.sif script.py
   ```

4. **Use SSD/NVMe** for container storage

## Troubleshooting

### GPU Not Detected

```bash
# Check --nv flag
apptainer exec --nv koolab-gpu.sif nvidia-smi

# Verify CUDA in container
apptainer exec --nv koolab-gpu.sif nvcc --version
```

### MPI Issues

```bash
# Use host MPI
mpirun -np 4 apptainer exec koolab.sif python script.py

# Or hybrid bind
apptainer exec --bind /usr/lib/x86_64-linux-gnu/openmpi koolab.sif
```

### Permission Denied

```bash
# Check file permissions
ls -la

# Bind with correct permissions
apptainer run --bind $PWD:/workspace --pwd /workspace koolab.sif
```

### Out of Memory

```bash
# Monitor memory usage
apptainer exec koolab.sif python -c "import koolab; ..."

# Increase GPU memory visibility
export CUDA_VISIBLE_DEVICES=0,1
```

## Container Registry

### Push to Registry (Future)

```bash
# Login
apptainer remote login

# Push
apptainer push koolab.sif library://user/collection/koolab:v6.0.0

# Or Docker Hub
docker push koochemsim/koolab:v6.0.0
```

### Pull from Registry

```bash
# From Sylabs Cloud
apptainer pull library://user/collection/koolab:v6.0.0

# From Docker Hub
apptainer pull docker://koochemsim/koolab:v6.0.0
```

## Comparison: Apptainer vs Docker

| Feature | Apptainer | Docker |
|---------|-----------|--------|
| Root daemon | No | Yes |
| HPC support | Excellent | Limited |
| MPI | Native | Complex |
| GPU | --nv flag | --gpus flag |
| Single file | Yes | No |
| Security | Strict | Moderate |

## Resources

- **Apptainer Docs**: https://apptainer.org/docs/
- **KooLab Repo**: https://github.com/squall321/KooChemicalSimulation
- **Issues**: https://github.com/squall321/KooChemicalSimulation/issues

## Version History

- **v6.0.0-alpha2** - Initial Apptainer support (Phase 60)
- **v6.0.0-alpha1** - GPU foundation (Phase 51-54)
- **v5.0.0** - Production release "Phoenix"

---

**Note**: Replace `apptainer` with `singularity` for older Singularity installations.
