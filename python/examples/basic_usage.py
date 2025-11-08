"""
Basic Usage Example
Phase 57-60: Python Interface Complete

Demonstrates basic KooChemicalSimulation functionality.
"""

import numpy as np
import koolab as koo

def main():
    print("=" * 60)
    print("KooChemicalSimulation - Basic Usage Example")
    print("=" * 60)
    print()

    # Print library info
    print("Library Information:")
    koo.print_info()
    print()

    # 1. Mesh Generation
    print("1. Creating Mesh...")
    mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
    print(f"   Nodes: {mesh.num_nodes()}")
    print(f"   Elements: {mesh.num_elements()}")
    print()

    # 2. Chemical Species
    print("2. Creating Chemical Species...")
    h2 = koo.Species("H2")
    h2.molar_mass = 0.002
    h2.set_composition({"H": 2})

    o2 = koo.Species("O2")
    o2.molar_mass = 0.032
    o2.set_composition({"O": 2})

    h2o = koo.Species("H2O")
    h2o.molar_mass = 0.018
    h2o.set_composition({"H": 2, "O": 1})

    print(f"   {h2}")
    print(f"   {o2}")
    print(f"   {h2o}")
    print()

    # 3. Chemical Reaction
    print("3. Creating Reaction...")
    rxn = koo.Reaction()
    rxn.add_reactant("H2", 2.0)
    rxn.add_reactant("O2", 1.0)
    rxn.add_product("H2O", 2.0)
    print(f"   {rxn}")
    print()

    # 4. Arrhenius Rate
    print("4. Arrhenius Rate Constant...")
    rate = koo.ArrheniusRate(A=1e13, beta=0.0, Ea=150000)
    print(f"   Parameters: {rate}")

    temperatures = [500, 1000, 1500, 2000]
    print("   k(T) at different temperatures:")
    for T in temperatures:
        k = rate(T)
        print(f"     T = {T} K: k = {k:.3e}")
    print()

    # 5. NumPy Integration
    print("5. NumPy Integration...")
    vec = koo.Vector(100, 1.5)
    arr = koo.numpy_utils.to_numpy(vec)
    print(f"   Vector size: {len(vec)}")
    print(f"   NumPy array shape: {arr.shape}")
    print(f"   Mean value: {np.mean(arr):.2f}")
    print()

    # 6. GPU Information
    print("6. GPU Information...")
    gpu_info = koo.gpu.get_gpu_info()
    print(f"   Device count: {gpu_info['device_count']}")
    print(f"   Runtime: {gpu_info['runtime']}")
    print(f"   CUDA enabled: {gpu_info['cuda_enabled']}")
    print()

    print("=" * 60)
    print("Example completed successfully!")
    print("=" * 60)

if __name__ == "__main__":
    main()
