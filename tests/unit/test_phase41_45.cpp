#include "parallel/mpi/MPIWrapper.h"
#include "parallel/domain/DomainDecomposition.h"
#include "parallel/linalg/ParallelVector.h"
#include "parallel/io/ParallelIO.h"
#include "parallel/balance/LoadBalancing.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

using namespace koo::parallel;

// Test 1: MPI Environment
void testMPIEnvironment() {
    std::cout << "\nTesting MPI environment..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    std::cout << "  → Rank: " << comm.getRank() << std::endl;
    std::cout << "  → Size: " << comm.getSize() << std::endl;
    std::cout << "  → Is Root: " << (comm.isRoot() ? "Yes" : "No") << std::endl;

    assert(comm.getRank() >= 0);
    assert(comm.getSize() >= 1);
    assert(comm.getRank() < comm.getSize());

    if (comm.getRank() == 0) {
        assert(comm.isRoot());
    }

    std::cout << "  ✓ MPI environment works" << std::endl;
}

// Test 2: Barrier Synchronization
void testBarrier() {
    std::cout << "\nTesting barrier synchronization..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    // All processes reach barrier
    comm.barrier();

    std::cout << "  ✓ Barrier synchronization works" << std::endl;
}

// Test 3: Broadcast
void testBroadcast() {
    std::cout << "\nTesting broadcast..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    int value = 0;
    if (comm.isRoot()) {
        value = 42;
    }

    comm.broadcast(&value, 1, 0);

    assert(value == 42);

    std::cout << "  → All processes have value: " << value << std::endl;
    std::cout << "  ✓ Broadcast works" << std::endl;
}

// Test 4: All-Reduce Sum
void testAllReduceSum() {
    std::cout << "\nTesting all-reduce sum..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    int localValue = comm.getRank() + 1;  // 1, 2, 3, ...
    int globalSum = comm.sum(localValue);

    // Sum should be n(n+1)/2
    int expectedSum = comm.getSize() * (comm.getSize() + 1) / 2;
    assert(globalSum == expectedSum);

    std::cout << "  → Global sum: " << globalSum << " (expected: " << expectedSum << ")" << std::endl;
    std::cout << "  ✓ All-reduce sum works" << std::endl;
}

// Test 5: All-Reduce Max/Min
void testAllReduceMaxMin() {
    std::cout << "\nTesting all-reduce max/min..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    int localValue = comm.getRank();
    int globalMax = comm.max(localValue);
    int globalMin = comm.min(localValue);

    assert(globalMax == comm.getSize() - 1);
    assert(globalMin == 0);

    std::cout << "  → Global max: " << globalMax << std::endl;
    std::cout << "  → Global min: " << globalMin << std::endl;
    std::cout << "  ✓ All-reduce max/min works" << std::endl;
}

// Test 6: Cartesian Decomposition
void testCartesianDecomposition() {
    std::cout << "\nTesting Cartesian decomposition..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    domain::CartesianDecomposition decomp(100, 100, 10, comm);

    auto bounds = decomp.getLocalBounds();
    auto procGrid = decomp.getProcessGrid();
    auto coords = decomp.getProcessCoords();

    std::cout << "  → Process grid: [" << procGrid[0] << ", "
              << procGrid[1] << ", " << procGrid[2] << "]" << std::endl;
    std::cout << "  → Process coords: [" << coords[0] << ", "
              << coords[1] << ", " << coords[2] << "]" << std::endl;
    std::cout << "  → Local bounds: ["
              << bounds[0] << "-" << bounds[1] << ", "
              << bounds[2] << "-" << bounds[3] << ", "
              << bounds[4] << "-" << bounds[5] << "]" << std::endl;

    // Verify decomposition is valid
    assert(bounds[0] <= bounds[1]);
    assert(bounds[2] <= bounds[3]);
    assert(bounds[4] <= bounds[5]);

    std::cout << "  ✓ Cartesian decomposition works" << std::endl;
}

