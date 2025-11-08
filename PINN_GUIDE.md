# PINN (Physics-Informed Neural Networks) Guide

**Version**: 6.0.0-alpha5
**Priority**: D2
**Status**: Complete
**Last Updated**: 2025-11-08

---

## Overview

KooLab now includes Physics-Informed Neural Networks (PINNs) for solving PDEs using deep learning. PINNs combine data-driven learning with physics constraints to solve forward and inverse problems.

### What are PINNs?

PINNs are neural networks that:
1. **Learn PDE solutions** directly from physics laws
2. **Incorporate PDE residuals** in the loss function
3. **Use automatic differentiation** for derivatives
4. **Work without mesh** (meshless method)
5. **Solve inverse problems** (estimate unknown parameters)

### Features

✅ **Solvers Implemented**
- 1D Diffusion PINN
- 2D Diffusion PINN
- Reaction-Diffusion PINN (Gray-Scott, Brusselator)

✅ **Advanced Techniques**
- Adaptive sampling (residual-based)
- Inverse problems (parameter estimation)
- Multi-parameter optimization

✅ **Integration**
- Pure Python with PyTorch
- GPU-ready (automatic)
- Model save/load
- Easy-to-use API

---

## Quick Start

### 1. Install Dependencies

```bash
pip install torch numpy matplotlib
```

### 2. Run 1D Diffusion Example

```bash
python examples/pinn_diffusion_1d.py
```

### 3. Basic Usage

```python
from koolab.ml import DiffusionPINN1D
import numpy as np

# Create PINN
pinn = DiffusionPINN1D(diffusivity=1.0e-9)

# Generate training data
x_pde = np.random.uniform(0, 1, (1000, 1))
t_pde = np.random.uniform(0, 100, (1000, 1))

x_ic = np.linspace(0, 1, 100).reshape(-1, 1)
t_ic = np.zeros((100, 1))
u_ic = np.exp(-(x_ic - 0.5)**2 / 0.02)  # Gaussian

# Train
pinn.train_model(
    x_pde=x_pde, t_pde=t_pde,
    x_ic=x_ic, t_ic=t_ic, u_ic=u_ic,
    epochs=5000, lr=1e-3
)

# Predict
x_test = np.linspace(0, 1, 100).reshape(-1, 1)
t_test = np.ones((100, 1)) * 50
u_pred = pinn.predict(x_test, t_test)
```

---

## Available PINNs

### 1. DiffusionPINN1D

**PDE**: ∂u/∂t = D ∂²u/∂x²

```python
from koolab.ml import DiffusionPINN1D

pinn = DiffusionPINN1D(
    diffusivity=1.0e-9,
    layers=[2, 50, 50, 50, 1]  # Optional
)

pinn.train_model(
    x_pde, t_pde,          # Collocation points
    x_ic, t_ic, u_ic,      # Initial condition
    x_bc, t_bc, u_bc,      # Boundary condition
    epochs=10000,
    lr=1e-3
)

u_pred = pinn.predict(x_test, t_test)
```

### 2. DiffusionPINN2D

**PDE**: ∂u/∂t = D (∂²u/∂x² + ∂²u/∂y²)

```python
from koolab.ml import DiffusionPINN2D

pinn = DiffusionPINN2D(
    diffusivity=1.0e-9,
    layers=[3, 64, 64, 64, 1]  # Input: (x,y,t) -> u
)

pinn.train_model_2d(
    x_pde, y_pde, t_pde,
    x_ic, y_ic, t_ic, u_ic,
    epochs=10000
)

u_pred = pinn.predict_2d(x_test, y_test, t_test)
```

### 3. GrayScottPINN

**PDEs**:
- ∂u/∂t = D_u ∂²u/∂x² - uv² + F(1-u)
- ∂v/∂t = D_v ∂²v/∂x² + uv² - (F+k)v

```python
from koolab.ml import GrayScottPINN

pinn = GrayScottPINN(
    Du=2.0e-5, Dv=1.0e-5,
    F=0.055, k=0.062  # Spot patterns
)

# Network outputs (u, v)
pinn.train_model(...)
```

### 4. AdaptivePINN

Adaptive collocation sampling based on PDE residual:

```python
from koolab.ml import AdaptivePINN

pinn = AdaptivePINN(diffusivity=1.0e-9)

pinn.train_adaptive(
    x_pde_init, t_pde_init,  # Initial points
    x_bounds=(0, 1),
    t_bounds=(0, 100),
    x_ic, t_ic, u_ic,
    n_adaptive_iter=5,        # Refinement iterations
    n_new_points_per_iter=200 # Points to add each iter
)
```

**Algorithm**:
1. Train with initial points
2. Compute |residual| at all points
3. Sample new points where residual is high
4. Add new points and retrain
5. Repeat

**Benefits**: Focuses computation on difficult regions, better accuracy with fewer points.

### 5. InversePINN

Parameter estimation from data:

```python
from koolab.ml import InversePINN

# Estimate diffusion coefficient D from measurements
pinn = InversePINN(
    initial_params={'D': 1.0e-9}  # Initial guess
)

pinn.train_inverse(
    x_data, t_data, u_data,  # Measurements
    x_pde, t_pde,            # Collocation points
    epochs=10000
)

# Get estimated parameter
D_estimated = pinn.get_parameter('D')
print(f"Estimated D: {D_estimated:.6e}")
```

**Multi-parameter**:

```python
from koolab.ml import MultiParameterInversePINN

pinn = MultiParameterInversePINN(
    initial_params={
        'Du': 2.0e-5,
        'Dv': 1.0e-5,
        'F': 0.055,
        'k': 0.062
    }
)

pinn.train_inverse(...)

# Get all estimated parameters
for name in ['Du', 'Dv', 'F', 'k']:
    print(f"{name} = {pinn.get_parameter(name):.6e}")
```

---

## Loss Function

PINN loss is multi-term:

```
Loss = λ_data * Loss_data       # Data fitting
     + λ_pde * Loss_pde         # Physics constraint
     + λ_bc * Loss_bc           # Boundary conditions
     + λ_ic * Loss_ic           # Initial conditions
```

### Loss Components

1. **Data Loss**: MSE(u_pred, u_data)
   - Fits network to measurement data

2. **Physics Loss**: MSE(PDE_residual, 0)
   - Enforces PDE: R = ∂u/∂t - D ∂²u/∂x²
   - Computed via automatic differentiation

3. **Boundary Loss**: MSE(u_pred_bc, u_bc)
   - Enforces boundary conditions

4. **Initial Loss**: MSE(u_pred_ic, u_ic)
   - Enforces initial conditions

### Tuning Loss Weights

```python
pinn = DiffusionPINN1D(diffusivity=1.0e-9)

# Adjust weights
pinn.lambda_data = 1.0    # Data importance
pinn.lambda_pde = 1.0     # Physics importance
pinn.lambda_bc = 10.0     # BC importance (often higher)
pinn.lambda_ic = 10.0     # IC importance (often higher)

pinn.train_model(...)
```

**Guidelines**:
- Start with all weights = 1.0
- Increase BC/IC weights if boundaries not satisfied
- Increase PDE weight if physics not respected
- Increase data weight if fitting is poor

---

## Training Tips

### 1. Network Architecture

```python
# Small problems (1D)
layers = [2, 50, 50, 50, 1]

# Medium problems (2D)
layers = [3, 64, 64, 64, 1]

# Complex problems
layers = [3, 100, 100, 100, 100, 1]
```

### 2. Collocation Points

```python
# Uniform sampling
x = np.linspace(0, 1, 1000)
t = np.linspace(0, 100, 1000)
X, T = np.meshgrid(x, t)
x_pde = X.flatten().reshape(-1, 1)
t_pde = T.flatten().reshape(-1, 1)

# Random sampling (recommended)
x_pde = np.random.uniform(0, 1, (10000, 1))
t_pde = np.random.uniform(0, 100, (10000, 1))

# More points near boundaries
x_bc = np.random.beta(0.5, 0.5, (2000, 1))  # Concentrates at 0 and 1
```

### 3. Optimizer Selection

```python
# Adam (default, good for most cases)
pinn.train_model(..., optimizer_type='adam', lr=1e-3)

# LBFGS (better convergence, slower)
pinn.train_model(..., optimizer_type='lbfgs', lr=1.0)

# Two-stage training (best results)
pinn.train_model(..., epochs=5000, optimizer_type='adam', lr=1e-3)
pinn.train_model(..., epochs=1000, optimizer_type='lbfgs', lr=1.0)
```

### 4. GPU Acceleration

```python
import torch

# Move model to GPU (automatic if available)
if torch.cuda.is_available():
    pinn = pinn.cuda()

# PyTorch handles GPU automatically for tensors
# No code changes needed!
```

