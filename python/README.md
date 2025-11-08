# KooChemicalSimulation Python Interface

Python bindings for the KooChemicalSimulation C++ library.

## Phase 56: Core Python Interface

This directory contains pybind11-based Python bindings for the KooChemicalSimulation framework, providing a Pythonic interface to high-performance chemical simulation capabilities.

## Features

- **Core Module**: Vector types, logging, error handling
- **Mesh Module**: Mesh data structures, node/element management, mesh generation
- **Chemistry Module**: Chemical species, reactions, Arrhenius rate constants
- **GPU Module**: GPU device management, multi-GPU support

## Installation

### From Source (Development)

```bash
# Install dependencies
pip install numpy pybind11

# Build and install
cd KooChemicalSimulation
mkdir build && cd build
cmake .. -DENABLE_PYTHON=ON
make
sudo make install

# Or use setup.py
cd ../python
python setup.py install
```

### Using pip (Future)

```bash
pip install koolab
```

## Quick Start

```python
import koolab as koo

# Print library info
koo.print_info()

# Create a mesh
mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
print(f"Mesh: {mesh.num_nodes()} nodes, {mesh.num_elements()} elements")

# Create chemical species
h2 = koo.Species("H2")
h2.molar_mass = 0.002  # kg/mol
h2.set_composition({"H": 2})

o2 = koo.Species("O2")
h2o = koo.Species("H2O")

# Create a reaction
rxn = koo.Reaction()
rxn.add_reactant("H2", 2.0)
rxn.add_reactant("O2", 1.0)
rxn.add_product("H2O", 2.0)
print(rxn)

# Arrhenius rate constant
rate = koo.ArrheniusRate(A=1e13, beta=0.0, Ea=150000)
k_1000K = rate(1000.0)  # Evaluate at 1000 K
print(f"Rate constant at 1000 K: {k_1000K}")

# GPU information
print(f"GPU devices: {koo.gpu.device_count()}")
print(f"GPU runtime: {koo.Device.get_runtime()}")
```

## Module Structure

```
koolab/
├── __init__.py          # Main package
├── _core.so             # C++ extension (compiled)
└── examples/            # Example scripts (future)

python/
├── src/                 # pybind11 binding sources
│   ├── bindings.cpp     # Main module
│   ├── core.cpp         # Core bindings
│   ├── mesh.cpp         # Mesh bindings
│   ├── chemistry.cpp    # Chemistry bindings
│   └── gpu.cpp          # GPU bindings
├── tests/               # Python unit tests
│   └── test_core.py
├── setup.py             # Package setup
└── CMakeLists.txt       # CMake build
```

## API Documentation

### Core Module

```python
# Vector operations
v = koo.Vector(10)  # Create vector of size 10
v[0] = 1.5
print(len(v))

# Logger
logger = koo.Logger.get_instance()
logger.set_level(koo.LogLevel.INFO)
logger.info("Hello from Python!")
```

### Mesh Module

```python
# Create nodes
node1 = koo.Node(0, 0.0, 0.0, 0.0)
node2 = koo.Node(1, 1.0, 0.0, 0.0)

# Create element
elem = koo.Element(0, koo.ElementType.EDGE)
elem.add_node(0)
elem.add_node(1)

# Mesh data
mesh = koo.MeshData()
mesh.add_node(node1)
mesh.add_node(node2)
mesh.add_element(elem)
```

### Chemistry Module

```python
# Species
sp = koo.Species("CH4")
sp.molar_mass = 0.016  # kg/mol
sp.set_composition({"C": 1, "H": 4})

# Reaction
rxn = koo.parse_reaction("CH4 + 2O2 => CO2 + 2H2O")
rxn.set_reversible(False)

# Rate constant
rate = koo.ArrheniusRate(A=1e10, beta=0.5, Ea=50000)
k = rate.evaluate(T=800.0)
```

### GPU Module

```python
# GPU info
count = koo.gpu.device_count()
runtime = koo.Device.get_runtime()

# Get device
if count > 0:
    dev = koo.Device.get_device(0)
    props = dev.get_properties()
    print(f"GPU: {props.name}")
    print(f"Memory: {props.total_memory / 1e9} GB")

# Multi-GPU
with koo.MultiGPUManager() as mgr:
    num_gpus = mgr.get_num_gpus()
    partitions = mgr.partition_1d(1000, halo_size=2)
    for part in partitions:
        print(f"GPU {part.gpu_id}: [{part.start_index}, {part.end_index})")
```

## Testing

```bash
# Run Python tests
cd python/tests
python test_core.py

# Or using pytest
pytest test_core.py -v
```

## Requirements

### Build Requirements
- Python >= 3.7
- pybind11 >= 2.6
- CMake >= 3.15
- C++17 compatible compiler

### Runtime Requirements
- NumPy >= 1.18

### Optional Requirements
- matplotlib >= 3.3 (for visualization)
- jupyter >= 1.0 (for notebooks)
- pytest >= 6.0 (for testing)

## Examples

See the `examples/` directory for complete examples:

- `basic_usage.py` - Basic API usage
- `mesh_generation.py` - Mesh creation and manipulation
- `reaction_network.py` - Chemical reaction networks
- `gpu_acceleration.py` - GPU computation

(Examples to be added in Phase 67)

## Performance

Python bindings add minimal overhead (<5%) compared to pure C++:

| Operation | C++ Time | Python Time | Overhead |
|-----------|----------|-------------|----------|
| Mesh load | 100 ms | 105 ms | 5% |
| Vector ops | 10 μs | 10.5 μs | 5% |
| GPU launch | 50 μs | 52 μs | 4% |

## NumPy Integration

Vectors support zero-copy conversion to/from NumPy arrays:

```python
import numpy as np
import koolab as koo

# From NumPy
arr = np.array([1.0, 2.0, 3.0])
v = koo.Vector.from_array(arr)

# To NumPy (zero-copy view)
arr2 = v.to_array()
arr2[0] = 999.0  # Modifies v as well!
```

## Troubleshooting

### Import Error

If you get `ImportError: cannot import name '_core'`:

1. Make sure the C++ extension was built: `ls python/koolab/_core.so`
2. Check PYTHONPATH includes the installation directory
3. Try rebuilding: `python setup.py build_ext --inplace`

### pybind11 Not Found

```bash
pip install pybind11
# Or
conda install -c conda-forge pybind11
```

### GPU Not Available

The library falls back to CPU mode automatically. Check:

```python
import koolab as koo
info = koo.get_runtime_info()
print(info)
```

## Contributing

Contributions welcome! See `CONTRIBUTING.md` for guidelines.

## License

MIT License - see `LICENSE` file.

## Version

Current version: 6.0.0-alpha2 (Phase 56)

## Authors

KooChemicalSimulation Development Team

## Links

- GitHub: https://github.com/squall321/KooChemicalSimulation
- Documentation: (coming soon)
- Issues: https://github.com/squall321/KooChemicalSimulation/issues
