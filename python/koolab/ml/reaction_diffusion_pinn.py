"""
Reaction-Diffusion PINN solver

Priority D2.2: PINN Solver Implementation
Version: 6.0.0-alpha5

Solves reaction-diffusion systems:
    ∂u/∂t = D_u ∇²u + f(u,v)
    ∂v/∂t = D_v ∇²v + g(u,v)

Supports:
- Gray-Scott model
- Brusselator model
- FitzHugh-Nagumo model
"""

import torch
import torch.autograd as autograd
import numpy as np
from typing import Callable, Optional
from .pinn import PINN


class ReactionDiffusionPINN(PINN):
    """
    Reaction-Diffusion PINN for coupled systems

    PDE system:
        ∂u/∂t = D_u ∂²u/∂x² + f(u,v)
        ∂v/∂t = D_v ∂²v/∂x² + g(u,v)

    Network input: (x, t)
    Network output: (u, v)
    """

    def __init__(self,
                 Du: float,
                 Dv: float,
                 reaction_u: Callable,
                 reaction_v: Callable,
                 layers: Optional[list] = None):
        """
        Initialize Reaction-Diffusion PINN

        Args:
            Du: Diffusion coefficient for u
            Dv: Diffusion coefficient for v
            reaction_u: Reaction term f(u,v)
            reaction_v: Reaction term g(u,v)
            layers: Network architecture, default [2, 64, 64, 64, 2]
        """
        if layers is None:
            layers = [2, 64, 64, 64, 2]  # (x, t) -> (u, v)

        super().__init__(layers, activation='tanh')

        self.Du = Du
        self.Dv = Dv
        self.reaction_u = reaction_u
        self.reaction_v = reaction_v

        # Loss weights
        self.lambda_pde = 1.0
        self.lambda_ic = 10.0
        self.lambda_bc = 10.0

    def physics_loss(self, x_pde: torch.Tensor, t_pde: torch.Tensor) -> torch.Tensor:
        """
        Compute PDE residual for both species

        Residuals:
            R_u = ∂u/∂t - D_u ∂²u/∂x² - f(u,v)
            R_v = ∂v/∂t - D_v ∂²v/∂x² - g(u,v)

        Args:
            x_pde: Spatial collocation points [N, 1]
            t_pde: Temporal collocation points [N, 1]

        Returns:
            Mean squared residual (both species)
        """
        # Concatenate inputs
        inputs = torch.cat([x_pde, t_pde], dim=1)

        # Predict u and v
        output = self.forward(inputs)
        u = output[:, 0:1]  # First output
        v = output[:, 1:2]  # Second output

        # First derivatives
        u_t = autograd.grad(u, t_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        v_t = autograd.grad(v, t_pde, grad_outputs=torch.ones_like(v),
                           create_graph=True)[0]

        u_x = autograd.grad(u, x_pde, grad_outputs=torch.ones_like(u),
                           create_graph=True)[0]
        v_x = autograd.grad(v, x_pde, grad_outputs=torch.ones_like(v),
                           create_graph=True)[0]

        # Second derivatives
        u_xx = autograd.grad(u_x, x_pde, grad_outputs=torch.ones_like(u_x),
                            create_graph=True)[0]
        v_xx = autograd.grad(v_x, x_pde, grad_outputs=torch::ones_like(v_x),
                            create_graph=True)[0]

        # Reaction terms (apply elementwise)
        reaction_u_vals = torch.zeros_like(u)
        reaction_v_vals = torch.zeros_like(v)

        for i in range(u.shape[0]):
            u_val = u[i, 0].item()
            v_val = v[i, 0].item()
            reaction_u_vals[i, 0] = self.reaction_u(u_val, v_val)
            reaction_v_vals[i, 0] = self.reaction_v(u_val, v_val)

        # PDE residuals
        residual_u = u_t - self.Du * u_xx - reaction_u_vals
        residual_v = v_t - self.Dv * v_xx - reaction_v_vals

        return torch.mean(residual_u ** 2 + residual_v ** 2)


class GrayScottPINN(ReactionDiffusionPINN):
    """
    Gray-Scott reaction-diffusion model

    Reactions:
        u + 2v → 3v  (rate k)
        v → P        (rate F)

    PDEs:
        ∂u/∂t = D_u ∂²u/∂x² - uv² + F(1-u)
        ∂v/∂t = D_v ∂²v/∂x² + uv² - (F+k)v

    Parameters for patterns:
        Spots:    F=0.055, k=0.062
        Stripes:  F=0.035, k=0.065
        Spirals:  F=0.018, k=0.051
    """

    def __init__(self, Du: float, Dv: float, F: float, k: float,
                 layers: Optional[list] = None):
        """
        Initialize Gray-Scott PINN

        Args:
            Du, Dv: Diffusion coefficients
            F: Feed rate
            k: Kill rate
            layers: Network architecture
        """
        # Define reaction terms
        def reaction_u(u, v):
            return -u * v * v + F * (1.0 - u)

        def reaction_v(u, v):
            return u * v * v - (F + k) * v

        super().__init__(Du, Dv, reaction_u, reaction_v, layers)

        self.F = F
        self.k = k


class BrusselatorPINN(ReactionDiffusionPINN):
    """
    Brusselator reaction-diffusion model

    PDEs:
        ∂u/∂t = D_u ∂²u/∂x² + a - (b+1)u + u²v
        ∂v/∂t = D_v ∂²v/∂x² + bu - u²v
    """

    def __init__(self, Du: float, Dv: float, a: float, b: float,
                 layers: Optional[list] = None):
        """
        Initialize Brusselator PINN

        Args:
            Du, Dv: Diffusion coefficients
            a, b: Reaction parameters
            layers: Network architecture
        """
        def reaction_u(u, v):
            return a - (b + 1.0) * u + u * u * v

        def reaction_v(u, v):
            return b * u - u * u * v

        super().__init__(Du, Dv, reaction_u, reaction_v, layers)

        self.a = a
        self.b = b