---

## Validation

### Check PDE Residual

```python
# Evaluate physics loss after training
x_test = np.random.uniform(0, 1, (1000, 1))
t_test = np.random.uniform(0, 100, (1000, 1))

x_t = torch.tensor(x_test, requires_grad=True)
t_t = torch.tensor(t_test, requires_grad=True)

residual_loss = pinn.physics_loss(x_t, t_t)
print(f"PDE Residual: {residual_loss.item():.6e}")
# Should be very small (< 1e-4)
```

### Compare with Analytical Solution

```python
# If analytical solution exists
def u_analytical(x, t, D):
    # e.g., Gaussian spreading
    sigma_t = np.sqrt(2 * D * t + sigma_0**2)
    return (sigma_0 / sigma_t) * np.exp(-(x - x0)**2 / (2 * sigma_t**2))

u_exact = u_analytical(x_test, t_test, D)
u_pred = pinn.predict(x_test, t_test)

l2_error = np.linalg.norm(u_pred - u_exact) / np.linalg.norm(u_exact)
print(f"L2 Relative Error: {l2_error:.6e}")
# Should be < 1e-2 for good fit
```

---

## Applications

### 1. Forward Problems

Solve PDEs when D is known:

```python
pinn = DiffusionPINN1D(diffusivity=1.0e-9)
pinn.train_model(x_pde, t_pde, x_ic, t_ic, u_ic)
u_pred = pinn.predict(x_test, t_test)
```

### 2. Inverse Problems

Estimate D from measurements:

```python
pinn = InversePINN(initial_params={'D': 1.0e-9})
pinn.train_inverse(x_data, t_data, u_data, x_pde, t_pde)
D_est = pinn.get_parameter('D')
```

### 3. Data Assimilation

Combine sparse measurements with physics:

```python
pinn = DiffusionPINN1D(diffusivity=1.0e-9)

# Sparse data
x_data = sparse_measurements_x
t_data = sparse_measurements_t
u_data = sparse_measurements_u

# Dense physics constraint
x_pde = np.random.uniform(0, 1, (10000, 1))
t_pde = np.random.uniform(0, 100, (10000, 1))

pinn.lambda_data = 100.0  # Trust data more
pinn.lambda_pde = 1.0

pinn.train_model(
    x_data=x_data, t_data=t_data, u_data=u_data,
    x_pde=x_pde, t_pde=t_pde
)
```

---

## Advantages vs Traditional Methods

| Aspect | PINN | Traditional (FDM/FEM) |
|--------|------|----------------------|
| Mesh | Not needed | Required |
| Irregular domains | Easy | Difficult |
| Inverse problems | Natural | Challenging |
| Data incorporation | Easy | Difficult |
| High dimensions | Feasible | Curse of dimensionality |
| Smoothness | C∞ smooth | C⁰ or C¹ |
| Training time | Hours | Minutes |
| Inference | Fast | Fast |

---

## Limitations

1. **Training time**: Can be slow (hours for complex problems)
2. **Hyperparameters**: Requires tuning (network size, learning rate, loss weights)
3. **Convergence**: Not always guaranteed
4. **Accuracy**: Typically 1e-2 to 1e-4 relative error
5. **Debugging**: Hard to diagnose failures

---

## Troubleshooting

### Loss not decreasing

- Increase learning rate (try 1e-2, 1e-3, 1e-4)
- Increase network size
- Check loss weights (may be unbalanced)
- Add more collocation points

### High PDE residual

- Increase λ_pde weight
- Add more PDE collocation points
- Use LBFGS optimizer
- Train longer

### Boundary conditions not satisfied

- Increase λ_bc and λ_ic weights
- Add more BC/IC points
- Check BC/IC data is correct

### NaN loss

- Reduce learning rate
- Check input normalization
- Check for numerical issues in PDE

---

## Next Steps

1. Try examples: `python examples/pinn_diffusion_1d.py`
2. Experiment with different networks
3. Tune loss weights for your problem
4. Compare with traditional methods
5. Explore inverse problems

---

## References

- Original PINN paper: Raissi et al. (2019)
- PyTorch documentation: https://pytorch.org/
- Automatic differentiation tutorial: https://pytorch.org/tutorials/beginner/blitz/autograd_tutorial.html

---

**For more information**: See python/koolab/ml/ source code and examples/
