# API Reference

Comprehensive API documentation for KooChemicalSimulation v6.0.0-alpha4

## Table of Contents

- [Core Module](#core-module)
- [Mesh Module](#mesh-module)
- [Chemistry Module](#chemistry-module)
- [Solver Module](#solver-module)
- [GPU Module](#gpu-module)
- [Utils Module](#utils-module)
- [Python Bindings](#python-bindings)

---

## Core Module

### Namespace: `koo::core`

#### CommonTypes.h

**ElementType Enum**

```cpp
enum class ElementType {
    VERTEX,         // 0D vertex element
    LINE,           // 1D line element
    TRIANGLE,       // 2D triangular element
    QUADRILATERAL,  // 2D quadrilateral element
    TETRAHEDRON,    // 3D tetrahedral element
    HEXAHEDRON,     // 3D hexahedral element
    PRISM,          // 3D prism element
    PYRAMID         // 3D pyramid element
};
```

**Usage**:
```cpp
koo::core::ElementType type = koo::core::ElementType::QUADRILATERAL;
```

---

## Mesh Module

### Namespace: `koo::mesh::core`

### Class: `Node`

Represents a mesh node with coordinates and ID.

**Header**: `mesh/core/Node.h`

#### Constructors

```cpp
// Default constructor
Node();

// Constructor with ID and coordinates
Node(size_t id, double x, double y, double z = 0.0);

// Constructor with ID, coordinates, and tag
Node(size_t id, double x, double y, double z, int tag);
```

#### Methods

| Method | Return | Description |
|--------|--------|-------------|
| `getId()` | `size_t` | Get node ID |
| `setId(size_t id)` | `void` | Set node ID |
| `getX()` | `double` | Get X coordinate |
| `getY()` | `double` | Get Y coordinate |
| `getZ()` | `double` | Get Z coordinate |
| `setCoordinates(double x, double y, double z)` | `void` | Set coordinates |
| `getTag()` | `int` | Get physical tag |
| `setTag(int tag)` | `void` | Set physical tag |

**Example**:
```cpp
using namespace koo::mesh::core;

Node node(0, 1.0, 2.0, 0.0);
std::cout << "Node ID: " << node.getId() << std::endl;
std::cout << "Position: (" << node.getX() << ", "
          << node.getY() << ", " << node.getZ() << ")" << std::endl;
```

---

### Class: `Element`

Represents a mesh element (triangle, quad, etc.).

**Header**: `mesh/core/Element.h`

#### Constructors

```cpp
// Default constructor
Element();

// Constructor with ID, type, and node IDs
Element(size_t id, koo::core::ElementType type,
        const std::vector<size_t>& nodeIds);

// Constructor with ID, type, node IDs, and tag
Element(size_t id, koo::core::ElementType type,
        const std::vector<size_t>& nodeIds, int tag);
```

#### Methods

| Method | Return | Description |
|--------|--------|-------------|
| `getId()` | `size_t` | Get element ID |
| `setId(size_t id)` | `void` | Set element ID |
| `getType()` | `ElementType` | Get element type |
| `getNodeIds()` | `const vector<size_t>&` | Get node IDs |
| `getNumNodes()` | `size_t` | Get number of nodes |
| `getNodeId(size_t index)` | `size_t` | Get node ID at index |
| `getTag()` | `int` | Get physical tag |
| `setTag(int tag)` | `void` | Set physical tag |

**Example**:
```cpp
using namespace koo::mesh::core;

std::vector<size_t> nodeIds = {0, 1, 2, 3};
Element elem(0, koo::core::ElementType::QUADRILATERAL, nodeIds);

std::cout << "Element has " << elem.getNumNodes() << " nodes" << std::endl;
for (size_t i = 0; i < elem.getNumNodes(); ++i) {
    std::cout << "  Node " << i << ": ID = " << elem.getNodeId(i) << std::endl;
}
```

---

### Class: `MeshData`

Container for mesh nodes and elements.

**Header**: `mesh/core/MeshData.h`

#### Constructors

```cpp
MeshData();  // Default constructor
```

#### Node Operations

| Method | Return | Description |
|--------|--------|-------------|
| `addNode(const Node& node)` | `bool` | Add node to mesh |
| `getNode(size_t id)` | `Node*` | Get mutable node by ID |
| `getNode(size_t id) const` | `const Node*` | Get const node by ID |
| `getNodeByIndex(size_t index)` | `Node&` | Get node by index |
| `getNodes()` | `const vector<Node>&` | Get all nodes |
| `getNumNodes()` | `size_t` | Get number of nodes |
| `hasNode(size_t id)` | `bool` | Check if node exists |

#### Element Operations

| Method | Return | Description |
|--------|--------|-------------|
| `addElement(const Element& elem)` | `bool` | Add element to mesh |
| `getElement(size_t id)` | `Element*` | Get mutable element by ID |
| `getElement(size_t id) const` | `const Element*` | Get const element by ID |
| `getElementByIndex(size_t index)` | `Element&` | Get element by index |
| `getElements()` | `const vector<Element>&` | Get all elements |
| `getNumElements()` | `size_t` | Get number of elements |
| `hasElement(size_t id)` | `bool` | Check if element exists |

#### Utility Methods

| Method | Return | Description |
|--------|--------|-------------|
| `clear()` | `void` | Clear all mesh data |
| `getBoundingBox()` | `tuple<Point3D, Point3D>` | Get mesh bounding box |
| `computeStatistics()` | `MeshStatistics` | Compute mesh statistics |

**Example**:
```cpp
using namespace koo::mesh::core;

MeshData mesh;

// Add nodes
mesh.addNode(Node(0, 0.0, 0.0, 0.0));
mesh.addNode(Node(1, 1.0, 0.0, 0.0));
mesh.addNode(Node(2, 1.0, 1.0, 0.0));
mesh.addNode(Node(3, 0.0, 1.0, 0.0));

// Add element
std::vector<size_t> nodeIds = {0, 1, 2, 3};
mesh.addElement(Element(0, koo::core::ElementType::QUADRILATERAL, nodeIds));

std::cout << "Mesh has " << mesh.getNumNodes() << " nodes and "
          << mesh.getNumElements() << " elements" << std::endl;

// Access node
const Node* node = mesh.getNode(0);
if (node) {
    std::cout << "Node 0 position: (" << node->getX() << ", "
              << node->getY() << ")" << std::endl;
}
```

---

## Chemistry Module

### Namespace: `koo::chemistry`

### Enum: `PhaseType`

```cpp
enum class PhaseType {
    GAS,     // Gas phase
    LIQUID,  // Liquid phase
    SOLID    // Solid phase
};
```

### Class: `Species`

Represents a chemical species.

**Header**: `chemistry/species/Species.h`

#### Constructors

```cpp
// Default constructor
Species();

// Constructor with name, composition, and phase
Species(const std::string& name,
        const std::map<std::string, int>& composition,
        PhaseType phase = PhaseType::GAS);
```

#### Methods

| Method | Return | Description |
|--------|--------|-------------|
| `getName()` | `const string&` | Get species name |
| `setName(const string& name)` | `void` | Set species name |
| `getComposition()` | `const map<string, int>&` | Get elemental composition |
| `setComposition(const map& comp)` | `void` | Set composition |
| `getPhase()` | `PhaseType` | Get phase type |
| `setPhase(PhaseType phase)` | `void` | Set phase type |
| `getMolecularWeight()` | `double` | Get molecular weight (g/mol) |
| `setMolecularWeight(double mw)` | `void` | Set molecular weight |
| `getElementCount(const string& elem)` | `int` | Get atom count of element |
| `hasElement(const string& elem)` | `bool` | Check if contains element |

**Example**:
```cpp
using namespace koo::chemistry;

// Create CO2
std::map<std::string, int> composition = {{"C", 1}, {"O", 2}};
Species co2("CO2", composition, PhaseType::GAS);
co2.setMolecularWeight(44.01);

std::cout << "Species: " << co2.getName() << std::endl;
std::cout << "Carbon atoms: " << co2.getElementCount("C") << std::endl;
std::cout << "Molecular weight: " << co2.getMolecularWeight() << " g/mol" << std::endl;
```

---

## Utils Module

### Namespace: `koo::utils::logger`

### Enum: `LogLevel`

```cpp
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    OFF
};
```

### Class: `Logger`

Logging functionality.

**Header**: `utils/logger/Logger.h`

#### Static Methods

| Method | Description |
|--------|-------------|
| `initialize(const string& name)` | Initialize logger with name |
| `setLevel(LogLevel level)` | Set minimum log level |
| `debug(const string& msg)` | Log debug message |
| `info(const string& msg)` | Log info message |
| `warning(const string& msg)` | Log warning message |
| `error(const string& msg)` | Log error message |
| `critical(const string& msg)` | Log critical message |

**Example**:
```cpp
using namespace koo::utils::logger;

Logger::initialize("MyApp");
Logger::setLevel(LogLevel::INFO);

Logger::info("Application started");
Logger::debug("This won't be printed (level is INFO)");
Logger::warning("This is a warning");
Logger::error("This is an error");
```

---

### Namespace: `koo::utils::error`

### Class: `KooException`

Base exception class.

**Header**: `utils/error/Exception.h`

#### Constructor

```cpp
KooException(const std::string& message);
```

#### Methods

| Method | Return | Description |
|--------|--------|-------------|
| `what()` | `const char*` | Get error message |

**Example**:
```cpp
using namespace koo::utils::error;

try {
    if (value < 0) {
        throw KooException("Value must be non-negative");
    }
} catch (const KooException& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

---

## Python Bindings

### Module: `_core`

Python interface to KooLab.

**Import**:
```python
import _core as koo
```

### Functions

#### `version()`

Get library version.

```python
version_str = koo.version()
print(f"KooLab version: {version_str}")
```

#### `create_rectangular_mesh(x0, y0, x1, y1, nx, ny)`

Create a rectangular 2D mesh.

**Parameters**:
- `x0, y0`: Bottom-left corner
- `x1, y1`: Top-right corner
- `nx, ny`: Number of divisions

**Returns**: `MeshData` object

**Example**:
```python
mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
print(f"Mesh: {mesh}")
print(f"Nodes: {mesh.get_num_nodes()}")
print(f"Elements: {mesh.get_num_elements()}")
```

---

### Class: `Logger`

Static methods for logging.

#### Methods

```python
# Initialize
koo.Logger.initialize("MyApp")

# Set level
koo.Logger.set_level(koo.LogLevel.INFO)

# Log messages
koo.Logger.debug("Debug message")
koo.Logger.info("Info message")
koo.Logger.warning("Warning message")
koo.Logger.error("Error message")
```

---

### Class: `Node`

Python wrapper for mesh node.

#### Constructor

```python
node = koo.Node(id=0, x=1.0, y=2.0, z=0.0)
```

#### Properties

```python
node.id = 5          # Set ID
print(node.id)       # Get ID
print(node.x)        # Get X coordinate
print(node.y)        # Get Y coordinate
print(node.z)        # Get Z coordinate
```

---

### Class: `Element`

Python wrapper for mesh element.

#### Constructor

```python
node_ids = [0, 1, 2, 3]
elem = koo.Element(id=0, type=koo.ElementType.QUADRILATERAL, node_ids=node_ids)
```

#### Methods

```python
print(elem.id)                    # Get ID
print(elem.type)                  # Get type
print(elem.get_num_nodes())       # Get node count
print(elem.get_node_ids())        # Get node IDs as list
```

---

### Class: `MeshData`

Python wrapper for mesh container.

#### Constructor

```python
mesh = koo.MeshData()
```

#### Methods

```python
# Add nodes
node = koo.Node(0, 0.0, 0.0, 0.0)
mesh.add_node(node)

# Add elements
elem = koo.Element(0, koo.ElementType.QUADRILATERAL, [0, 1, 2, 3])
mesh.add_element(elem)

# Query
num_nodes = mesh.get_num_nodes()
num_elements = mesh.get_num_elements()

# Access
node = mesh.get_node(0)
elem = mesh.get_element(0)

# Clear
mesh.clear()
```

---

### Class: `Species`

Python wrapper for chemical species.

#### Constructor

```python
composition = {"C": 1, "O": 2}
species = koo.Species("CO2", composition, koo.PhaseType.GAS)
```

#### Properties

```python
species.name = "CH4"             # Set name
print(species.name)              # Get name
print(species.molecular_weight)  # Get molecular weight
```

---

### Enum: `ElementType`

```python
koo.ElementType.VERTEX
koo.ElementType.LINE
koo.ElementType.TRIANGLE
koo.ElementType.QUADRILATERAL
koo.ElementType.TETRAHEDRON
koo.ElementType.HEXAHEDRON
koo.ElementType.PRISM
koo.ElementType.PYRAMID
```

### Enum: `LogLevel`

```python
koo.LogLevel.TRACE
koo.LogLevel.DEBUG
koo.LogLevel.INFO
koo.LogLevel.WARNING
koo.LogLevel.ERROR
koo.LogLevel.CRITICAL
koo.LogLevel.OFF
```

### Enum: `PhaseType`

```python
koo.PhaseType.GAS
koo.PhaseType.LIQUID
koo.PhaseType.SOLID
```

---

## Complete Example

### C++ Example

```cpp
#include "mesh/core/MeshData.h"
#include "mesh/core/Node.h"
#include "mesh/core/Element.h"
#include "utils/logger/Logger.h"
#include "chemistry/species/Species.h"

int main() {
    using namespace koo;

    // Initialize logger
    utils::logger::Logger::initialize("CompleteExample");
    utils::logger::Logger::setLevel(utils::logger::LogLevel::INFO);
    utils::logger::Logger::info("Starting application");

    // Create mesh
    mesh::core::MeshData mesh;

    // Add nodes (4-node quad)
    mesh.addNode(mesh::core::Node(0, 0.0, 0.0, 0.0));
    mesh.addNode(mesh::core::Node(1, 1.0, 0.0, 0.0));
    mesh.addNode(mesh::core::Node(2, 1.0, 1.0, 0.0));
    mesh.addNode(mesh::core::Node(3, 0.0, 1.0, 0.0));

    // Add element
    std::vector<size_t> nodeIds = {0, 1, 2, 3};
    mesh.addElement(mesh::core::Element(0,
                   core::ElementType::QUADRILATERAL,
                   nodeIds));

    utils::logger::Logger::info("Created mesh with " +
                               std::to_string(mesh.getNumNodes()) + " nodes");

    // Create species
    std::map<std::string, int> comp = {{"H", 2}, {"O", 1}};
    chemistry::Species h2o("H2O", comp, chemistry::PhaseType::LIQUID);
    h2o.setMolecularWeight(18.015);

    utils::logger::Logger::info("Created species: " + h2o.getName());

    return 0;
}
```

### Python Example

```python
import _core as koo

# Initialize
koo.Logger.initialize("PythonExample")
koo.Logger.set_level(koo.LogLevel.INFO)
koo.Logger.info("Starting Python application")

# Create mesh
mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 5, 5)
koo.Logger.info(f"Created mesh with {mesh.get_num_nodes()} nodes")

# Create species
composition = {"H": 2, "O": 1}
h2o = koo.Species("H2O", composition, koo.PhaseType.LIQUID)
koo.Logger.info(f"Created species: {h2o.name}")

print("Application complete")
```

---

## Additional Resources

- [GETTING_STARTED.md](GETTING_STARTED.md) - Installation and setup
- [TUTORIALS.md](TUTORIALS.md) - Step-by-step tutorials
- [README.md](README.md) - Project overview
- `examples/` - Complete example programs
- `notebooks/` - Jupyter notebook tutorials

---

## Version History

- **v6.0.0-alpha4** (Current) - Production features
- **v6.0.0-alpha3** - Advanced GPU features
- **v6.0.0-alpha2** - Python ecosystem
- **v6.0.0-alpha1** - GPU acceleration foundation
- **v5.0.0** - CPU framework (Phoenix release)

For detailed changes, see [PROGRESS_SUMMARY.md](PROGRESS_SUMMARY.md).
