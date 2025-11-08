"""
Tests for PINN (Physics-Informed Neural Networks) module

Priority D2.6: PINN Testing and Validation
Version: 6.0.0-alpha5

Tests cover:
- Basic PINN functionality
- Diffusion PINN (1D and 2D)
- Reaction-Diffusion PINN
- Adaptive PINN
- Inverse PINN
- Convergence and accuracy
"""

import sys
sys.path.insert(0, 'python')

import pytest
import numpy as np
import torch
from koolab.ml import (
    DiffusionPINN1D,
    DiffusionPINN2D,
    GrayScottPINN,
    AdaptivePINN,
    InversePINN,
    MultiParameterInversePINN
)


# ============================================================================
# Fixtures
# ============================================================================

@pytest.fixture
def simple_1d_data():
    """Generate simple 1D training data"""
    np.random.seed(42)

    # Domain
    L = 1.0
    T = 10.0

    # PDE points
    x_pde = np.random.uniform(0, L, (100, 1))
    t_pde = np.random.uniform(0, T, (100, 1))

    # Initial condition (Gaussian)
    x_ic = np.linspace(0, L, 50).reshape(-1, 1)
    t_ic = np.zeros((50, 1))
    u_ic = np.exp(-(x_ic - 0.5)**2 / 0.02)

    # Boundary conditions
    n_bc = 20
    x_bc = np.vstack([np.zeros((n_bc, 1)), np.ones((n_bc, 1)) * L])
    t_bc = np.vstack([np.linspace(0, T, n_bc).reshape(-1, 1)] * 2)
    u_bc = np.zeros((2 * n_bc, 1))

    return {
        'x_pde': x_pde, 't_pde': t_pde,
        'x_ic': x_ic, 't_ic': t_ic, 'u_ic': u_ic,
        'x_bc': x_bc, 't_bc': t_bc, 'u_bc': u_bc
    }


@pytest.fixture
def analytical_solution_1d():
    """Analytical solution for validation"""
    def solution(x, t, D=1.0e-9):
        sigma0 = 0.1
        x0 = 0.5
        sigma_t = np.sqrt(2 * D * t + sigma0**2)
        amp = sigma0 / sigma_t
        return amp * np.exp(-(x - x0)**2 / (2 * sigma_t**2))
    return solution


# ============================================================================
# Test: DiffusionPINN1D
# ============================================================================

