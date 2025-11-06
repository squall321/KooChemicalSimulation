"""
Unit tests for KooChemicalSimulation Python bindings
Phase 56: Core Python Interface
"""

import unittest
import sys

try:
    import koolab as koo
    CPP_AVAILABLE = True
except ImportError:
    CPP_AVAILABLE = False
    print("WARNING: C++ extension not available, skipping tests")


@unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
class TestCore(unittest.TestCase):
    """Test core module"""

    def test_version(self):
        """Test version string"""
        self.assertIsNotNone(koo.__version__)
        self.assertIn("6.0.0", koo.__version__)

    def test_runtime_info(self):
        """Test runtime info"""
        info = koo.get_runtime_info()
        self.assertIsInstance(info, dict)
        self.assertIn("version", info)

    def test_vector_creation(self):
        """Test Vector creation"""
        v = koo.Vector(10)
        self.assertEqual(len(v), 10)

    def test_vector_indexing(self):
        """Test Vector indexing"""
        v = koo.Vector(5, 1.5)
        self.assertEqual(v[0], 1.5)

        v[0] = 2.5
        self.assertEqual(v[0], 2.5)

    def test_logger(self):
        """Test Logger"""
        logger = koo.Logger.get_instance()
        self.assertIsNotNone(logger)

        logger.set_level(koo.LogLevel.INFO)
        logger.info("Test message")


@unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
class TestMesh(unittest.TestCase):
    """Test mesh module"""

    def test_node_creation(self):
        """Test Node creation"""
        node = koo.Node(1, 0.0, 1.0, 0.0)
        self.assertEqual(node.id, 1)
        self.assertEqual(node.x, 0.0)
        self.assertEqual(node.y, 1.0)

    def test_element_creation(self):
        """Test Element creation"""
        elem = koo.Element(1, koo.ElementType.TRIANGLE)
        self.assertEqual(elem.id, 1)
        self.assertEqual(elem.type, koo.ElementType.TRIANGLE)

        elem.add_node(0)
        elem.add_node(1)
        elem.add_node(2)
        self.assertEqual(elem.num_nodes(), 3)

    def test_mesh_data(self):
        """Test MeshData"""
        mesh = koo.MeshData()
        self.assertEqual(mesh.num_nodes(), 0)
        self.assertEqual(mesh.num_elements(), 0)

        # Add nodes
        mesh.add_node(koo.Node(0, 0.0, 0.0, 0.0))
        mesh.add_node(koo.Node(1, 1.0, 0.0, 0.0))
        mesh.add_node(koo.Node(2, 0.0, 1.0, 0.0))
        self.assertEqual(mesh.num_nodes(), 3)

        # Add element
        elem = koo.Element(0, koo.ElementType.TRIANGLE)
        elem.add_node(0)
        elem.add_node(1)
        elem.add_node(2)
        mesh.add_element(elem)
        self.assertEqual(mesh.num_elements(), 1)

    def test_create_rectangular_mesh(self):
        """Test rectangular mesh creation"""
        mesh = koo.create_rectangular_mesh(0, 0, 1, 1, 10, 10)
        self.assertEqual(mesh.num_nodes(), 121)  # 11 x 11
        self.assertEqual(mesh.num_elements(), 100)  # 10 x 10


@unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
class TestChemistry(unittest.TestCase):
    """Test chemistry module"""

    def test_species_creation(self):
        """Test Species creation"""
        sp = koo.Species("H2")
        self.assertEqual(sp.name, "H2")

        sp.molar_mass = 0.002  # kg/mol
        self.assertAlmostEqual(sp.molar_mass, 0.002)

    def test_species_composition(self):
        """Test Species composition"""
        sp = koo.Species("H2O")
        sp.set_composition({"H": 2, "O": 1})

        comp = sp.get_composition()
        self.assertEqual(comp["H"], 2)
        self.assertEqual(comp["O"], 1)

    def test_reaction_creation(self):
        """Test Reaction creation"""
        rxn = koo.Reaction()
        rxn.add_reactant("H2", 2.0)
        rxn.add_reactant("O2", 1.0)
        rxn.add_product("H2O", 2.0)

        reactants = rxn.get_reactants()
        self.assertEqual(reactants["H2"], 2.0)
        self.assertEqual(reactants["O2"], 1.0)

        products = rxn.get_products()
        self.assertEqual(products["H2O"], 2.0)

    def test_arrhenius_rate(self):
        """Test ArrheniusRate"""
        rate = koo.ArrheniusRate(1e13, 0.0, 150000)
        self.assertAlmostEqual(rate.A, 1e13)
        self.assertAlmostEqual(rate.beta, 0.0)
        self.assertAlmostEqual(rate.Ea, 150000)

        # Evaluate at temperature
        k = rate.evaluate(1000.0)
        self.assertGreater(k, 0)

        # Test __call__
        k2 = rate(1000.0)
        self.assertAlmostEqual(k, k2)

    def test_parse_reaction(self):
        """Test reaction parsing"""
        rxn = koo.parse_reaction("H2 + O2 => H2O")
        self.assertIn("H2", rxn.get_reactants())
        self.assertIn("O2", rxn.get_reactants())
        self.assertIn("H2O", rxn.get_products())
        self.assertFalse(rxn.is_reversible())

        rxn2 = koo.parse_reaction("A + B <=> C")
        self.assertTrue(rxn2.is_reversible())


@unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
class TestGPU(unittest.TestCase):
    """Test GPU module"""

    def test_device_count(self):
        """Test device count"""
        count = koo.gpu.device_count()
        self.assertGreaterEqual(count, 0)

    def test_gpu_runtime(self):
        """Test GPU runtime"""
        runtime = koo.Device.get_runtime()
        self.assertIn(runtime, ["CUDA", "HIP", "CPU"])

    def test_gpu_info(self):
        """Test GPU info"""
        info = koo.gpu.get_gpu_info()
        self.assertIsInstance(info, dict)
        self.assertIn("device_count", info)
        self.assertIn("runtime", info)

    @unittest.skipIf(koo.gpu.device_count() == 0, "No GPU available")
    def test_device_creation(self):
        """Test Device creation"""
        dev = koo.Device.get_device(0)
        self.assertEqual(dev.get_id(), 0)
        self.assertIsInstance(dev.get_name(), str)

    @unittest.skipIf(koo.gpu.device_count() == 0, "No GPU available")
    def test_multi_gpu(self):
        """Test MultiGPU manager"""
        mgr = koo.MultiGPUManager()
        mgr.initialize()

        self.assertTrue(mgr.is_initialized())
        self.assertGreater(mgr.get_num_gpus(), 0)

        mgr.finalize()
        self.assertFalse(mgr.is_initialized())

    @unittest.skipIf(koo.gpu.device_count() == 0, "No GPU available")
    def test_multi_gpu_context(self):
        """Test MultiGPU manager as context manager"""
        with koo.MultiGPUManager() as mgr:
            self.assertTrue(mgr.is_initialized())

        # Should be finalized after context
        self.assertFalse(mgr.is_initialized())


class TestPackage(unittest.TestCase):
    """Test package-level functions"""

    @unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
    def test_get_info(self):
        """Test get_info"""
        info = koo.get_info()
        self.assertIsInstance(info, dict)
        self.assertIn("version", info)
        self.assertIn("cpp_available", info)

    @unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
    def test_print_info(self):
        """Test print_info"""
        # Just make sure it doesn't crash
        koo.print_info()

    @unittest.skipUnless(CPP_AVAILABLE, "C++ extension not available")
    def test_hello(self):
        """Test hello"""
        koo.hello()


def run_tests():
    """Run all tests"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestCore))
    suite.addTests(loader.loadTestsFromTestCase(TestMesh))
    suite.addTests(loader.loadTestsFromTestCase(TestChemistry))
    suite.addTests(loader.loadTestsFromTestCase(TestGPU))
    suite.addTests(loader.loadTestsFromTestCase(TestPackage))

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
