/**
 * @file IMesh.h
 * @brief Interface for mesh representation and management
 * @author KooChemicalSimulation Development Team
 * @version 0.1.0-alpha2
 * @date 2025-11-06
 *
 * This file defines the IMesh interface, which provides an abstraction
 * for different mesh formats and mesh management systems.
 */

#ifndef KOO_CORE_INTERFACES_IMESH_H
#define KOO_CORE_INTERFACES_IMESH_H

#include <memory>
#include <string>
#include <vector>

namespace koo {
namespace core {

/**
 * @brief Enumeration of element types
 */
enum class ElementType {
    VERTEX,      ///< 0D vertex element
    LINE,        ///< 1D line element
    TRIANGLE,    ///< 2D triangular element
    QUADRILATERAL, ///< 2D quadrilateral element
    TETRAHEDRON, ///< 3D tetrahedral element
    HEXAHEDRON,  ///< 3D hexahedral element
    PRISM,       ///< 3D prism element
    PYRAMID      ///< 3D pyramid element
};

/**
 * @brief Enumeration of mesh dimensions
 */
enum class MeshDimension {
    ONE_D = 1,   ///< 1D mesh
    TWO_D = 2,   ///< 2D mesh
    THREE_D = 3  ///< 3D mesh
};

/**
 * @brief Structure representing a bounding box
 */
struct BoundingBox {
    double xmin, xmax;  ///< X-axis bounds
    double ymin, ymax;  ///< Y-axis bounds
    double zmin, zmax;  ///< Z-axis bounds
};

/**
 * @brief Interface for mesh objects
 *
 * IMesh provides a unified interface for different mesh representations.
 * This abstraction allows the framework to work with meshes from various
 * sources (gmsh, MFEM, manual generation, etc.).
 *
 * Key responsibilities:
 * - Loading and saving mesh data
 * - Providing access to mesh topology (nodes, elements, faces)
 * - Supporting boundary and domain markers
 * - Mesh refinement and coarsening
 * - Geometric queries (bounding box, element search)
 *
 * Design Pattern: Adapter Pattern (for different mesh formats)
 *
 * @note Mesh objects should be immutable after loading, except for refinement
 *
 * Example usage:
 * @code
 * auto mesh = createGmshMesh();
 * mesh->load("geometry.msh");
 * std::cout << "Nodes: " << mesh->getNumNodes() << std::endl;
 * std::cout << "Elements: " << mesh->getNumElements() << std::endl;
 * @endcode
 */
class IMesh {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IMesh() = default;

    /**
     * @brief Load mesh from file
     *
     * Loads mesh data from the specified file. The file format is
     * determined by the file extension or implementation.
     *
     * @param filename Path to mesh file
     * @throws std::runtime_error if file cannot be read or format invalid
     */
    virtual void load(const std::string& filename) = 0;

    /**
     * @brief Save mesh to file
     *
     * Saves the current mesh to a file in the implementation's native format.
     *
     * @param filename Path to output file
     * @throws std::runtime_error if file cannot be written
     */
    virtual void save(const std::string& filename) const = 0;

    /**
     * @brief Get mesh dimension
     *
     * @return Spatial dimension of the mesh (1D, 2D, or 3D)
     */
    virtual MeshDimension getDimension() const = 0;

    /**
     * @brief Get total number of nodes
     *
     * @return Number of nodes (vertices) in the mesh
     */
    virtual size_t getNumNodes() const = 0;

    /**
     * @brief Get total number of elements
     *
     * @return Number of elements in the mesh
     */
    virtual size_t getNumElements() const = 0;

    /**
     * @brief Get total number of boundary elements
     *
     * Boundary elements are lower-dimensional elements on the mesh boundary
     * (e.g., faces in 3D, edges in 2D).
     *
     * @return Number of boundary elements
     */
    virtual size_t getNumBoundaryElements() const = 0;

    /**
     * @brief Get coordinates of a specific node
     *
     * @param nodeId Node index (0-based)
     * @return Vector containing [x, y, z] coordinates (z=0 for 2D)
     * @throws std::out_of_range if nodeId is invalid
     */
    virtual std::vector<double> getNodeCoordinates(size_t nodeId) const = 0;