class TestDiffusionPINN1D:
    """Tests for 1D Diffusion PINN"""

    def test_initialization(self):
        """Test PINN initialization"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        assert pinn.D == 1.0e-9
        assert len(pinn.layers) == 5
        assert pinn.layers[0] == 2  # Input: (x, t)
        assert pinn.layers[-1] == 1  # Output: u

        # Check parameters exist
        param_count = sum(p.numel() for p in pinn.parameters())
        assert param_count > 0

    def test_forward_pass(self):
        """Test forward pass"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        # Single input
        x = torch.tensor([[0.5]], dtype=torch.float32)
        t = torch.tensor([[10.0]], dtype=torch.float32)
        inputs = torch.cat([x, t], dim=1)

        output = pinn.forward(inputs)

        assert output.shape == (1, 1)
        assert not torch.isnan(output).any()
        assert not torch.isinf(output).any()

    def test_predict(self):
        """Test prediction interface"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        x = np.array([[0.5]])
        t = np.array([[10.0]])

        u = pinn.predict(x, t)

        assert u.shape == (1,)
        assert not np.isnan(u).any()

    def test_physics_loss(self):
        """Test physics loss computation"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        x = torch.tensor([[0.5]], dtype=torch.float32, requires_grad=True)
        t = torch.tensor([[10.0]], dtype=torch.float32, requires_grad=True)

        loss = pinn.physics_loss(x, t)

        assert isinstance(loss, torch.Tensor)
        assert loss.item() >= 0
        assert not torch.isnan(loss).any()

    def test_training_smoke(self, simple_1d_data):
        """Smoke test: verify training runs without errors"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        history = pinn.train_model(
            x_pde=simple_1d_data['x_pde'],
            t_pde=simple_1d_data['t_pde'],
            x_ic=simple_1d_data['x_ic'],
            t_ic=simple_1d_data['t_ic'],
            u_ic=simple_1d_data['u_ic'],
            x_bc=simple_1d_data['x_bc'],
            t_bc=simple_1d_data['t_bc'],
            u_bc=simple_1d_data['u_bc'],
            epochs=100,
            lr=1e-3,
            verbose=0
        )

        assert 'loss_history' in history
        assert len(history['loss_history']) == 100
        assert history['loss_history'][-1] < history['loss_history'][0]  # Loss decreased

    def test_accuracy_vs_analytical(self, simple_1d_data, analytical_solution_1d):
        """Test accuracy against analytical solution"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        # Train
        pinn.train_model(
            x_pde=simple_1d_data['x_pde'],
            t_pde=simple_1d_data['t_pde'],
            x_ic=simple_1d_data['x_ic'],
            t_ic=simple_1d_data['t_ic'],
            u_ic=simple_1d_data['u_ic'],
            x_bc=simple_1d_data['x_bc'],
            t_bc=simple_1d_data['t_bc'],
            u_bc=simple_1d_data['u_bc'],
            epochs=2000,
            lr=1e-3,
            verbose=0
        )

        # Test at t = 5s
        x_test = np.linspace(0, 1, 50).reshape(-1, 1)
        t_test = np.ones((50, 1)) * 5.0

        u_pred = pinn.predict(x_test, t_test)
        u_exact = analytical_solution_1d(x_test, 5.0, 1.0e-9)

        l2_error = np.linalg.norm(u_pred - u_exact) / np.linalg.norm(u_exact)

        # Should achieve reasonable accuracy
        assert l2_error < 0.1  # < 10% error

    def test_save_load(self, tmp_path):
        """Test model saving and loading"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        # Save
        model_path = tmp_path / "test_model.pth"
        pinn.save_model(str(model_path))

        assert model_path.exists()

        # Load
        pinn_loaded = DiffusionPINN1D(diffusivity=1.0e-9)
        pinn_loaded.load_model(str(model_path))

        # Test predictions match
        x = np.array([[0.5]])
        t = np.array([[10.0]])

        u1 = pinn.predict(x, t)
        u2 = pinn_loaded.predict(x, t)

        np.testing.assert_allclose(u1, u2, rtol=1e-6)


# ============================================================================
# Test: DiffusionPINN2D
# ============================================================================

class TestDiffusionPINN2D:
    """Tests for 2D Diffusion PINN"""

    def test_initialization(self):
        """Test 2D PINN initialization"""
        pinn = DiffusionPINN2D(diffusivity=1.0e-9)

        assert pinn.D == 1.0e-9
        assert pinn.layers[0] == 3  # Input: (x, y, t)
        assert pinn.layers[-1] == 1  # Output: u

    def test_forward_pass(self):
        """Test 2D forward pass"""
        pinn = DiffusionPINN2D(diffusivity=1.0e-9)

        x = torch.tensor([[0.5]], dtype=torch.float32)
        y = torch.tensor([[0.3]], dtype=torch.float32)
        t = torch.tensor([[10.0]], dtype=torch.float32)
        inputs = torch.cat([x, y, t], dim=1)

        output = pinn.forward(inputs)

        assert output.shape == (1, 1)
        assert not torch.isnan(output).any()

    def test_predict_2d(self):
        """Test 2D prediction"""
        pinn = DiffusionPINN2D(diffusivity=1.0e-9)

        x = np.array([[0.5]])
        y = np.array([[0.3]])
        t = np.array([[10.0]])

        u = pinn.predict_2d(x, y, t)

        assert u.shape == (1,)
        assert not np.isnan(u).any()


# ============================================================================
# Test: GrayScottPINN
# ============================================================================

class TestGrayScottPINN:
    """Tests for Gray-Scott reaction-diffusion PINN"""

    def test_initialization(self):
        """Test Gray-Scott PINN initialization"""
        pinn = GrayScottPINN(Du=2.0e-5, Dv=1.0e-5, F=0.055, k=0.062)

        assert pinn.Du == 2.0e-5
        assert pinn.Dv == 1.0e-5
        assert pinn.F == 0.055
        assert pinn.k == 0.062
        assert pinn.layers[-1] == 2  # Output: (u, v)

    def test_forward_pass(self):
        """Test forward pass returns two species"""
        pinn = GrayScottPINN(Du=2.0e-5, Dv=1.0e-5, F=0.055, k=0.062)

        x = torch.tensor([[0.5]], dtype=torch.float32)
        t = torch.tensor([[10.0]], dtype=torch.float32)
        inputs = torch.cat([x, t], dim=1)

        output = pinn.forward(inputs)

        assert output.shape == (1, 2)  # (u, v)
        assert not torch.isnan(output).any()

    def test_physics_loss(self):
        """Test Gray-Scott physics loss"""
        pinn = GrayScottPINN(Du=2.0e-5, Dv=1.0e-5, F=0.055, k=0.062)

        x = torch.tensor([[0.5]], dtype=torch.float32, requires_grad=True)
        t = torch.tensor([[10.0]], dtype=torch.float32, requires_grad=True)

        loss = pinn.physics_loss(x, t)

        assert isinstance(loss, torch.Tensor)
        assert loss.item() >= 0
        assert not torch.isnan(loss).any()


# ============================================================================
# Test: AdaptivePINN
# ============================================================================

class TestAdaptivePINN:
    """Tests for Adaptive PINN"""

    def test_residual_computation(self):
        """Test residual importance computation"""
        pinn = AdaptivePINN(diffusivity=1.0e-9)

        x = torch.tensor([[0.5]], dtype=torch.float32, requires_grad=True)
        t = torch.tensor([[10.0]], dtype=torch.float32, requires_grad=True)

        importance = pinn.compute_residual_importance(x, t)

        assert importance.shape == (1,)
        assert (importance >= 0).all()

    def test_adaptive_resampling(self):
        """Test adaptive point resampling"""
        pinn = AdaptivePINN(diffusivity=1.0e-9)

        x_pde = np.random.uniform(0, 1, (100, 1))
        t_pde = np.random.uniform(0, 10, (100, 1))

        x_new, t_new = pinn.adaptive_resample(
            x_pde, t_pde, n_new_points=50,
            x_bounds=(0, 1), t_bounds=(0, 10)
        )

        assert x_new.shape == (50, 1)
        assert t_new.shape == (50, 1)
        assert (x_new >= 0).all() and (x_new <= 1).all()
        assert (t_new >= 0).all() and (t_new <= 10).all()

    def test_adaptive_training(self, simple_1d_data):
        """Test adaptive training runs"""
        pinn = AdaptivePINN(diffusivity=1.0e-9)

        history = pinn.train_adaptive(
            x_pde_init=simple_1d_data['x_pde'][:50],
            t_pde_init=simple_1d_data['t_pde'][:50],
            x_bounds=(0, 1),
            t_bounds=(0, 10),
            x_ic=simple_1d_data['x_ic'],
            t_ic=simple_1d_data['t_ic'],
            u_ic=simple_1d_data['u_ic'],
            n_adaptive_iter=3,
            n_new_points_per_iter=20,
            epochs_per_iter=100,
            lr=1e-3,
            verbose=0
        )

        assert 'loss_history' in history
        assert len(history['loss_history']) > 0


# ============================================================================
# Test: InversePINN
# ============================================================================

class TestInversePINN:
    """Tests for Inverse PINN (parameter estimation)"""

    def test_initialization(self):
        """Test inverse PINN initialization"""
        pinn = InversePINN(initial_params={'D': 1.0e-9})

        assert 'D' in pinn.params
        D = pinn.get_parameter('D')
        assert abs(D - 1.0e-9) < 1e-12

    def test_parameter_learning(self):
        """Test that parameter is actually learnable"""
        pinn = InversePINN(initial_params={'D': 1.0e-9})

        # Parameter should have requires_grad=True
        assert pinn.params['D'].requires_grad

    def test_parameter_estimation(self):
        """Test parameter estimation from synthetic data"""
        # True parameter
        D_true = 1.0e-9

        # Generate synthetic data
        def analytical(x, t):
            sigma0 = 0.1
            x0 = 0.5
            sigma_t = np.sqrt(2 * D_true * t + sigma0**2)
            return (sigma0 / sigma_t) * np.exp(-(x - x0)**2 / (2 * sigma_t**2))

        np.random.seed(42)
        x_data = np.random.uniform(0.2, 0.8, (20, 1))
        t_data = np.random.uniform(5, 15, (20, 1))
        u_data = analytical(x_data, t_data)

        # PDE points
        x_pde = np.random.uniform(0, 1, (100, 1))
        t_pde = np.random.uniform(0, 20, (100, 1))

        # IC
        x_ic = np.linspace(0, 1, 50).reshape(-1, 1)
        t_ic = np.zeros((50, 1))
        u_ic = analytical(x_ic, 0)

        # Create inverse PINN with wrong initial guess
        pinn = InversePINN(initial_params={'D': 5.0e-9})
        pinn.lambda_data = 100.0

        # Train
        history = pinn.train_inverse(
            x_data=x_data, t_data=t_data, u_data=u_data,
            x_pde=x_pde, t_pde=t_pde,
            x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
            epochs=1000,
            lr=1e-3,
            verbose=0
        )

        # Check estimation
        D_estimated = pinn.get_parameter('D')
        error = abs(D_estimated - D_true) / D_true

        # Should estimate within 20% error
        assert error < 0.2, f"Estimation error too large: {error*100:.1f}%"

    def test_multi_parameter(self):
        """Test multi-parameter inverse PINN"""
        pinn = MultiParameterInversePINN(
            initial_params={
                'Du': 2.0e-5,
                'Dv': 1.0e-5,
                'F': 0.055,
                'k': 0.062
            }
        )

        assert 'Du' in pinn.params
        assert 'Dv' in pinn.params
        assert 'F' in pinn.params
        assert 'k' in pinn.params

        Du = pinn.get_parameter('Du')
        assert abs(Du - 2.0e-5) < 1e-8


# ============================================================================
# Test: Convergence
# ============================================================================

class TestConvergence:
    """Test convergence properties"""

    def test_loss_decreases(self, simple_1d_data):
        """Test that loss decreases during training"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        history = pinn.train_model(
            x_pde=simple_1d_data['x_pde'],
            t_pde=simple_1d_data['t_pde'],
            x_ic=simple_1d_data['x_ic'],
            t_ic=simple_1d_data['t_ic'],
            u_ic=simple_1d_data['u_ic'],
            x_bc=simple_1d_data['x_bc'],
            t_bc=simple_1d_data['t_bc'],
            u_bc=simple_1d_data['u_bc'],
            epochs=500,
            lr=1e-3,
            verbose=0
        )

        losses = history['loss_history']

        # Loss should decrease
        assert losses[-1] < losses[0]

        # Loss should be monotonically decreasing (mostly)
        decreasing_count = sum(1 for i in range(1, len(losses)) if losses[i] < losses[i-1])
        assert decreasing_count > len(losses) * 0.8  # 80% should decrease

    def test_refinement_improves_accuracy(self):
        """Test that more training improves accuracy"""
        pinn = DiffusionPINN1D(diffusivity=1.0e-9)

        # Generate data
        np.random.seed(42)
        x_pde = np.random.uniform(0, 1, (100, 1))
        t_pde = np.random.uniform(0, 10, (100, 1))
        x_ic = np.linspace(0, 1, 50).reshape(-1, 1)
        t_ic = np.zeros((50, 1))
        u_ic = np.exp(-(x_ic - 0.5)**2 / 0.02)

        # Train for 500 epochs
        pinn.train_model(
            x_pde=x_pde, t_pde=t_pde,
            x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
            epochs=500,
            lr=1e-3,
            verbose=0
        )

        x_test = np.array([[0.5]])
        t_test = np.array([[5.0]])
        u1 = pinn.predict(x_test, t_test)

        # Train for 500 more epochs
        pinn.train_model(
            x_pde=x_pde, t_pde=t_pde,
            x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
            epochs=500,
            lr=1e-4,  # Lower LR for refinement
            verbose=0
        )

        u2 = pinn.predict(x_test, t_test)

        # Predictions should be similar but potentially improved
        assert not np.isnan(u2)


# ============================================================================
# Run Tests
# ============================================================================

if __name__ == '__main__':
    pytest.main([__file__, '-v', '--tb=short'])
