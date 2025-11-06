"""
NumPy Integration Utilities
Phase 57: NumPy Advanced Integration

Provides advanced NumPy integration including zero-copy operations,
structured arrays, and efficient data conversion.
"""

import numpy as np
from typing import Optional, Union, Tuple, List

try:
    from . import _core
    CPP_AVAILABLE = True
except ImportError:
    CPP_AVAILABLE = False


def to_numpy(vector, copy: bool = False) -> np.ndarray:
    """
    Convert C++ Vector to NumPy array.

    Parameters
    ----------
    vector : koolab.Vector
        C++ vector to convert
    copy : bool, optional
        If True, copy data. If False, use zero-copy view (default)

    Returns
    -------
    np.ndarray
        NumPy array (view or copy)

    Examples
    --------
    >>> import koolab as koo
    >>> v = koo.Vector(100, 1.5)
    >>> arr = koo.numpy_utils.to_numpy(v)
    >>> arr[0] = 2.5  # Modifies v as well (zero-copy)
    """
    if not CPP_AVAILABLE:
        raise RuntimeError("C++ extension not available")

    arr = vector.to_array()
    return arr.copy() if copy else arr


def from_numpy(array: np.ndarray, copy: bool = True):
    """
    Create C++ Vector from NumPy array.

    Parameters
    ----------
    array : np.ndarray
        NumPy array to convert
    copy : bool, optional
        If True, copy data (default). If False, use view when possible

    Returns
    -------
    koolab.Vector
        C++ vector

    Examples
    --------
    >>> import numpy as np
    >>> import koolab as koo
    >>> arr = np.array([1.0, 2.0, 3.0])
    >>> v = koo.numpy_utils.from_numpy(arr)
    """
    if not CPP_AVAILABLE:
        raise RuntimeError("C++ extension not available")

    from . import Vector

    # Ensure contiguous double array
    arr = np.ascontiguousarray(array, dtype=np.float64)
    return Vector.from_array(arr)


def mesh_to_arrays(mesh) -> Tuple[np.ndarray, np.ndarray]:
    """
    Convert mesh to NumPy arrays.

    Parameters
    ----------
    mesh : koolab.MeshData
        Mesh to convert

    Returns
    -------
    nodes : np.ndarray
        Node coordinates (n_nodes x 3)
    elements : list of np.ndarray
        Element connectivity arrays

    Examples
    --------
    >>> mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
    >>> nodes, elements = koo.numpy_utils.mesh_to_arrays(mesh)
    >>> print(nodes.shape)  # (121, 3)
    """
    if not CPP_AVAILABLE:
        raise RuntimeError("C++ extension not available")

    n_nodes = mesh.num_nodes()
    nodes = np.zeros((n_nodes, 3))

    for i in range(n_nodes):
        node = mesh.get_node(i)
        nodes[i] = [node.x, node.y, node.z]

    n_elements = mesh.num_elements()
    elements = []

    for i in range(n_elements):
        elem = mesh.get_element(i)
        elem_nodes = np.array(elem.get_nodes())
        elements.append(elem_nodes)

    return nodes, elements


def create_structured_array(data: dict) -> np.ndarray:
    """
    Create NumPy structured array from dictionary.

    Parameters
    ----------
    data : dict
        Dictionary with field names as keys and arrays as values

    Returns
    -------
    np.ndarray
        Structured array

    Examples
    --------
    >>> data = {
    ...     'concentration': np.array([1.0, 2.0, 3.0]),
    ...     'temperature': np.array([300, 400, 500]),
    ...     'pressure': np.array([1e5, 2e5, 3e5])
    ... }
    >>> arr = koo.numpy_utils.create_structured_array(data)
    >>> print(arr['concentration'])
    """
    # Determine dtypes and create dtype list
    dtype_list = []
    first_key = next(iter(data))
    n = len(data[first_key])

    for key, val in data.items():
        if len(val) != n:
            raise ValueError(f"All arrays must have same length, got {len(val)} for {key}")
        dtype_list.append((key, val.dtype))

    # Create structured array
    structured = np.zeros(n, dtype=dtype_list)

    for key, val in data.items():
        structured[key] = val

    return structured


