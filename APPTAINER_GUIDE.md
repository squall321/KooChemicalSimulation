# Apptainer Container Guide

This guide explains how to use Apptainer (formerly Singularity) to run KooLab in HPC environments.

---

## 📦 What is Apptainer?

[Apptainer](https://apptainer.org/) (formerly Singularity) is a container platform designed for HPC, scientific computing, and environments where Docker isn't suitable.

### Why Apptainer for KooLab?

- **HPC Standard**: Widely used in supercomputing centers
- **No Root Required**: Run without administrator privileges
- **MPI Integration**: Seamless parallel computing
- **GPU Support**: Easy NVIDIA GPU access with `--nv` flag
- **Reproducibility**: Identical environment across systems
- **Security**: Better isolation than Docker for multi-tenant systems

---

## 🚀 Quick Start

### Install Apptainer

**Ubuntu/Debian:**
```bash
# Add repository
sudo add-apt-repository -y ppa:apptainer/ppa
sudo apt update

# Install
sudo apt install -y apptainer

# Verify
apptainer --version
```

**CentOS/RHEL/Rocky:**
```bash
# Enable EPEL
sudo yum install -y epel-release

# Install Apptainer
sudo yum install -y apptainer

# Verify
apptainer --version
```

**From Source:**
```bash
# Install dependencies
sudo apt-get install -y build-essential libseccomp-dev pkg-config squashfs-tools cryptsetup

# Install Go
wget https://go.dev/dl/go1.21.0.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.21.0.linux-amd64.tar.gz
export PATH=/usr/local/go/bin:$PATH

# Clone and build Apptainer
git clone https://github.com/apptainer/apptainer.git
cd apptainer
./mconfig
make -C builddir
sudo make -C builddir install
```

### Build KooLab Container

**CPU Version:**
```bash
# Clone repository
git clone https://github.com/squall321/KooChemicalSimulation.git
cd KooChemicalSimulation

# Build container (requires sudo for build only)
sudo apptainer build koolab.sif koolab.def

# Run (no sudo needed!)
apptainer run koolab.sif /opt/koolab/bin/diffusion_example
```

**GPU Version:**
```bash
# Build GPU container
sudo apptainer build koolab_gpu.sif koolab_gpu.def

# Run with GPU support
apptainer run --nv koolab_gpu.sif /opt/koolab/bin/gpu_performance_comparison
```

---

## 🏗️ Building Containers

### CPU Container

```bash
# Build from definition file
sudo apptainer build koolab.sif koolab.def

# Build with custom cache directory
sudo APPTAINER_CACHEDIR=/tmp/cache apptainer build koolab.sif koolab.def

# Build from Docker Hub (if KooLab is published)
apptainer build koolab.sif docker://koolab/koolab:6.0.0-alpha5
```

**Build time**: ~15-20 minutes (depending on CPU and network)
**Image size**: ~2-3 GB

### GPU Container

```bash
# Build GPU version
sudo apptainer build koolab_gpu.sif koolab_gpu.def

# Specify CUDA architectures (optional, for custom builds)
# Edit koolab_gpu.def and modify CMAKE_CUDA_ARCHITECTURES
```

**Build time**: ~20-25 minutes
**Image size**: ~4-5 GB (includes CUDA toolkit)

### Build on HPC Without Root

Most HPC systems allow building without root using `--fakeroot`:

```bash
# Build without sudo (requires user namespace support)
apptainer build --fakeroot koolab.sif koolab.def
```

If `--fakeroot` isn't available, build on your local machine and transfer:

```bash
# On local machine (with sudo)
sudo apptainer build koolab.sif koolab.def

# Transfer to HPC
scp koolab.sif user@hpc.university.edu:~/containers/
```

---

## 🎮 Running Containers

### Basic Usage

```bash
# Run default script
apptainer run koolab.sif

# Execute specific command
apptainer exec koolab.sif /opt/koolab/bin/full_simulation_example

# Interactive shell
apptainer shell koolab.sif

# Python interface
apptainer exec koolab.sif python3 -c "import _core as koo; print('KooLab loaded')"
```

### With GPU Support

```bash
# Run with GPU (--nv flag)
apptainer run --nv koolab_gpu.sif /opt/koolab/bin/gpu_performance_comparison

# Check GPU
apptainer exec --nv koolab_gpu.sif nvidia-smi

# Python with CuPy
apptainer exec --nv koolab_gpu.sif python3 -c "import cupy; print(f'GPUs: {cupy.cuda.runtime.getDeviceCount()}')"
```

### Binding Directories

```bash
# Bind current directory (default: $HOME, /tmp, /proc, /sys, /dev)
apptainer exec --bind $(pwd):/data koolab.sif ls /data

# Multiple bindings
apptainer exec \
  --bind /scratch:/scratch \
  --bind /project:/project \
  koolab.sif /opt/koolab/bin/full_simulation_example

# Read-only binding
apptainer exec --bind /data:/data:ro koolab.sif cat /data/input.yaml
```

### Environment Variables

```bash
# Pass environment variable
apptainer exec --env OMP_NUM_THREADS=16 koolab.sif /opt/koolab/bin/diffusion_example

# Use cleanenv to ignore host environment
apptainer exec --cleanenv koolab.sif env

# Set multiple variables
apptainer exec \
  --env OMP_NUM_THREADS=8 \
  --env CUDA_VISIBLE_DEVICES=0,1 \
  koolab_gpu.sif /opt/koolab/bin/gpu_benchmark
```

---

## 🔬 HPC Cluster Usage

### SLURM Job Script

**CPU Job:**
```bash
#!/bin/bash
#SBATCH --job-name=koolab_cpu
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --time=01:00:00
#SBATCH --partition=compute

# Load Apptainer module (if needed)
module load apptainer

# Set OpenMP threads
export OMP_NUM_THREADS=16

# Run simulation
apptainer exec \
  --bind $SCRATCH:/scratch \
  /path/to/koolab.sif \
  /opt/koolab/bin/full_simulation_example /scratch/input.yaml
```

**GPU Job:**
```bash
#!/bin/bash
#SBATCH --job-name=koolab_gpu
#SBATCH --nodes=1
#SBATCH --gres=gpu:2
#SBATCH --time=01:00:00
#SBATCH --partition=gpu

# Load modules
module load apptainer cuda

# Run with GPU support
apptainer exec --nv \
  --bind $SCRATCH:/scratch \
  /path/to/koolab_gpu.sif \
  /opt/koolab/bin/gpu_performance_comparison
```

### PBS/Torque Job Script

```bash
#!/bin/bash
#PBS -N koolab_job
#PBS -l nodes=1:ppn=16
#PBS -l walltime=01:00:00
#PBS -q batch

cd $PBS_O_WORKDIR

apptainer exec \
  --bind $PBS_O_WORKDIR:/work \
  /home/user/containers/koolab.sif \
  /opt/koolab/bin/diffusion_example
```

### MPI Jobs (Future)

```bash
#!/bin/bash
#SBATCH --job-name=koolab_mpi
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=16

# MPI with Apptainer
mpirun -n 64 apptainer exec koolab.sif /opt/koolab/bin/mpi_simulation
```

---

## 🐍 Python & Jupyter

### Interactive Python

```bash
# Python interpreter
apptainer exec koolab.sif python3

# Run script
apptainer exec koolab.sif python3 my_simulation.py

# Run notebook
apptainer exec koolab.sif python3 -m jupyter run my_notebook.ipynb
```

### Jupyter Notebook Server

```bash
# Start Jupyter server
apptainer exec koolab.sif jupyter notebook --ip=0.0.0.0 --port=8888 --no-browser

# In another terminal, create SSH tunnel
ssh -L 8888:localhost:8888 user@hpc.university.edu

# Open in browser: http://localhost:8888
```

### JupyterLab

```bash
# Start JupyterLab
apptainer exec koolab.sif jupyter lab --ip=0.0.0.0 --port=8888 --no-browser
```

---

## 📊 Examples

### Example 1: Run Diffusion Simulation

```bash
# Create input file
cat > diffusion_input.yaml << EOF
simulation:
  name: "1D Diffusion Test"
  type: diffusion
  dimension: 1

mesh:
  nx: 1000
  length: 1.0

solver:
  dt: 0.001
  t_final: 1.0

output:
  format: vtk
  frequency: 100
EOF

# Run simulation
apptainer exec \
  --bind $(pwd):/work \
  koolab.sif \
  /opt/koolab/bin/diffusion_example /work/diffusion_input.yaml
```

### Example 2: GPU Performance Comparison

```bash
# Run GPU benchmark
apptainer exec --nv koolab_gpu.sif \
  /opt/koolab/bin/gpu_performance_comparison > results.txt

# View results
cat results.txt
```

### Example 3: Python Simulation

```bash
# Create Python script
cat > simulation.py << 'EOF'
import _core as koo
import numpy as np
import matplotlib.pyplot as plt

# Initialize
koo.Logger.initialize("Simulation")

# Create mesh
mesh = koo.create_rectangular_mesh(0.0, 0.0, 1.0, 1.0, 50, 50)

# Run simulation
print(f"Mesh nodes: {mesh.getNumNodes()}")
print(f"Mesh elements: {mesh.getNumElements()}")
EOF

# Run with Apptainer
apptainer exec koolab.sif python3 simulation.py
```

---

## 🔧 Advanced Features

### Overlay Filesystems

Add writable layer to read-only container:

```bash
# Create overlay
apptainer overlay create --size 1024 overlay.img

# Use overlay
apptainer exec --overlay overlay.img koolab.sif bash

# Inside container, changes persist in overlay
pip install --user extra-package
```

### Container Instances

Run container as background service:

```bash
# Start instance
apptainer instance start koolab.sif koolab_instance

# Execute in instance
apptainer exec instance://koolab_instance /opt/koolab/bin/long_running_job

# Stop instance
apptainer instance stop koolab_instance
```

### Custom Configuration

**~/.apptainer/apptainer.conf:**
```bash
# Bind directories automatically
bind path = /scratch
bind path = /project
bind path = /data

# Disable automatic home mount
mount home = no

# Use specific cache directory
cache dir = /tmp/apptainer-cache
```

---

## 🐛 Troubleshooting

### Problem: "permission denied" when building

**Solution**: Use sudo for build (not for run):
```bash
sudo apptainer build koolab.sif koolab.def
```

Or use `--fakeroot` if available:
```bash
apptainer build --fakeroot koolab.sif koolab.def
```

### Problem: GPU not detected

**Solution**: Use `--nv` flag and check driver:
```bash
# Check host driver
nvidia-smi

# Run with GPU support
apptainer exec --nv koolab_gpu.sif nvidia-smi
```

### Problem: "out of space" during build

**Solution**: Set cache directory to location with more space:
```bash
export APPTAINER_CACHEDIR=/scratch/tmp
sudo -E apptainer build koolab.sif koolab.def
```

### Problem: Cannot access files

**Solution**: Bind the directory:
```bash
apptainer exec --bind /path/to/data:/data koolab.sif ls /data
```

### Problem: Python module not found

**Solution**: Check PYTHONPATH is set (automatic in %environment):
```bash
apptainer exec koolab.sif python3 -c "import sys; print(sys.path)"
```

---

## 📊 Performance Tips

### CPU Optimization

```bash
# Set optimal thread count
export OMP_NUM_THREADS=$(nproc)
apptainer exec koolab.sif /opt/koolab/bin/benchmark_suite

# Bind to specific CPUs
taskset -c 0-15 apptainer exec koolab.sif /opt/koolab/bin/diffusion_example
```

### GPU Optimization

```bash
# Select specific GPU
export CUDA_VISIBLE_DEVICES=1
apptainer exec --nv koolab_gpu.sif /opt/koolab/bin/gpu_benchmark

# Multiple GPUs
export CUDA_VISIBLE_DEVICES=0,1,2,3
apptainer exec --nv koolab_gpu.sif /opt/koolab/bin/multi_gpu_example
```

### Memory Management

```bash
# Limit memory
apptainer exec --memory 16GB koolab.sif /opt/koolab/bin/large_simulation

# Check memory usage
apptainer exec koolab.sif free -h
```

---

## 📚 Additional Resources

- **Apptainer Documentation**: https://apptainer.org/docs/
- **HPC Container Guide**: https://apptainer.org/docs/user/main/quick_start.html
- **GPU Support**: https://apptainer.org/docs/user/main/gpu.html
- **MPI**: https://apptainer.org/docs/user/main/mpi.html
- **KooLab Repository**: https://github.com/squall321/KooChemicalSimulation

---

## ✅ Quick Reference

```bash
# Build containers
sudo apptainer build koolab.sif koolab.def
sudo apptainer build koolab_gpu.sif koolab_gpu.def

# Run examples
apptainer run koolab.sif
apptainer run --nv koolab_gpu.sif

# Execute command
apptainer exec koolab.sif /opt/koolab/bin/diffusion_example

# Interactive shell
apptainer shell koolab.sif
apptainer shell --nv koolab_gpu.sif

# Python
apptainer exec koolab.sif python3 script.py

# Jupyter
apptainer exec koolab.sif jupyter notebook --ip=0.0.0.0

# With bindings
apptainer exec --bind /data:/data koolab.sif ls /data

# With GPU
apptainer exec --nv koolab_gpu.sif nvidia-smi

# Background instance
apptainer instance start koolab.sif my_instance
apptainer exec instance://my_instance command
apptainer instance stop my_instance
```

---

**Ready for HPC!** 🚀
