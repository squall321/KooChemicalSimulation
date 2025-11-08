"""
Matplotlib Visualization Module
Phase 58: Matplotlib Visualization

Provides convenient plotting functions for chemical simulation results.
"""

import numpy as np
from typing import Optional, List, Union, Tuple

try:
    import matplotlib.pyplot as plt
    import matplotlib.tri as tri
    from mpl_toolkits.mplot3d import Axes3D
    MPL_AVAILABLE = True
except ImportError:
    MPL_AVAILABLE = False


def check_matplotlib():
    """Check if matplotlib is available."""
    if not MPL_AVAILABLE:
        raise ImportError("matplotlib not installed. Install with: pip install matplotlib")


def plot_timeseries(times: np.ndarray, data: np.ndarray,
                   labels: Optional[List[str]] = None,
                   title: str = "Time Series",
                   xlabel: str = "Time",
                   ylabel: str = "Value",
                   figsize: Tuple[float, float] = (10, 6),
                   **kwargs):
    """
    Plot time series data.

    Parameters
    ----------
    times : np.ndarray
        Time points (n,)
    data : np.ndarray
        Data values (n,) or (n, m) for multiple series
    labels : list of str, optional
        Labels for each series
    title, xlabel, ylabel : str
        Plot labels
    figsize : tuple
        Figure size (width, height)
    **kwargs
        Additional matplotlib plot arguments

    Returns
    -------
    fig, ax
        Matplotlib figure and axes

    Examples
    --------
    >>> import numpy as np
    >>> t = np.linspace(0, 10, 100)
    >>> data = np.column_stack([np.exp(-t), np.exp(-0.5*t)])
    >>> fig, ax = koo.plot_timeseries(t, data, labels=['Fast', 'Slow'])
    >>> plt.show()
    """
    check_matplotlib()

    fig, ax = plt.subplots(figsize=figsize)

    if data.ndim == 1:
        ax.plot(times, data, label=labels[0] if labels else None, **kwargs)
    else:
        for i in range(data.shape[1]):
            label = labels[i] if labels and i < len(labels) else f"Series {i}"
            ax.plot(times, data[:, i], label=label, **kwargs)

    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    return fig, ax


def plot_1d(x: np.ndarray, values: np.ndarray,
           title: str = "1D Profile",
           xlabel: str = "Position",
           ylabel: str = "Value",
           figsize: Tuple[float, float] = (10, 6),
           **kwargs):
    """
    Plot 1D concentration or field profile.

    Parameters
    ----------
    x : np.ndarray
        Position coordinates
    values : np.ndarray
        Field values
    title, xlabel, ylabel : str
        Plot labels
    figsize : tuple
        Figure size
    **kwargs
        Additional matplotlib plot arguments

    Returns
    -------
    fig, ax
        Matplotlib figure and axes

    Examples
    --------
    >>> x = np.linspace(0, 1, 100)
    >>> c = np.exp(-10 * (x - 0.5)**2)  # Gaussian profile
    >>> fig, ax = koo.plot_1d(x, c, ylabel='Concentration')
    >>> plt.show()
    """
    check_matplotlib()

    fig, ax = plt.subplots(figsize=figsize)

    ax.plot(x, values, linewidth=2, **kwargs)
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    return fig, ax