def concentration_matrix(species_data: dict, num_cells: int) -> np.ndarray:
    """
    Create concentration matrix for multiple species.

    Parameters
    ----------
    species_data : dict
        Dictionary mapping species names to concentration arrays
    num_cells : int
        Number of spatial cells

    Returns
    -------
    np.ndarray
        Concentration matrix (num_cells x num_species)

    Examples
    --------
    >>> data = {
    ...     'H2': np.ones(100) * 0.5,
    ...     'O2': np.ones(100) * 0.25,
    ...     'H2O': np.zeros(100)
    ... }
    >>> C = koo.numpy_utils.concentration_matrix(data, 100)
    >>> print(C.shape)  # (100, 3)
    """
    num_species = len(species_data)
    C = np.zeros((num_cells, num_species))

    for i, (name, conc) in enumerate(species_data.items()):
        if len(conc) != num_cells:
            raise ValueError(f"Species {name} has wrong size: {len(conc)} != {num_cells}")
        C[:, i] = conc

    return C


def compute_statistics(data: np.ndarray) -> dict:
    """
    Compute statistical summary of data.

    Parameters
    ----------
    data : np.ndarray
        Input data

    Returns
    -------
    dict
        Statistical summary (min, max, mean, std, median)

    Examples
    --------
    >>> data = np.random.randn(1000)
    >>> stats = koo.numpy_utils.compute_statistics(data)
    >>> print(f"Mean: {stats['mean']:.3f}, Std: {stats['std']:.3f}")
    """
    return {
        'min': np.min(data),
        'max': np.max(data),
        'mean': np.mean(data),
        'std': np.std(data),
        'median': np.median(data),
        'shape': data.shape,
        'dtype': data.dtype
    }


def interpolate_to_grid(nodes: np.ndarray, values: np.ndarray,
                       grid_x: np.ndarray, grid_y: np.ndarray) -> np.ndarray:
    """
    Interpolate scattered data to regular grid.

    Parameters
    ----------
    nodes : np.ndarray
        Node coordinates (n x 2 or n x 3)
    values : np.ndarray
        Values at nodes (n,)
    grid_x, grid_y : np.ndarray
        Regular grid coordinates

    Returns
    -------
    np.ndarray
        Interpolated values on grid

    Examples
    --------
    >>> nodes = np.random.rand(100, 2)
    >>> values = np.random.rand(100)
    >>> x = np.linspace(0, 1, 50)
    >>> y = np.linspace(0, 1, 50)
    >>> grid = koo.numpy_utils.interpolate_to_grid(nodes, values, x, y)
    """
    from scipy.interpolate import griddata

    points = nodes[:, :2]  # Use only x, y
    grid_values = griddata(points, values, (grid_x, grid_y), method='linear')

    return grid_values


def save_arrays(filename: str, **arrays):
    """
    Save multiple arrays to NPZ file.

    Parameters
    ----------
    filename : str
        Output filename (will add .npz if not present)
    **arrays
        Arrays to save as keyword arguments

    Examples
    --------
    >>> koo.numpy_utils.save_arrays('simulation.npz',
    ...                             concentration=C,
    ...                             temperature=T,
    ...                             time=t)
    """
    if not filename.endswith('.npz'):
        filename += '.npz'

    np.savez_compressed(filename, **arrays)


def load_arrays(filename: str) -> dict:
    """
    Load arrays from NPZ file.

    Parameters
    ----------
    filename : str
        Input filename

    Returns
    -------
    dict
        Dictionary of loaded arrays

    Examples
    --------
    >>> data = koo.numpy_utils.load_arrays('simulation.npz')
    >>> C = data['concentration']
    >>> T = data['temperature']
    """
    if not filename.endswith('.npz'):
        filename += '.npz'

    data = np.load(filename)
    return {key: data[key] for key in data.files}


__all__ = [
    'to_numpy',
    'from_numpy',
    'mesh_to_arrays',
    'create_structured_array',
    'concentration_matrix',
    'compute_statistics',
    'interpolate_to_grid',
    'save_arrays',
    'load_arrays',
]
