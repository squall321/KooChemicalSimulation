"""
Diffusion equation PINN solver

Priority D2.2: PINN Solver Implementation
Version: 6.0.0-alpha5

Solves 1D/2D diffusion equation:
    ∂u/∂t = D ∇²u

Using physics-informed neural networks
"""

import torch
import torch.autograd as autograd
import numpy as np
from typing import Optional
from .pinn import PINN


class DiffusionPINN1D(PINN):
    """
    1D Diffusion PINN

    PDE: ∂u/∂t = D ∂²u/∂x²

    Network input: (x, t)
    Network output: u(x, t)

    Example:
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)
        pinn.train_model(x_pde, t_pde, x_ic, t_ic, u_ic)
        u_pred = pinn.predict(x_test, t_test)
    """

    def __init__(self, diffusivity: float, layers: Optional[list] = None):
        """
        Initialize 1D Diffusion PINN

        Args:
            diffusivity: Diffusion coefficient D (m²/s)
            layers: Network architecture, default [2, 50, 50, 50, 1]
        """
        if layers is None:
            layers = [2, 50, 50, 50, 1]  # (x, t) -> u

        super().__init__(layers, activation='tanh')

        self.D = diffusivity

        # Loss weights (tune these for better training)
        self.lambda_pde = 1.0
        self.lambda_ic = 10.0
        self.lambda_bc = 10.0

    def physics_loss(self, x_pde: torch.Tensor, t_pde: torch.Tensor) -> torch.Tensor:
        """
        Compute PDE residual: ∂u/∂t - D ∂²u/∂x²

        Uses automatic differentiation to compute derivatives

        Args:
            x_pde: Spatial collocation points [N, 1]
            t_pde: Temporal collocation points [N, 1]

        Returns:
            Mean squared PDE residual
        """
        # Concatenate inputs
        inputs = torch.cat([x_pde, t_pde], dim=1)

        # Predict u
        u = self.forward(inputs)

        # First derivatives
        u_t = autograd.grad(u, t_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        u_x = autograd.grad(u, x_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]

        # Second derivative
        u_xx = autograd.grad(u_x, x_pde, grad_outputs=torch.ones_like(u_x),
                            create_graph=True)[0]

        # PDE residual: ∂u/∂t - D ∂²u/∂x²
        residual = u_t - self.D * u_xx

        return torch.mean(residual ** 2)


class DiffusionPINN2D(PINN):
    """
    2D Diffusion PINN

    PDE: ∂u/∂t = D (∂²u/∂x² + ∂²u/∂y²)

    Network input: (x, y, t)
    Network output: u(x, y, t)
    """

    def __init__(self, diffusivity: float, layers: Optional[list] = None):
        """
        Initialize 2D Diffusion PINN

        Args:
            diffusivity: Diffusion coefficient D (m²/s)
            layers: Network architecture, default [3, 64, 64, 64, 1]
        """
        if layers is None:
            layers = [3, 64, 64, 64, 1]  # (x, y, t) -> u

        super().__init__(layers, activation='tanh')

        self.D = diffusivity

        # Loss weights
        self.lambda_pde = 1.0
        self.lambda_ic = 10.0
        self.lambda_bc = 10.0

    def physics_loss(self, x_pde: torch.Tensor, y_pde: torch.Tensor,
                     t_pde: torch.Tensor) -> torch.Tensor:
        """
        Compute PDE residual: ∂u/∂t - D (∂²u/∂x² + ∂²u/∂y²)

        Args:
            x_pde: X collocation points [N, 1]
            y_pde: Y collocation points [N, 1]
            t_pde: T collocation points [N, 1]

        Returns:
            Mean squared PDE residual
        """
        # Concatenate inputs
        inputs = torch.cat([x_pde, y_pde, t_pde], dim=1)

        # Predict u
        u = self.forward(inputs)

        # First derivatives
        u_t = autograd.grad(u, t_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        u_x = autograd.grad(u, x_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        u_y = autograd.grad(u, y_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]

        # Second derivatives
        u_xx = autograd.grad(u_x, x_pde, grad_outputs=torch.ones_like(u_x),
                            create_graph=True)[0]
        u_yy = autograd.grad(u_y, y_pde, grad_outputs=torch.ones_like(u_y),
                            create_graph=True)[0]

        # PDE residual
        residual = u_t - self.D * (u_xx + u_yy)

        return torch.mean(residual ** 2)

    def train_model_2d(self,
                       x_pde: np.ndarray, y_pde: np.ndarray, t_pde: np.ndarray,
                       x_ic: Optional[np.ndarray] = None,
                       y_ic: Optional[np.ndarray] = None,
                       t_ic: Optional[np.ndarray] = None,
                       u_ic: Optional[np.ndarray] = None,
                       x_bc: Optional[np.ndarray] = None,
                       y_bc: Optional[np.ndarray] = None,
                       t_bc: Optional[np.ndarray] = None,
                       u_bc: Optional[np.ndarray] = None,
                       epochs: int = 10000,
                       lr: float = 1e-3,
                       verbose: int = 1000) -> dict:
        """
        Train 2D diffusion PINN

        Args:
            x_pde, y_pde, t_pde: PDE collocation points
            x_ic, y_ic, t_ic, u_ic: Initial condition
            x_bc, y_bc, t_bc, u_bc: Boundary condition
            epochs: Training epochs
            lr: Learning rate
            verbose: Print interval

        Returns:
            Training history
        """
        # Convert to tensors
        def to_tensor(arr):
            if arr is None:
                return None
            return torch.tensor(arr, dtype=torch.float32, requires_grad=True)

        x_pde_t = to_tensor(x_pde)
        y_pde_t = to_tensor(y_pde)
        t_pde_t = to_tensor(t_pde)
        x_ic_t = to_tensor(x_ic)
        y_ic_t = to_tensor(y_ic)
        t_ic_t = to_tensor(t_ic)
        u_ic_t = to_tensor(u_ic)
        x_bc_t = to_tensor(x_bc)
        y_bc_t = to_tensor(y_bc)
        t_bc_t = to_tensor(t_bc)
        u_bc_t = to_tensor(u_bc)

        # Optimizer
        optimizer = torch.optim.Adam(self.parameters(), lr=lr)

        # Training loop
        import time
        start_time = time.time()

        for epoch in range(epochs):
            optimizer.zero_grad()

            # Total loss
            loss = torch.tensor(0.0)

            # Physics loss
            if x_pde_t is not None:
                loss_pde = self.physics_loss(x_pde_t, y_pde_t, t_pde_t)
                loss += self.lambda_pde * loss_pde

            # Initial condition loss
            if x_ic_t is not None:
                inputs_ic = torch.cat([x_ic_t, y_ic_t, t_ic_t], dim=1)
                u_pred_ic = self.forward(inputs_ic)
                loss_ic = torch.mean((u_pred_ic - u_ic_t) ** 2)
                loss += self.lambda_ic * loss_ic

            # Boundary condition loss
            if x_bc_t is not None:
                inputs_bc = torch.cat([x_bc_t, y_bc_t, t_bc_t], dim=1)
                u_pred_bc = self.forward(inputs_bc)
                loss_bc = torch.mean((u_pred_bc - u_bc_t) ** 2)
                loss += self.lambda_bc * loss_bc

            loss.backward()
            optimizer.step()

            # Store history
            self.loss_history.append(loss.item())

            # Print progress
            if verbose > 0 and (epoch % verbose == 0 or epoch == epochs - 1):
                elapsed = time.time() - start_time
                print(f"Epoch {epoch}/{epochs}, Loss: {loss.item():.6e}, "
                      f"Time: {elapsed:.2f}s")

        total_time = time.time() - start_time
        print(f"\nTraining complete! Total time: {total_time:.2f}s")

        return {
            'loss_history': self.loss_history,
            'total_time': total_time
        }

    def predict_2d(self, x: np.ndarray, y: np.ndarray, t: np.ndarray) -> np.ndarray:
        """
        Predict 2D solution

        Args:
            x, y, t: Coordinates

        Returns:
            Predicted solution
        """
        x_t = torch.tensor(x, dtype=torch.float32)
        y_t = torch.tensor(y, dtype=torch.float32)
        t_t = torch.tensor(t, dtype=torch.float32)
        inputs = torch.cat([x_t, y_t, t_t], dim=1)

        with torch.no_grad():
            u_pred = self.forward(inputs)

        return u_pred.numpy()