    /**
     * @brief Get connectivity of a specific element
     *
     * Returns the node indices that define the element.
     *
     * @param elementId Element index (0-based)
     * @return Vector of node indices
     * @throws std::out_of_range if elementId is invalid
     */
    virtual std::vector<size_t> getElementNodes(size_t elementId) const = 0;

    /**
     * @brief Get type of a specific element
     *
     * @param elementId Element index (0-based)
     * @return Element type
     * @throws std::out_of_range if elementId is invalid
     */
    virtual ElementType getElementType(size_t elementId) const = 0;

    /**
     * @brief Get physical domain marker for an element
     *
     * Physical markers are used to identify different regions or materials
     * in the mesh (as defined in gmsh or similar tools).
     *
     * @param elementId Element index (0-based)
     * @return Physical marker ID (0 if no marker)
     * @throws std::out_of_range if elementId is invalid
     */
    virtual int getElementMarker(size_t elementId) const = 0;

    /**
     * @brief Get boundary marker for a boundary element
     *
     * Boundary markers identify different boundary regions for applying
     * boundary conditions.
     *
     * @param boundaryElementId Boundary element index (0-based)
     * @return Boundary marker ID (0 if no marker)
     * @throws std::out_of_range if boundaryElementId is invalid
     */
    virtual int getBoundaryMarker(size_t boundaryElementId) const = 0;

    /**
     * @brief Get bounding box of the mesh
     *
     * @return Bounding box containing all mesh nodes
     */
    virtual BoundingBox getBoundingBox() const = 0;

    /**
     * @brief Refine the mesh uniformly
     *
     * Performs uniform refinement by subdividing all elements.
     *
     * @param levels Number of refinement levels (must be positive)
     * @throws std::invalid_argument if levels <= 0
     */
    virtual void refine(int levels = 1) = 0;

    /**
     * @brief Check if mesh is valid
     *
     * Validates mesh topology and geometry (e.g., checks for inverted elements,
     * duplicate nodes, disconnected regions).
     *
     * @return true if mesh is valid, false otherwise
     */
    virtual bool validate() const = 0;

    /**
     * @brief Get mesh quality metric
     *
     * Returns a mesh quality score, typically based on element shape quality.
     * Higher values indicate better quality.
     *
     * @return Quality metric in range [0, 1], where 1 is perfect
     */
    virtual double getQuality() const = 0;

    /**
     * @brief Get name/identifier of the mesh
     *
     * @return Mesh name or filename
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Check if mesh is empty
     *
     * @return true if mesh has no elements
     */
    virtual bool isEmpty() const {
        return getNumElements() == 0;
    }

    /**
     * @brief Get all physical group names
     *
     * Returns names of all physical groups defined in the mesh.
     * This is useful for identifying regions by name rather than ID.
     *
     * @return Vector of physical group names
     */
    virtual std::vector<std::string> getPhysicalGroupNames() const = 0;

    /**
     * @brief Get physical group ID by name
     *
     * @param name Physical group name
     * @return Physical group ID, or -1 if not found
     */
    virtual int getPhysicalGroupId(const std::string& name) const = 0;

protected:
    /**
     * @brief Protected default constructor
     */
    IMesh() = default;

    /**
     * @brief Protected copy constructor (deleted - meshes are not copyable)
     */
    IMesh(const IMesh&) = delete;

    /**
     * @brief Protected copy assignment (deleted)
     */
    IMesh& operator=(const IMesh&) = delete;

    /**
     * @brief Protected move constructor
     */
    IMesh(IMesh&&) = default;

    /**
     * @brief Protected move assignment
     */
    IMesh& operator=(IMesh&&) = default;
};

/**
 * @brief Shared pointer type for IMesh
 */
using IMeshPtr = std::shared_ptr<IMesh>;

/**
 * @brief Unique pointer type for IMesh
 */
using IMeshUniquePtr = std::unique_ptr<IMesh>;

} // namespace core
} // namespace koo

#endif // KOO_CORE_INTERFACES_IMESH_H