def plot_2d(nodes: np.ndarray, values: np.ndarray,
           elements: Optional[np.ndarray] = None,
           title: str = "2D Field",
           colormap: str = "viridis",
           show_colorbar: bool = True,
           figsize: Tuple[float, float] = (10, 8),
           **kwargs):
    """
    Plot 2D field data on mesh.

    Parameters
    ----------
    nodes : np.ndarray
        Node coordinates (n x 2 or n x 3)
    values : np.ndarray
        Field values at nodes (n,)
    elements : np.ndarray, optional
        Element connectivity for triangulation
    title : str
        Plot title
    colormap : str
        Matplotlib colormap name
    show_colorbar : bool
        Show colorbar
    figsize : tuple
        Figure size
    **kwargs
        Additional matplotlib arguments

    Returns
    -------
    fig, ax
        Matplotlib figure and axes

    Examples
    --------
    >>> mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 20, 20)
    >>> nodes, elements = koo.numpy_utils.mesh_to_arrays(mesh)
    >>> values = nodes[:, 0]**2 + nodes[:, 1]**2  # r^2
    >>> fig, ax = koo.plot_2d(nodes, values)
    >>> plt.show()
    """
    check_matplotlib()

    fig, ax = plt.subplots(figsize=figsize)

    x = nodes[:, 0]
    y = nodes[:, 1]

    if elements is not None:
        # Use triangulation if elements provided
        triang = tri.Triangulation(x, y, triangles=elements)
        contour = ax.tricontourf(triang, values, cmap=colormap, **kwargs)
    else:
        # Scatter plot
        contour = ax.tricontourf(x, y, values, cmap=colormap, **kwargs)

    ax.set_xlabel('X', fontsize=12)
    ax.set_ylabel('Y', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')
    ax.set_aspect('equal')

    if show_colorbar:
        cbar = plt.colorbar(contour, ax=ax)
        cbar.set_label('Value', fontsize=12)

    plt.tight_layout()
    return fig, ax


def plot_3d_surface(x: np.ndarray, y: np.ndarray, z: np.ndarray,
                   title: str = "3D Surface",
                   colormap: str = "viridis",
                   figsize: Tuple[float, float] = (12, 9),
                   **kwargs):
    """
    Plot 3D surface.

    Parameters
    ----------
    x, y, z : np.ndarray
        Coordinate arrays (can be meshgrid or 1D)
    title : str
        Plot title
    colormap : str
        Colormap name
    figsize : tuple
        Figure size
    **kwargs
        Additional plot_surface arguments

    Returns
    -------
    fig, ax
        Matplotlib figure and 3D axes

    Examples
    --------
    >>> x = np.linspace(-2, 2, 50)
    >>> y = np.linspace(-2, 2, 50)
    >>> X, Y = np.meshgrid(x, y)
    >>> Z = np.exp(-(X**2 + Y**2))
    >>> fig, ax = koo.plot_3d_surface(X, Y, Z, title='Gaussian')
    >>> plt.show()
    """
    check_matplotlib()

    fig = plt.figure(figsize=figsize)
    ax = fig.add_subplot(111, projection='3d')

    if x.ndim == 1 and y.ndim == 1:
        x, y = np.meshgrid(x, y)

    surf = ax.plot_surface(x, y, z, cmap=colormap, **kwargs)

    ax.set_xlabel('X', fontsize=12)
    ax.set_ylabel('Y', fontsize=12)
    ax.set_zlabel('Z', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')

    fig.colorbar(surf, ax=ax, shrink=0.5)

    plt.tight_layout()
    return fig, ax


def plot_reaction_rates(times: np.ndarray, rates: np.ndarray,
                       reaction_labels: Optional[List[str]] = None,
                       title: str = "Reaction Rates",
                       figsize: Tuple[float, float] = (10, 6),
                       log_scale: bool = False):
    """
    Plot reaction rates over time.

    Parameters
    ----------
    times : np.ndarray
        Time points
    rates : np.ndarray
        Reaction rates (n_times x n_reactions)
    reaction_labels : list of str, optional
        Reaction labels
    title : str
        Plot title
    figsize : tuple
        Figure size
    log_scale : bool
        Use logarithmic y-scale

    Returns
    -------
    fig, ax
        Matplotlib figure and axes
    """
    check_matplotlib()

    fig, ax = plt.subplots(figsize=figsize)

    if rates.ndim == 1:
        rates = rates.reshape(-1, 1)

    for i in range(rates.shape[1]):
        label = reaction_labels[i] if reaction_labels and i < len(reaction_labels) else f"R{i}"
        ax.plot(times, rates[:, i], label=label, marker='o', markersize=3)

    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('Rate', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')

    if log_scale:
        ax.set_yscale('log')

    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    return fig, ax


def plot_species_evolution(times: np.ndarray, concentrations: np.ndarray,
                          species_names: List[str],
                          title: str = "Species Evolution",
                          figsize: Tuple[float, float] = (10, 6),
                          log_scale: bool = False):
    """
    Plot species concentration evolution.

    Parameters
    ----------
    times : np.ndarray
        Time points (n,)
    concentrations : np.ndarray
        Concentrations (n x n_species)
    species_names : list of str
        Species names
    title : str
        Plot title
    figsize : tuple
        Figure size
    log_scale : bool
        Use logarithmic y-scale

    Returns
    -------
    fig, ax
        Matplotlib figure and axes

    Examples
    --------
    >>> t = np.linspace(0, 10, 100)
    >>> C = np.column_stack([np.exp(-t), 1 - np.exp(-t)])
    >>> fig, ax = koo.plot_species_evolution(t, C, ['A', 'B'])
    >>> plt.show()
    """
    check_matplotlib()

    fig, ax = plt.subplots(figsize=figsize)

    for i, name in enumerate(species_names):
        ax.plot(times, concentrations[:, i], label=name, linewidth=2)

    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('Concentration', fontsize=12)
    ax.set_title(title, fontsize=14, fontweight='bold')

    if log_scale:
        ax.set_yscale('log')

    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    return fig, ax


def save_figure(fig, filename: str, dpi: int = 300, **kwargs):
    """
    Save figure to file.

    Parameters
    ----------
    fig : matplotlib.figure.Figure
        Figure to save
    filename : str
        Output filename
    dpi : int
        Resolution in dots per inch
    **kwargs
        Additional savefig arguments

    Examples
    --------
    >>> fig, ax = koo.plot_timeseries(t, data)
    >>> koo.save_figure(fig, 'timeseries.png', dpi=300)
    """
    check_matplotlib()

    fig.savefig(filename, dpi=dpi, bbox_inches='tight', **kwargs)
    print(f"Figure saved to {filename}")


__all__ = [
    'plot_timeseries',
    'plot_1d',
    'plot_2d',
    'plot_3d_surface',
    'plot_reaction_rates',
    'plot_species_evolution',
    'save_figure',
]
