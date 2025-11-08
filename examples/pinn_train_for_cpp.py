#!/usr/bin/env python3
"""
Train PINN model and export for C++ usage

Priority D2.4: PINN C++ Integration
Version: 6.0.0-alpha5

This script trains a PINN model in Python and saves it in TorchScript format
for use in C++ applications.

Usage:
    python examples/pinn_train_for_cpp.py

Output:
    - diffusion_pinn_1d.pt (TorchScript model)
    - training_history.png (loss curves)
"""

import sys
sys.path.insert(0, 'python')

import numpy as np
import matplotlib.pyplot as plt
import torch
from koolab.ml import DiffusionPINN1D


def main():
    print("=" * 60)
    print("Train PINN for C++ Integration")
    print("=" * 60)

    # ========================================================================
    # 1. Problem Setup
    # ========================================================================

    print("\n1. Problem setup...")

    L = 1.0           # Domain length (m)
    D = 1.0e-9        # Diffusion coefficient (m²/s)
    T = 100.0         # Final time (s)

    # Initial condition: Gaussian pulse
    x0 = L / 2.0
    sigma = 0.1

    def u_initial(x):
        return np.exp(-(x - x0) ** 2 / (2 * sigma ** 2))

    print(f"   Domain: [0, {L}] m")
    print(f"   Diffusivity: {D} m²/s")
    print(f"   Time: [0, {T}] s")
    print(f"   IC: Gaussian at x={x0}, σ={sigma}")

    # ========================================================================
    # 2. Create PINN
    # ========================================================================

    print("\n2. Creating PINN...")

    pinn = DiffusionPINN1D(diffusivity=D)

    print(f"   Network: {pinn.layers}")
    print(f"   Parameters: {sum(p.numel() for p in pinn.parameters())}")

    # ========================================================================
    # 3. Generate Training Data
    # ========================================================================

    print("\n3. Generating training data...")

    # PDE collocation points
    n_pde = 1000
    x_pde = np.random.uniform(0, L, (n_pde, 1))
    t_pde = np.random.uniform(0, T, (n_pde, 1))

    # Initial condition points
    n_ic = 100
    x_ic = np.linspace(0, L, n_ic).reshape(-1, 1)
    t_ic = np.zeros((n_ic, 1))
    u_ic = u_initial(x_ic)

    # Boundary condition points (u = 0 at boundaries)
    n_bc = 50
    x_bc_left = np.zeros((n_bc, 1))
    x_bc_right = np.ones((n_bc, 1)) * L
    t_bc = np.linspace(0, T, n_bc).reshape(-1, 1)
    u_bc_left = np.zeros((n_bc, 1))
    u_bc_right = np.zeros((n_bc, 1))

    x_bc = np.vstack([x_bc_left, x_bc_right])
    t_bc_all = np.vstack([t_bc, t_bc])
    u_bc = np.vstack([u_bc_left, u_bc_right])

    print(f"   PDE points: {n_pde}")
    print(f"   IC points: {n_ic}")
    print(f"   BC points: {len(x_bc)}")

    # ========================================================================
    # 4. Train PINN
    # ========================================================================

    print("\n4. Training PINN...")
    print("   (This may take several minutes...)")

    history = pinn.train_model(
        x_pde=x_pde, t_pde=t_pde,
        x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
        x_bc=x_bc, t_bc=t_bc_all, u_bc=u_bc,
        epochs=5000,
        lr=1e-3,
        verbose=1000
    )

    print(f"\n   Final loss: {history['loss_history'][-1]:.6e}")

    # ========================================================================
    # 5. Export to TorchScript
    # ========================================================================

    print("\n5. Exporting to TorchScript...")

    # Set to evaluation mode
    pinn.eval()

    # Create example input
    example_input = torch.randn(1, 2)  # (x, t)

    # Trace the model
    try:
        # Use torch.jit.trace for better compatibility
        traced_model = torch.jit.trace(pinn, example_input)

        # Save
        output_path = 'diffusion_pinn_1d.pt'
        traced_model.save(output_path)

        print(f"   ✓ Model saved to: {output_path}")

        # Verify the saved model
        loaded_model = torch.jit.load(output_path)
        test_output = loaded_model(example_input)
        print(f"   ✓ Model verification successful")
        print(f"   Output shape: {test_output.shape}")

    except Exception as e:
        print(f"   ✗ Error during export: {e}")
        print("\n   Trying alternative method (torch.jit.script)...")

        try:
            # Try scripting instead
            scripted_model = torch.jit.script(pinn)
            output_path = 'diffusion_pinn_1d.pt'
            scripted_model.save(output_path)
            print(f"   ✓ Model saved to: {output_path} (using script)")

        except Exception as e2:
            print(f"   ✗ Script export also failed: {e2}")
            return

    # ========================================================================
    # 6. Visualize Training History
    # ========================================================================

    print("\n6. Visualizing training history...")

    plt.figure(figsize=(10, 6))
    plt.semilogy(history['loss_history'], 'b-', linewidth=2, label='Total Loss')
    plt.xlabel('Epoch', fontsize=12)
    plt.ylabel('Loss', fontsize=12)
    plt.title('PINN Training History', fontsize=14)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig('training_history.png', dpi=150)
    print("   ✓ Saved training history to: training_history.png")

    # ========================================================================
    # 7. Test Predictions
    # ========================================================================

    print("\n7. Testing predictions...")

    pinn.eval()
    with torch.no_grad():
        # Predict at several times
        x_test = np.linspace(0, L, 100).reshape(-1, 1)
        times = [0, 25, 50, 75, 100]

        plt.figure(figsize=(12, 8))
        for i, t in enumerate(times):
            t_test = np.ones((100, 1)) * t
            u_pred = pinn.predict(x_test, t_test)

            plt.subplot(2, 3, i + 1)
            plt.plot(x_test, u_pred, 'b-', linewidth=2)
            plt.xlabel('x (m)')
            plt.ylabel('u')
            plt.title(f't = {t} s')
            plt.grid(True, alpha=0.3)

        plt.tight_layout()
        plt.savefig('pinn_predictions.png', dpi=150)
        print("   ✓ Saved predictions to: pinn_predictions.png")

    # ========================================================================
    # Summary
    # ========================================================================

    print("\n" + "=" * 60)
    print("Summary")
    print("=" * 60)
    print("✓ PINN trained successfully")
    print("✓ Model exported to TorchScript format")
    print("✓ Training history saved")
    print("✓ Predictions visualized")
    print("\nNext steps:")
    print("1. Compile C++ example:")
    print("   mkdir build && cd build")
    print("   cmake -DCMAKE_PREFIX_PATH=/path/to/libtorch ..")
    print("   make pinn_cpp_example")
    print("\n2. Run C++ example:")
    print("   ./pinn_cpp_example ../diffusion_pinn_1d.pt")
    print("=" * 60)


if __name__ == '__main__':
    main()
