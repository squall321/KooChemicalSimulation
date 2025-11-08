"""
Real-time visualization for KooChemicalSimulation
Phase 68: Real-time Visualization

Features:
- Live plotting during simulation
- Interactive controls (pause, resume, stop)
- Multiple plot types (timeseries, 2D fields, 3D)
- Automatic update at specified intervals
- Data buffering and downsampling
- Export to video/images
"""

import numpy as np
import threading
import time
import queue
from typing import Optional, Callable, Dict, List, Tuple, Any

try:
    import matplotlib
    matplotlib.use('TkAgg')  # Use Tk backend for interactive plotting
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    from matplotlib.figure import Figure
    from matplotlib.axes import Axes
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

try:
    from mpl_toolkits.mplot3d import Axes3D
    HAS_3D = True
except ImportError:
    HAS_3D = False


class RealtimePlotter:
    """
    Real-time plotter with interactive controls
    """

    def __init__(self, update_interval: float = 0.1,
                 max_points: int = 1000,
                 figsize: Tuple[int, int] = (10, 6)):
        """
        Initialize real-time plotter

        Args:
            update_interval: Update interval in seconds
            max_points: Maximum number of points to display
            figsize: Figure size (width, height)
        """
        if not HAS_MATPLOTLIB:
            raise ImportError("matplotlib is required for real-time plotting")

        self.update_interval = update_interval
        self.max_points = max_points
        self.figsize = figsize

        self.fig: Optional[Figure] = None
        self.axes: List[Axes] = []
        self.lines: Dict[str, Any] = {}
        self.data_queue: queue.Queue = queue.Queue()

        self.is_running = False
        self.is_paused = False
        self.thread: Optional[threading.Thread] = None

        # Data storage
        self.time_data: List[float] = []
        self.series_data: Dict[str, List[float]] = {}

    def create_figure(self, num_subplots: int = 1,
                     subplot_layout: Optional[Tuple[int, int]] = None):
        """
        Create figure with subplots

        Args:
            num_subplots: Number of subplots
            subplot_layout: Layout as (rows, cols), auto if None
        """
        if subplot_layout is None:
            # Auto layout
            rows = int(np.ceil(np.sqrt(num_subplots)))
            cols = int(np.ceil(num_subplots / rows))
            subplot_layout = (rows, cols)

        self.fig, axes = plt.subplots(*subplot_layout, figsize=self.figsize)

        if num_subplots == 1:
            self.axes = [axes]
        else:
            self.axes = axes.flatten().tolist()[:num_subplots]

        plt.ion()  # Interactive mode on
        self.fig.show()

    def add_line(self, name: str, ax_index: int = 0,
                label: Optional[str] = None,
                color: Optional[str] = None,
                linestyle: str = '-',
                marker: Optional[str] = None):
        """
        Add a line to plot

        Args:
            name: Series name
            ax_index: Subplot index
            label: Legend label
            color: Line color
            linestyle: Line style
            marker: Marker style
        """
        if ax_index >= len(self.axes):
            raise ValueError(f"Invalid subplot index: {ax_index}")

        ax = self.axes[ax_index]
        line, = ax.plot([], [], label=label or name,
                       color=color, linestyle=linestyle, marker=marker)

        self.lines[name] = {'line': line, 'ax_index': ax_index}
        self.series_data[name] = []

        if label:
            ax.legend()

    def update_data(self, time: float, data: Dict[str, float]):
        """
        Update data (called from simulation)

        Args:
            time: Current time
            data: Dictionary of {series_name: value}
        """
        self.data_queue.put((time, data))

    def _process_queue(self):
        """Process queued data updates"""
        try:
            while not self.data_queue.empty():
                time, data = self.data_queue.get_nowait()

                # Add time point
                self.time_data.append(time)

                # Add data for each series
                for name, value in data.items():
                    if name in self.series_data:
                        self.series_data[name].append(value)

                # Downsample if too many points
                if len(self.time_data) > self.max_points:
                    step = 2
                    self.time_data = self.time_data[::step]
                    for name in self.series_data:
                        self.series_data[name] = self.series_data[name][::step]

        except queue.Empty:
            pass

    def _update_plot(self):
        """Update the plot display"""
        self._process_queue()

        if not self.time_data:
            return

        # Update each line
        for name, line_info in self.lines.items():
            if name in self.series_data and self.series_data[name]:
                line = line_info['line']
                ax_index = line_info['ax_index']

                # Update line data
                line.set_data(self.time_data, self.series_data[name])

                # Auto-scale
                ax = self.axes[ax_index]
                ax.relim()
                ax.autoscale_view()

        self.fig.canvas.draw()
        self.fig.canvas.flush_events()

    def start(self):
        """Start real-time plotting"""
        if self.is_running:
            return

        if self.fig is None:
            self.create_figure()

        self.is_running = True
        self.is_paused = False

        # Start update thread
        self.thread = threading.Thread(target=self._run_loop, daemon=True)
        self.thread.start()

    def _run_loop(self):
        """Main update loop"""
        while self.is_running:
            if not self.is_paused:
                try:
                    self._update_plot()
                except Exception as e:
                    print(f"Plot update error: {e}")

            time.sleep(self.update_interval)

    def pause(self):
        """Pause plotting"""
        self.is_paused = True

    def resume(self):
        """Resume plotting"""
        self.is_paused = False

    def stop(self):
        """Stop plotting"""
        self.is_running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        plt.ioff()

    def clear(self):
        """Clear all data"""
        self.time_data.clear()
        for name in self.series_data:
            self.series_data[name].clear()

    def save_figure(self, filename: str, dpi: int = 150):
        """Save current figure to file"""
        if self.fig:
            self.fig.savefig(filename, dpi=dpi, bbox_inches='tight')

    def __enter__(self):
        """Context manager entry"""
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop()


