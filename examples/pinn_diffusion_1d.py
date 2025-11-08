#!/usr/bin/env python3
"""
1D Diffusion PINN Example

Priority D2.2: PINN Solver Implementation
Version: 6.0.0-alpha5

Solves 1D diffusion equation using Physics-Informed Neural Network:
    ∂u/∂t = D ∂²u/∂x²

Initial condition: Gaussian pulse
Boundary conditions: u(0,t) = u(L,t) = 0

Usage:
    python examples/pinn_diffusion_1d.py
"""

import sys
sys.path.insert(0, 'python')

import numpy as np
import matplotlib.pyplot as plt
from koolab.ml import DiffusionPINN1D


def main():
    print("=" * 60)
    print("1D Diffusion PINN Example")
    print("=" * 60)

    # Problem setup
    L = 1.0           # Domain length (m)
    D = 1.0e-9        # Diffusion coefficient (m²/s)
    T = 100.0         # Final time (s)

    # Initial condition: Gaussian pulse
    x0 = L / 2.0
    sigma = 0.1

    def u_initial(x):
        return np.exp(-(x - x0) ** 2 / (2 * sigma ** 2))

    # Create PINN
    print("\nCreating PINN...")
    pinn = DiffusionPINN1D(diffusivity=D)

    # Training data

    # PDE collocation points (random sampling in domain)
    n_pde = 1000
    x_pde = np.random.uniform(0, L, (n_pde, 1))
    t_pde = np.random.uniform(0, T, (n_pde, 1))

    # Initial condition points
    n_ic = 100
    x_ic = np.linspace(0, L, n_ic).reshape(-1, 1)
    t_ic = np.zeros((n_ic, 1))
    u_ic = u_initial(x_ic)

    # Boundary condition points
    n_bc = 50
    x_bc_left = np.zeros((n_bc, 1))
    x_bc_right = np.ones((n_bc, 1)) * L
    t_bc = np.linspace(0, T, n_bc).reshape(-1, 1)
    u_bc_left = np.zeros((n_bc, 1))
    u_bc_right = np.zeros((n_bc, 1))

    x_bc = np.vstack([x_bc_left, x_bc_right])
    t_bc_all = np.vstack([t_bc, t_bc])
    u_bc = np.vstack([u_bc_left, u_bc_right])

    # Train PINN
    print("\nTraining PINN...")
    print(f"PDE points: {n_pde}")
    print(f"IC points: {n_ic}")
    print(f"BC points: {len(x_bc)}")

    history = pinn.train_model(
        x_pde=x_pde, t_pde=t_pde,
        x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
        x_bc=x_bc, t_bc=t_bc_all, u_bc=u_bc,
        epochs=5000,
        lr=1e-3,
        verbose=1000
    )

    # Predict solution at different times
    print("\nGenerating predictions...")
    n_test = 100
    x_test = np.linspace(0, L, n_test)
    times = [0, 25, 50, 75, 100]

    plt.figure(figsize=(12, 8))

    for i, t in enumerate(times):
        t_test = np.ones((n_test, 1)) * t
        u_pred = pinn.predict(x_test.reshape(-1, 1), t_test)

        plt.subplot(2, 3, i + 1)
        plt.plot(x_test, u_pred, 'b-', linewidth=2, label='PINN')
        plt.xlabel('x (m)')
        plt.ylabel('u')
        plt.title(f't = {t} s')
        plt.grid(True)
        plt.legend()

    # Loss history
    plt.subplot(2, 3, 6)
    plt.semilogy(history['loss_history'], 'b-', linewidth=2)
    plt.xlabel('Epoch')
    plt.ylabel('Loss')
    plt.title('Training Loss')
    plt.grid(True)

    plt.tight_layout()
    plt.savefig('pinn_diffusion_1d.png', dpi=150)
    print("\nPlot saved to: pinn_diffusion_1d.png")

    # Save model
    pinn.save_model('pinn_diffusion_1d.pth')
    print("Model saved to: pinn_diffusion_1d.pth")

    print("\n" + "=" * 60)
    print("PINN training complete!")
    print("=" * 60)


if __name__ == '__main__':
    main()
