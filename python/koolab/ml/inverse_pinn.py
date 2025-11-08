"""
Inverse PINN for parameter estimation

Priority D2.3: Advanced PINN Techniques
Version: 6.0.0-alpha5

Solves inverse problems: estimate unknown parameters from data

Example: Estimate diffusion coefficient D from concentration measurements
"""

import torch
import torch.nn as nn
import torch.autograd as autograd
import numpy as np
from typing import Optional
from .pinn import PINN


class InversePINN(PINN):
    """
    Inverse PINN for parameter estimation

    Estimates unknown PDE parameters from measurement data

    Example:
        ∂u/∂t = D ∂²u/∂x²
        Given: u(x, t) measurements
        Estimate: D

    The parameter D is a learnable variable (nn.Parameter)
    """

    def __init__(self, layers: Optional[list] = None,
                 initial_params: Optional[dict] = None):
        """
        Initialize Inverse PINN

        Args:
            layers: Network architecture
            initial_params: Initial parameter guesses, e.g. {'D': 1.0e-9}
        """
        if layers is None:
            layers = [2, 50, 50, 50, 1]

        super().__init__(layers, activation='tanh')

        # Learnable parameters (to be estimated)
        if initial_params is None:
            initial_params = {'D': 1.0e-9}

        self.params = nn.ParameterDict()
        for name, value in initial_params.items():
            # Log-transform for positive parameters
            self.params[name] = nn.Parameter(
                torch.tensor([np.log(value)], dtype=torch.float32)
            )

        self.param_history = {name: [] for name in initial_params.keys()}

        # Increase data loss weight for inverse problems
        self.lambda_data = 100.0
        self.lambda_pde = 1.0

    def get_parameter(self, name: str) -> float:
        """
        Get current parameter estimate

        Args:
            name: Parameter name

        Returns:
            Parameter value
        """
        return torch.exp(self.params[name]).item()

    def physics_loss(self, x_pde: torch.Tensor, t_pde: torch.Tensor) -> torch.Tensor:
        """
        Compute PDE residual with learnable D

        PDE: ∂u/∂t = D ∂²u/∂x²
        """
        # Get current D estimate
        D = torch.exp(self.params['D'])

        # Concatenate inputs
        inputs = torch.cat([x_pde, t_pde], dim=1)

        # Predict u
        u = self.forward(inputs)

        # Derivatives
        u_t = autograd.grad(u, t_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        u_x = autograd.grad(u, x_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        u_xx = autograd.grad(u_x, x_pde, grad_outputs=torch.ones_like(u_x),
                            create_graph=True)[0]

        # PDE residual
        residual = u_t - D * u_xx

        return torch.mean(residual ** 2)

    def train_inverse(self,
                      x_data: np.ndarray,
                      t_data: np.ndarray,
                      u_data: np.ndarray,
                      x_pde: np.ndarray,
                      t_pde: np.ndarray,
                      epochs: int = 10000,
                      lr: float = 1e-3,
                      verbose: int = 1000) -> dict:
        """
        Train inverse PINN to estimate parameters

        Args:
            x_data, t_data, u_data: Measurement data
            x_pde, t_pde: PDE collocation points
            epochs: Training epochs
            lr: Learning rate
            verbose: Print interval

        Returns:
            Training history with parameter estimates
        """
        # Convert to tensors
        x_data_t = torch.tensor(x_data, dtype=torch.float32)
        t_data_t = torch.tensor(t_data, dtype=torch.float32)
        u_data_t = torch.tensor(u_data, dtype=torch.float32)
        x_pde_t = torch.tensor(x_pde, dtype=torch.float32, requires_grad=True)
        t_pde_t = torch.tensor(t_pde, dtype=torch.float32, requires_grad=True)

        # Optimizer (optimize both network weights AND parameters)
        optimizer = torch.optim.Adam(self.parameters(), lr=lr)

        # Training loop
        import time
        start_time = time.time()

        for epoch in range(epochs):
            optimizer.zero_grad()

            # Data loss
            inputs_data = torch.cat([x_data_t, t_data_t], dim=1)
            u_pred = self.forward(inputs_data)
            loss_data = torch.mean((u_pred - u_data_t) ** 2)

            # Physics loss
            loss_pde = self.physics_loss(x_pde_t, t_pde_t)

            # Total loss
            loss = self.lambda_data * loss_data + self.lambda_pde * loss_pde

            loss.backward()
            optimizer.step()

            # Store history
            self.loss_history.append(loss.item())
            for name in self.params.keys():
                self.param_history[name].append(self.get_parameter(name))

            # Print progress
            if verbose > 0 and (epoch % verbose == 0 or epoch == epochs - 1):
                elapsed = time.time() - start_time
                param_str = ", ".join([f"{name}={self.get_parameter(name):.6e}"
                                      for name in self.params.keys()])
                print(f"Epoch {epoch}/{epochs}, Loss: {loss.item():.6e}, "
                      f"{param_str}, Time: {elapsed:.2f}s")

        total_time = time.time() - start_time

        # Final parameter estimates
        final_params = {name: self.get_parameter(name) for name in self.params.keys()}

        print(f"\nInverse problem solved! Total time: {total_time:.2f}s")
        print("Estimated parameters:")
        for name, value in final_params.items():
            print(f"  {name} = {value:.6e}")

        return {
            'loss_history': self.loss_history,
            'param_history': self.param_history,
            'final_params': final_params,
            'total_time': total_time
        }


class MultiParameterInversePINN(InversePINN):
    """
    Inverse PINN for multiple parameters

    Example: Estimate Du, Dv, F, k in Gray-Scott model
    """

    def __init__(self, layers: Optional[list] = None,
                 initial_params: Optional[dict] = None):
        """
        Initialize multi-parameter inverse PINN

        Args:
            layers: Network architecture (must output 2 for reaction-diffusion)
            initial_params: Dict of initial guesses
                           e.g. {'Du': 2e-5, 'Dv': 1e-5, 'F': 0.055, 'k': 0.062}
        """
        if layers is None:
            layers = [2, 64, 64, 64, 2]  # Output (u, v)

        super().__init__(layers, initial_params)

    def physics_loss(self, x_pde: torch.Tensor, t_pde: torch.Tensor) -> torch.Tensor:
        """
        Gray-Scott physics loss with learnable parameters
        """
        # Get parameter estimates
        Du = torch.exp(self.params['Du'])
        Dv = torch.exp(self.params['Dv'])
        F = torch.exp(self.params['F'])
        k = torch.exp(self.params['k'])

        # Concatenate inputs
        inputs = torch.cat([x_pde, t_pde], dim=1)

        # Predict u and v
        output = self.forward(inputs)
        u = output[:, 0:1]
        v = output[:, 1:2]

        # Derivatives
        u_t = autograd.grad(u, t_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        v_t = autograd.grad(v, t_pde, grad_outputs=torch.ones_like(v),
                           create_graph=True)[0]

        u_x = autograd.grad(u, x_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        v_x = autograd.grad(v, x_pde, grad_outputs=torch.ones_like(v),
                           create_graph=True)[0]

        u_xx = autograd.grad(u_x, x_pde, grad_outputs=torch.ones_like(u_x),
                            create_graph=True)[0]
        v_xx = autograd.grad(v_x, x_pde, grad_outputs=torch.ones_like(v_x),
                            create_graph=True)[0]

        # Gray-Scott reactions
        uvv = u * v * v
        react_u = -uvv + F * (1.0 - u)
        react_v = uvv - (F + k) * v

        # PDE residuals
        residual_u = u_t - Du * u_xx - react_u
        residual_v = v_t - Dv * v_xx - react_v

        return torch.mean(residual_u ** 2 + residual_v ** 2)