class Realtime2DPlotter:
    """
    Real-time 2D field plotter (heatmap/contour)
    """

    def __init__(self, update_interval: float = 0.1,
                 figsize: Tuple[int, int] = (8, 6)):
        """
        Initialize 2D plotter

        Args:
            update_interval: Update interval in seconds
            figsize: Figure size
        """
        if not HAS_MATPLOTLIB:
            raise ImportError("matplotlib is required for real-time plotting")

        self.update_interval = update_interval
        self.figsize = figsize

        self.fig: Optional[Figure] = None
        self.ax: Optional[Axes] = None
        self.im: Optional[Any] = None
        self.colorbar: Optional[Any] = None

        self.data_queue: queue.Queue = queue.Queue()
        self.current_data: Optional[np.ndarray] = None
        self.extent: Optional[Tuple[float, float, float, float]] = None

        self.is_running = False
        self.thread: Optional[threading.Thread] = None

    def create_figure(self, title: str = "2D Field",
                     xlabel: str = "X", ylabel: str = "Y",
                     cmap: str = "viridis"):
        """Create figure for 2D plotting"""
        self.fig, self.ax = plt.subplots(figsize=self.figsize)
        self.ax.set_title(title)
        self.ax.set_xlabel(xlabel)
        self.ax.set_ylabel(ylabel)
        self.cmap = cmap

        plt.ion()
        self.fig.show()

    def update_data(self, data: np.ndarray,
                   extent: Optional[Tuple[float, float, float, float]] = None):
        """
        Update 2D field data

        Args:
            data: 2D array (ny, nx)
            extent: Domain extent (xmin, xmax, ymin, ymax)
        """
        self.data_queue.put((data.copy(), extent))

    def _update_plot(self):
        """Update the plot display"""
        try:
            while not self.data_queue.empty():
                data, extent = self.data_queue.get_nowait()
                self.current_data = data
                if extent is not None:
                    self.extent = extent
        except queue.Empty:
            pass

        if self.current_data is None:
            return

        if self.im is None:
            # First plot
            self.im = self.ax.imshow(self.current_data,
                                    origin='lower',
                                    extent=self.extent,
                                    cmap=self.cmap,
                                    aspect='auto')
            self.colorbar = self.fig.colorbar(self.im, ax=self.ax)
        else:
            # Update existing plot
            self.im.set_data(self.current_data)
            self.im.set_clim(vmin=self.current_data.min(),
                           vmax=self.current_data.max())

        self.fig.canvas.draw()
        self.fig.canvas.flush_events()

    def start(self):
        """Start real-time plotting"""
        if self.is_running:
            return

        if self.fig is None:
            self.create_figure()

        self.is_running = True
        self.thread = threading.Thread(target=self._run_loop, daemon=True)
        self.thread.start()

    def _run_loop(self):
        """Main update loop"""
        while self.is_running:
            try:
                self._update_plot()
            except Exception as e:
                print(f"Plot update error: {e}")

            time.sleep(self.update_interval)

    def stop(self):
        """Stop plotting"""
        self.is_running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        plt.ioff()

    def save_figure(self, filename: str, dpi: int = 150):
        """Save current figure to file"""
        if self.fig:
            self.fig.savefig(filename, dpi=dpi, bbox_inches='tight')

    def __enter__(self):
        """Context manager entry"""
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop()


class SimulationMonitor:
    """
    Combined simulation monitor with multiple plots
    """

    def __init__(self):
        """Initialize simulation monitor"""
        self.plotters: Dict[str, Any] = {}

    def add_timeseries(self, name: str, **kwargs):
        """Add timeseries plotter"""
        plotter = RealtimePlotter(**kwargs)
        self.plotters[name] = plotter
        return plotter

    def add_field2d(self, name: str, **kwargs):
        """Add 2D field plotter"""
        plotter = Realtime2DPlotter(**kwargs)
        self.plotters[name] = plotter
        return plotter

    def start_all(self):
        """Start all plotters"""
        for plotter in self.plotters.values():
            plotter.start()

    def stop_all(self):
        """Stop all plotters"""
        for plotter in self.plotters.values():
            plotter.stop()

    def __enter__(self):
        """Context manager entry"""
        self.start_all()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop_all()


# Example usage
if __name__ == "__main__":
    # Demo: Real-time sine wave plotting
    import math

    with RealtimePlotter(update_interval=0.05) as plotter:
        plotter.create_figure()
        plotter.add_line("sin", label="sin(t)", color='blue')
        plotter.add_line("cos", label="cos(t)", color='red')

        for i in range(200):
            t = i * 0.1
            plotter.update_data(t, {
                'sin': math.sin(t),
                'cos': math.cos(t)
            })
            time.sleep(0.05)

        time.sleep(2)  # Display for 2 seconds before closing

    print("Demo complete!")