// Test 7: Neighbors
void testNeighbors() {
    std::cout << "\nTesting neighbor detection..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    domain::CartesianDecomposition decomp(100, 100, 10, comm);

    auto neighbors = decomp.getNeighborRanks();

    std::cout << "  → Number of neighbors: " << neighbors.size() << std::endl;
    if (!neighbors.empty()) {
        std::cout << "  → Neighbor ranks: ";
        for (int n : neighbors) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "  ✓ Neighbor detection works" << std::endl;
}

// Test 8: Graph Decomposition
void testGraphDecomposition() {
    std::cout << "\nTesting graph decomposition..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    domain::GraphDecomposition decomp(1000, comm);

    auto partition = decomp.getPartition();

    std::cout << "  → Local nodes: " << partition.localNodeIDs.size() << std::endl;
    std::cout << "  → Process rank: " << partition.processRank << std::endl;

    assert(!partition.localNodeIDs.empty());
    assert(partition.processRank == comm.getRank());

    std::cout << "  ✓ Graph decomposition works" << std::endl;
}

// Test 9: Parallel Vector Creation
void testParallelVectorCreation() {
    std::cout << "\nTesting parallel vector creation..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    int localSize = 10;
    int ghostSize = 2;

    linalg::ParallelVector vec(localSize, ghostSize, comm);

    assert(vec.getLocalSize() == localSize);
    assert(vec.getGhostSize() == ghostSize);
    assert(vec.getGlobalSize() == localSize * comm.getSize());

    std::cout << "  → Local size: " << vec.getLocalSize() << std::endl;
    std::cout << "  → Ghost size: " << vec.getGhostSize() << std::endl;
    std::cout << "  → Global size: " << vec.getGlobalSize() << std::endl;

    std::cout << "  ✓ Parallel vector creation works" << std::endl;
}

// Test 10: Parallel Vector Operations
void testParallelVectorOperations() {
    std::cout << "\nTesting parallel vector operations..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    linalg::ParallelVector vec(10, 0, comm);

    // Fill with rank
    vec.fill(comm.getRank() + 1.0);

    // Test norms
    double norm2 = vec.norm2();
    double norm1 = vec.norm1();
    double sum = vec.sum();

    std::cout << "  → L2 norm: " << norm2 << std::endl;
    std::cout << "  → L1 norm: " << norm1 << std::endl;
    std::cout << "  → Sum: " << sum << std::endl;

    assert(norm2 > 0.0);
    assert(norm1 > 0.0);
    assert(sum > 0.0);

    std::cout << "  ✓ Parallel vector operations work" << std::endl;
}

// Test 11: Dot Product
void testDotProduct() {
    std::cout << "\nTesting dot product..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    linalg::ParallelVector vec1(10, 0, comm);
    linalg::ParallelVector vec2(10, 0, comm);

    vec1.fill(2.0);
    vec2.fill(3.0);

    double dot = vec1.dot(vec2);

    // Expected: 2 * 3 * 10 * nprocs = 60 * nprocs
    double expected = 60.0 * comm.getSize();
    assert(std::abs(dot - expected) < 1e-10);

    std::cout << "  → Dot product: " << dot << " (expected: " << expected << ")" << std::endl;
    std::cout << "  ✓ Dot product works" << std::endl;
}

// Test 12: AXPY
void testAXPY() {
    std::cout << "\nTesting AXPY..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    linalg::ParallelVector y(10, 0, comm);
    linalg::ParallelVector x(10, 0, comm);

    y.fill(1.0);
    x.fill(2.0);

    // y = 3*x + y = 3*2 + 1 = 7
    y.axpy(3.0, x);

    // Check first local element
    assert(std::abs(y[0] - 7.0) < 1e-10);

    std::cout << "  → First element after AXPY: " << y[0] << " (expected: 7)" << std::endl;
    std::cout << "  ✓ AXPY works" << std::endl;
}

// Test 13: Parallel CSR Matrix
void testParallelCSRMatrix() {
    std::cout << "\nTesting parallel CSR matrix..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    int localRows = 5;
    linalg::ParallelCSRMatrix mat(localRows, comm);

    // Add some entries
    for (int i = 0; i < localRows; ++i) {
        mat.addEntry(i, i, 2.0);  // Diagonal
    }

    mat.finalize();

    assert(mat.getLocalRows() == localRows);
    assert(mat.getGlobalRows() == localRows * comm.getSize());

    std::cout << "  → Local rows: " << mat.getLocalRows() << std::endl;
    std::cout << "  → Global rows: " << mat.getGlobalRows() << std::endl;

    std::cout << "  ✓ Parallel CSR matrix works" << std::endl;
}

// Test 14: Parallel I/O - Distributed
void testParallelIODistributed() {
    std::cout << "\nTesting parallel I/O (distributed)..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    io::ParallelWriter writer(comm);

    std::vector<double> localData = {1.0, 2.0, 3.0, 4.0, 5.0};

    bool success = writer.writeDistributed("test_parallel", localData);

    assert(success);

    std::cout << "  ✓ Parallel I/O (distributed) works" << std::endl;
}

// Test 15: Load Monitor
void testLoadMonitor() {
    std::cout << "\nTesting load monitor..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    balance::LoadMonitor monitor(comm);

    balance::LoadStats stats;
    stats.computeTime = (comm.getRank() + 1) * 0.1;  // Varying load
    stats.workUnits = 100;

    monitor.recordStats(stats);

    auto globalStats = monitor.getGlobalStats();

    std::cout << "  → Min load: " << globalStats.minLoad << std::endl;
    std::cout << "  → Max load: " << globalStats.maxLoad << std::endl;
    std::cout << "  → Avg load: " << globalStats.avgLoad << std::endl;
    std::cout << "  → Imbalance: " << globalStats.imbalance << std::endl;

    assert(globalStats.minLoad > 0.0);
    assert(globalStats.maxLoad > 0.0);
    assert(globalStats.avgLoad > 0.0);
    assert(globalStats.imbalance >= 1.0);

    std::cout << "  ✓ Load monitor works" << std::endl;
}

// Test 16: Work Timer
void testWorkTimer() {
    std::cout << "\nTesting work timer..." << std::endl;

    balance::WorkTimer timer;

    timer.start();

    // Simulate some work
    volatile double sum = 0.0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }

    double elapsed = timer.stop();

    std::cout << "  → Elapsed time: " << elapsed << " seconds" << std::endl;

    assert(elapsed > 0.0);

    std::cout << "  ✓ Work timer works" << std::endl;
}

