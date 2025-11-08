"""
Adaptive PINN with residual-based sampling

Priority D2.3: Advanced PINN Techniques
Version: 6.0.0-alpha5

Features:
- Adaptive collocation point sampling based on PDE residuals
- Importance sampling for efficient training
- Dynamic point redistribution
"""

import torch
import numpy as np
from typing import Tuple
from .diffusion_pinn import DiffusionPINN1D


class AdaptivePINN(DiffusionPINN1D):
    """
    Adaptive PINN with residual-based point sampling

    During training, adds more collocation points where PDE residual is large.
    This focuses computational effort on difficult regions.

    Algorithm:
        1. Compute PDE residual at all collocation points
        2. Sample new points with probability ∝ |residual|
        3. Add high-residual points to training set
        4. Continue training

    Example:
        pinn = AdaptivePINN(diffusivity=1.0e-9)
        pinn.train_adaptive(x_pde_init, t_pde_init, ...)
    """

    def __init__(self, diffusivity: float, layers=None):
        """
        Initialize Adaptive PINN

        Args:
            diffusivity: Diffusion coefficient
            layers: Network architecture
        """
        super().__init__(diffusivity, layers)

        self.adaptive_history = []

    def compute_residual_importance(self,
                                     x_pde: torch.Tensor,
                                     t_pde: torch.Tensor) -> np.ndarray:
        """
        Compute importance (absolute residual) at each point

        Args:
            x_pde, t_pde: Collocation points

        Returns:
            Importance values [N]
        """
        with torch.no_grad():
            inputs = torch.cat([x_pde, t_pde], dim=1)
            u = self.forward(inputs)

            # Compute residual at each point
            residuals = torch.zeros(x_pde.shape[0])

            for i in range(x_pde.shape[0]):
                x_i = x_pde[i:i+1]
                t_i = t_pde[i:i+1]

                # Need requires_grad for autodiff
                x_i.requires_grad = True
                t_i.requires_grad = True

                inputs_i = torch.cat([x_i, t_i], dim=1)
                u_i = self.forward(inputs_i)

                # Compute derivatives
                u_t = torch.autograd.grad(u_i, t_i, torch.ones_like(u_i),
                                          create_graph=True)[0]
                u_x = torch.autograd.grad(u_i, x_i, torch.ones_like(u_i),
                                          create_graph=True)[0]
                u_xx = torch.autograd.grad(u_x, x_i, torch.ones_like(u_x),
                                           create_graph=True)[0]

                # Residual
                res = u_t - self.D * u_xx
                residuals[i] = torch.abs(res).item()

        return residuals.numpy()

    def adaptive_resample(self,
                          x_pde: np.ndarray,
                          t_pde: np.ndarray,
                          n_new_points: int,
                          x_bounds: Tuple[float, float],
                          t_bounds: Tuple[float, float]) -> Tuple[np.ndarray, np.ndarray]:
        """
        Resample points based on residual importance

        Args:
            x_pde, t_pde: Current collocation points
            n_new_points: Number of new points to sample
            x_bounds: (x_min, x_max)
            t_bounds: (t_min, t_max)

        Returns:
            New (x, t) collocation points
        """
        # Convert to tensors
        x_t = torch.tensor(x_pde, dtype=torch.float32, requires_grad=True)
        t_t = torch.tensor(t_pde, dtype=torch.float32, requires_grad=True)

        # Compute importance
        importance = self.compute_residual_importance(x_t, t_t)

        # Normalize to probability distribution
        prob = importance / importance.sum()

        # Sample indices with replacement
        indices = np.random.choice(len(prob), size=n_new_points,
                                  p=prob, replace=True)

        # Add small noise around selected points
        x_new = x_pde[indices] + np.random.randn(n_new_points) * 0.01
        t_new = t_pde[indices] + np.random.randn(n_new_points) * 0.01

        # Clip to bounds
        x_new = np.clip(x_new, x_bounds[0], x_bounds[1])
        t_new = np.clip(t_new, t_bounds[0], t_bounds[1])

        return x_new.reshape(-1, 1), t_new.reshape(-1, 1)

    def train_adaptive(self,
                       x_pde_init: np.ndarray,
                       t_pde_init: np.ndarray,
                       x_bounds: Tuple[float, float],
                       t_bounds: Tuple[float, float],
                       x_ic: np.ndarray,
                       t_ic: np.ndarray,
                       u_ic: np.ndarray,
                       n_adaptive_iter: int = 5,
                       n_new_points_per_iter: int = 200,
                       epochs_per_iter: int = 2000,
                       lr: float = 1e-3,
                       verbose: int = 500) -> dict:
        """
        Train with adaptive sampling

        Args:
            x_pde_init, t_pde_init: Initial collocation points
            x_bounds, t_bounds: Domain bounds
            x_ic, t_ic, u_ic: Initial condition
            n_adaptive_iter: Number of adaptive iterations
            n_new_points_per_iter: Points to add each iteration
            epochs_per_iter: Training epochs per iteration
            lr: Learning rate
            verbose: Print interval

        Returns:
            Training history
        """
        x_pde = x_pde_init.copy()
        t_pde = t_pde_init.copy()

        print("Starting adaptive PINN training...")
        print(f"Initial points: {len(x_pde)}")

        for iter in range(n_adaptive_iter):
            print(f"\n=== Adaptive Iteration {iter+1}/{n_adaptive_iter} ===")
            print(f"Current collocation points: {len(x_pde)}")

            # Train with current points
            history = self.train_model(
                x_pde=x_pde, t_pde=t_pde,
                x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
                epochs=epochs_per_iter, lr=lr, verbose=verbose
            )

            # Adaptive resampling
            if iter < n_adaptive_iter - 1:
                x_new, t_new = self.adaptive_resample(
                    x_pde, t_pde, n_new_points_per_iter,
                    x_bounds, t_bounds
                )

                # Add new points
                x_pde = np.vstack([x_pde, x_new])
                t_pde = np.vstack([t_pde, t_new])

                # Store history
                self.adaptive_history.append({
                    'iteration': iter,
                    'n_points': len(x_pde),
                    'final_loss': history['loss_history'][-1]
                })

        print(f"\nAdaptive training complete!")
        print(f"Final collocation points: {len(x_pde)}")

        return {
            'adaptive_history': self.adaptive_history,
            'final_x_pde': x_pde,
            'final_t_pde': t_pde
        }
