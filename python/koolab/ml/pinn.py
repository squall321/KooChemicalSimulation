"""
Base PINN (Physics-Informed Neural Network) class

Priority D2.1/D2.2: PINN Solver Implementation
Version: 6.0.0-alpha5
Author: KooChemicalSimulation Development Team

Physics-Informed Neural Networks combine:
1. Data fitting (boundary/initial conditions)
2. Physics constraints (PDE residuals)
3. Automatic differentiation for derivatives
"""

import torch
import torch.nn as nn
import numpy as np
from typing import Callable, Optional, Tuple, List
import time


class PINN(nn.Module):
    """
    Base Physics-Informed Neural Network

    Architecture:
    - Fully connected feedforward network
    - Automatic differentiation for PDE residuals
    - Multi-term loss function (data + physics + BC/IC)

    Usage:
        pinn = PINN(layers=[2, 50, 50, 50, 1])
        pinn.train_model(x_data, t_data, u_data, x_pde, t_pde)
    """

    def __init__(self, layers: List[int], activation: str = 'tanh'):
        """
        Initialize PINN

        Args:
            layers: List of layer sizes, e.g., [2, 50, 50, 1]
                   First: input dimension
                   Last: output dimension
                   Middle: hidden layers
            activation: Activation function ('tanh', 'relu', 'sigmoid')
        """
        super(PINN, self).__init__()

        self.layers = layers
        self.depth = len(layers) - 1

        # Build network
        self.network = nn.ModuleList()
        for i in range(self.depth):
            self.network.append(nn.Linear(layers[i], layers[i+1]))

        # Activation function
        if activation == 'tanh':
            self.activation = torch.tanh
        elif activation == 'relu':
            self.activation = torch.relu
        elif activation == 'sigmoid':
            self.activation = torch.sigmoid
        else:
            raise ValueError(f"Unknown activation: {activation}")

        # Xavier initialization
        self.init_weights()

        # Loss weights
        self.lambda_data = 1.0
        self.lambda_pde = 1.0
        self.lambda_bc = 1.0
        self.lambda_ic = 1.0

        # Training history
        self.loss_history = []
        self.loss_data_history = []
        self.loss_pde_history = []

    def init_weights(self):
        """Xavier uniform initialization"""
        for layer in self.network:
            nn.init.xavier_uniform_(layer.weight)
            nn.init.zeros_(layer.bias)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Forward pass through network

        Args:
            x: Input tensor [batch, input_dim]

        Returns:
            Output tensor [batch, output_dim]
        """
        for i in range(self.depth - 1):
            x = self.network[i](x)
            x = self.activation(x)

        # Last layer (no activation)
        x = self.network[-1](x)

        return x

    def physics_loss(self, x_pde: torch.Tensor, t_pde: torch.Tensor) -> torch.Tensor:
        """
        Compute physics loss (PDE residual)

        This should be overridden by subclasses for specific PDEs

        Args:
            x_pde: Spatial collocation points
            t_pde: Temporal collocation points

        Returns:
            PDE residual loss (scalar)
        """
        raise NotImplementedError("Subclasses must implement physics_loss()")

    def data_loss(self, x_data: torch.Tensor, t_data: torch.Tensor,
                  u_data: torch.Tensor) -> torch.Tensor:
        """
        Compute data fitting loss

        Args:
            x_data: Spatial data points
            t_data: Temporal data points
            u_data: Solution values

        Returns:
            MSE loss
        """
        inputs = torch.cat([x_data, t_data], dim=1)
        u_pred = self.forward(inputs)

        return torch.mean((u_pred - u_data) ** 2)

    def boundary_loss(self, x_bc: torch.Tensor, t_bc: torch.Tensor,
                      u_bc: torch.Tensor) -> torch.Tensor:
        """
        Compute boundary condition loss

        Args:
            x_bc: Boundary spatial points
            t_bc: Boundary temporal points
            u_bc: Boundary values

        Returns:
            BC loss
        """
        inputs = torch.cat([x_bc, t_bc], dim=1)
        u_pred = self.forward(inputs)

        return torch.mean((u_pred - u_bc) ** 2)

    def initial_loss(self, x_ic: torch.Tensor, t_ic: torch.Tensor,
                     u_ic: torch.Tensor) -> torch.Tensor:
        """
        Compute initial condition loss

        Args:
            x_ic: Initial spatial points
            t_ic: Initial temporal points (usually all t=0)
            u_ic: Initial values

        Returns:
            IC loss
        """
        inputs = torch.cat([x_ic, t_ic], dim=1)
        u_pred = self.forward(inputs)

        return torch.mean((u_pred - u_ic) ** 2)

    def total_loss(self, x_data: Optional[torch.Tensor] = None,
                   t_data: Optional[torch.Tensor] = None,
                   u_data: Optional[torch.Tensor] = None,
                   x_pde: Optional[torch.Tensor] = None,
                   t_pde: Optional[torch.Tensor] = None,
                   x_bc: Optional[torch.Tensor] = None,
                   t_bc: Optional[torch.Tensor] = None,
                   u_bc: Optional[torch.Tensor] = None,
                   x_ic: Optional[torch.Tensor] = None,
                   t_ic: Optional[torch.Tensor] = None,
                   u_ic: Optional[torch.Tensor] = None) -> Tuple[torch.Tensor, dict]:
        """
        Compute total weighted loss

        Returns:
            Total loss and dictionary of individual losses
        """
        loss = torch.tensor(0.0)
        losses = {}

        # Data loss
        if x_data is not None and u_data is not None:
            loss_data = self.data_loss(x_data, t_data, u_data)
            loss += self.lambda_data * loss_data
            losses['data'] = loss_data.item()

        # Physics loss
        if x_pde is not None and t_pde is not None:
            loss_pde = self.physics_loss(x_pde, t_pde)
            loss += self.lambda_pde * loss_pde
            losses['pde'] = loss_pde.item()

        # Boundary loss
        if x_bc is not None and u_bc is not None:
            loss_bc = self.boundary_loss(x_bc, t_bc, u_bc)
            loss += self.lambda_bc * loss_bc
            losses['bc'] = loss_bc.item()

        # Initial loss
        if x_ic is not None and u_ic is not None:
            loss_ic = self.initial_loss(x_ic, t_ic, u_ic)
            loss += self.lambda_ic * loss_ic
            losses['ic'] = loss_ic.item()

        losses['total'] = loss.item()

        return loss, losses

    def train_model(self,
                    x_pde: np.ndarray,
                    t_pde: np.ndarray,
                    x_ic: Optional[np.ndarray] = None,
                    t_ic: Optional[np.ndarray] = None,
                    u_ic: Optional[np.ndarray] = None,
                    x_bc: Optional[np.ndarray] = None,
                    t_bc: Optional[np.ndarray] = None,
                    u_bc: Optional[np.ndarray] = None,
                    x_data: Optional[np.ndarray] = None,
                    t_data: Optional[np.ndarray] = None,
                    u_data: Optional[np.ndarray] = None,
                    epochs: int = 10000,
                    lr: float = 1e-3,
                    verbose: int = 1000,
                    optimizer_type: str = 'adam') -> dict:
        """
        Train the PINN

        Args:
            x_pde, t_pde: PDE collocation points
            x_ic, t_ic, u_ic: Initial condition data
            x_bc, t_bc, u_bc: Boundary condition data
            x_data, t_data, u_data: Additional training data
            epochs: Number of training epochs
            lr: Learning rate
            verbose: Print interval (0 = no printing)
            optimizer_type: 'adam' or 'lbfgs'

        Returns:
            Training history dictionary
        """
        # Convert to tensors
        def to_tensor(arr):
            if arr is None:
                return None
            return torch.tensor(arr, dtype=torch.float32, requires_grad=True)

        x_pde_t = to_tensor(x_pde)
        t_pde_t = to_tensor(t_pde)
        x_ic_t = to_tensor(x_ic)
        t_ic_t = to_tensor(t_ic)
        u_ic_t = to_tensor(u_ic)
        x_bc_t = to_tensor(x_bc)
        t_bc_t = to_tensor(t_bc)
        u_bc_t = to_tensor(u_bc)
        x_data_t = to_tensor(x_data)
        t_data_t = to_tensor(t_data)
        u_data_t = to_tensor(u_data)

        # Optimizer
        if optimizer_type == 'adam':
            optimizer = torch.optim.Adam(self.parameters(), lr=lr)
        elif optimizer_type == 'lbfgs':
            optimizer = torch.optim.LBFGS(self.parameters(), lr=lr,
                                         max_iter=20, line_search_fn='strong_wolfe')
        else:
            raise ValueError(f"Unknown optimizer: {optimizer_type}")

        # Training loop
        start_time = time.time()

        for epoch in range(epochs):
            def closure():
                optimizer.zero_grad()
                loss, losses = self.total_loss(
                    x_data=x_data_t, t_data=t_data_t, u_data=u_data_t,
                    x_pde=x_pde_t, t_pde=t_pde_t,
                    x_bc=x_bc_t, t_bc=t_bc_t, u_bc=u_bc_t,
                    x_ic=x_ic_t, t_ic=t_ic_t, u_ic=u_ic_t
                )
                loss.backward()

                # Store history
                self.loss_history.append(losses['total'])
                if 'data' in losses:
                    self.loss_data_history.append(losses['data'])
                if 'pde' in losses:
                    self.loss_pde_history.append(losses['pde'])

                return loss

            loss = optimizer.step(closure)

            # Print progress
            if verbose > 0 and (epoch % verbose == 0 or epoch == epochs - 1):
                elapsed = time.time() - start_time
                print(f"Epoch {epoch}/{epochs}, Loss: {loss.item():.6e}, "
                      f"Time: {elapsed:.2f}s")

        total_time = time.time() - start_time
        print(f"\nTraining complete! Total time: {total_time:.2f}s")

        return {
            'loss_history': self.loss_history,
            'loss_data_history': self.loss_data_history,
            'loss_pde_history': self.loss_pde_history,
            'total_time': total_time
        }

    def predict(self, x: np.ndarray, t: np.ndarray) -> np.ndarray:
        """
        Predict solution at given points

        Args:
            x: Spatial coordinates
            t: Temporal coordinates

        Returns:
            Predicted solution values
        """
        x_t = torch.tensor(x, dtype=torch.float32)
        t_t = torch.tensor(t, dtype=torch.float32)
        inputs = torch.cat([x_t, t_t], dim=1)

        with torch.no_grad():
            u_pred = self.forward(inputs)

        return u_pred.numpy()

    def save_model(self, filepath: str):
        """Save model weights"""
        torch.save(self.state_dict(), filepath)

    def load_model(self, filepath: str):
        """Load model weights"""
        self.load_state_dict(torch.load(filepath))