// Test 17: Load Balancer
void testLoadBalancer() {
    std::cout << "\nTesting load balancer..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    balance::LoadBalancer balancer(comm);

    // Record some work
    balancer.recordComputeTime(0.1 * (comm.getRank() + 1), 100);

    // Current distribution
    std::vector<int> distribution(comm.getSize(), 100);

    // Compute new distribution
    auto newDist = balancer.computeNewDistribution(distribution);

    std::cout << "  → Old distribution: ";
    for (int d : distribution) std::cout << d << " ";
    std::cout << std::endl;

    std::cout << "  → New distribution: ";
    for (int d : newDist) std::cout << d << " ";
    std::cout << std::endl;

    // Check total work is conserved
    int totalOld = 0, totalNew = 0;
    for (size_t i = 0; i < distribution.size(); ++i) {
        totalOld += distribution[i];
        totalNew += newDist[i];
    }

    assert(totalOld == totalNew);

    std::cout << "  ✓ Load balancer works" << std::endl;
}

// Test 18: Adaptive Partitioner
void testAdaptivePartitioner() {
    std::cout << "\nTesting adaptive partitioner..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    balance::AdaptivePartitioner partitioner(comm);

    domain::DomainPartition partition;
    partition.processRank = comm.getRank();
    partition.localNodeIDs.resize(100);

    // Update with some compute time
    partitioner.updatePartition(partition, 0.1);

    double imbalance = partitioner.getImbalance();

    std::cout << "  → Imbalance: " << imbalance << std::endl;
    std::cout << "  → Rebalance count: " << partitioner.getRebalanceCount() << std::endl;

    assert(imbalance >= 1.0);

    std::cout << "  ✓ Adaptive partitioner works" << std::endl;
}

// Test 19: Integration - Decomposition + Vector
void testDecompositionVectorIntegration() {
    std::cout << "\nTesting decomposition + vector integration..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    // Create decomposition
    domain::GraphDecomposition decomp(1000, comm);
    auto partition = decomp.getPartition();

    // Create vector matching decomposition
    int localSize = partition.localNodeIDs.size();
    linalg::ParallelVector vec(localSize, 0, comm);

    vec.fill(1.0);

    double sum = vec.sum();
    double expected = 1000.0;  // Total nodes

    assert(std::abs(sum - expected) < 1e-10);

    std::cout << "  → Vector sum: " << sum << " (expected: " << expected << ")" << std::endl;
    std::cout << "  ✓ Integration works" << std::endl;
}

// Test 20: Complete Workflow
void testCompleteWorkflow() {
    std::cout << "\nTesting complete parallel workflow..." << std::endl;

    auto comm = mpi::MPIEnvironment::getWorldComm();

    // 1. Domain decomposition
    domain::CartesianDecomposition decomp(10, 10, 10, comm);
    auto localSize = decomp.getLocalSize();
    int totalLocal = localSize[0] * localSize[1] * localSize[2];

    // 2. Create parallel vector
    linalg::ParallelVector vec(totalLocal, 0, comm);
    vec.fill(comm.getRank() + 1.0);

    // 3. Perform operations
    double norm = vec.norm2();

    // 4. Monitor load
    balance::LoadMonitor monitor(comm);
    balance::LoadStats stats;
    stats.computeTime = 0.1;
    stats.workUnits = totalLocal;
    monitor.recordStats(stats);

    // 5. Write output
    io::ParallelWriter writer(comm);
    auto localData = vec.getLocal();
    writer.writeDistributed("test_workflow", localData);

    std::cout << "  → Local domain size: " << totalLocal << std::endl;
    std::cout << "  → Vector norm: " << norm << std::endl;
    std::cout << "  → Load imbalance: " << monitor.getGlobalStats().imbalance << std::endl;

    std::cout << "  ✓ Complete workflow works" << std::endl;
}

int main(int argc, char** argv) {
    // Initialize MPI environment (RAII)
    mpi::MPIEnvironment mpiEnv(&argc, &argv);

    auto comm = mpi::MPIEnvironment::getWorldComm();

    if (comm.isRoot()) {
        std::cout << "Phase 41-45 Tests - Parallel Computing" << std::endl;
        std::cout << "=======================================" << std::endl;
        std::cout << "Running with " << comm.getSize() << " process(es)" << std::endl;
    }

    // Phase 41: MPI Wrapper
    testMPIEnvironment();
    testBarrier();
    testBroadcast();
    testAllReduceSum();
    testAllReduceMaxMin();

    // Phase 42: Domain Decomposition
    testCartesianDecomposition();
    testNeighbors();
    testGraphDecomposition();

    // Phase 43: Parallel Linear Algebra
    testParallelVectorCreation();
    testParallelVectorOperations();
    testDotProduct();
    testAXPY();
    testParallelCSRMatrix();

    // Phase 44: Parallel I/O
    testParallelIODistributed();

    // Phase 45: Load Balancing
    testLoadMonitor();
    testWorkTimer();
    testLoadBalancer();
    testAdaptivePartitioner();

    // Integration
    testDecompositionVectorIntegration();
    testCompleteWorkflow();

    if (comm.isRoot()) {
        std::cout << "\n=======================================" << std::endl;
        std::cout << "All Phase 41-45 tests passed!" << std::endl;
        std::cout << "Parallel computing system verified." << std::endl;
    }

    return 0;
}
