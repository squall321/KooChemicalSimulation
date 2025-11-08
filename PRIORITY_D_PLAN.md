# Priority D: Advanced Features - Detailed Plan

**작성일**: 2025-11-07
**현재 상태**: Priority A, B, C 완료 (75%)
**목표**: MPI 병렬화 + PINN 통합

---

## 🎯 Priority D 구성 (재정의)

### 제외된 항목
- ❌ **D1 (GUI)**: 현재 불필요
- ⏸️ **D2 (추가 물리 솔버)**: 필요 시 추가

### 포함된 항목
- ✅ **D1: MPI 병렬화** (HPC 확장)
- ✅ **D2: PINN 통합** (Physics-Informed Neural Networks)

---

## 📋 D1: MPI 병렬화 구현

### 개요
**목표**: 대규모 문제를 여러 노드/프로세스로 분산 처리
**난이도**: 🔴🔴 어려움
**예상 시간**: 2-3주 (80-120시간)
**우선순위**: ⭐⭐⭐ 높음

---

### D1.1: 도메인 분해 (Domain Decomposition)

**작업 시간**: 1주 (40시간)

#### 1.1.1: 1D 도메인 분해 (8시간)
```cpp
// 파일: include/koo/parallel/DomainDecomposition1D.h
class DomainDecomposition1D {
public:
    DomainDecomposition1D(int global_nx, int num_procs, int rank);

    // 각 프로세스의 로컬 도메인 정보
    int getLocalStart() const;
    int getLocalEnd() const;
    int getLocalSize() const;

    // 이웃 프로세스 정보
    int getLeftNeighbor() const;
    int getRightNeighbor() const;

    // 글로벌 ↔ 로컬 인덱스 변환
    int globalToLocal(int global_idx) const;
    int localToGlobal(int local_idx) const;

private:
    int global_nx_;
    int num_procs_;
    int rank_;
    int local_start_;
    int local_end_;
};
```

**구현 내용**:
- 균등 분할 (균형된 워크로드)
- 경계 셀 처리 (ghost cells)
- 글로벌/로컬 인덱스 매핑

#### 1.1.2: 2D/3D 도메인 분해 (16시간)
```cpp
// 파일: include/koo/parallel/DomainDecomposition2D.h
class DomainDecomposition2D {
public:
    DomainDecomposition2D(int global_nx, int global_ny,
                          int num_procs_x, int num_procs_y,
                          int rank);

    // 카테시안 그리드 토폴로지
    void setupCartesianTopology(MPI_Comm& cart_comm);

    // 이웃 프로세스 (상하좌우)
    int getNeighbor(Direction dir) const;  // NORTH, SOUTH, EAST, WEST

    // 헤일로 영역 (ghost cells)
    void allocateHaloRegions();
    std::vector<double>& getHaloBuffer(Direction dir);

private:
    MPI_Comm cart_comm_;
    int coords_[2];  // 카테시안 좌표
    std::map<Direction, int> neighbors_;
    std::map<Direction, std::vector<double>> halo_buffers_;
};
```

**구현 내용**:
- 2D 카테시안 그리드 분해
- MPI 카테시안 토폴로지 사용
- 4방향 이웃 통신
- 3D 확장 (6방향 이웃)

#### 1.1.3: 동적 부하 균형 (Dynamic Load Balancing) (16시간)
```cpp
// 파일: include/koo/parallel/LoadBalancer.h
class LoadBalancer {
public:
    // 작업량 측정
    double measureLocalWorkload();

    // 작업 재분배 필요성 판단
    bool needsRebalancing(double threshold = 0.1);

    // 도메인 재분배
    void rebalance(DomainDecomposition& decomp);

private:
    std::vector<double> workload_history_;
};
```

**구현 내용**:
- 각 프로세스의 작업량 모니터링
- 불균형 감지 (10% 이상 차이)
- 도메인 재분배 알고리즘

---

### D1.2: MPI 통신 계층 (Communication Layer)

**작업 시간**: 1주 (40시간)

#### 1.2.1: 기본 MPI 래퍼 (8시간)
```cpp
// 파일: include/koo/parallel/MPIWrapper.h
namespace koo::parallel {

class MPIEnvironment {
public:
    static void initialize(int argc, char** argv);
    static void finalize();
    static int getRank();
    static int getSize();
    static MPI_Comm getComm();
};

class MPIRequest {
public:
    void wait();
    bool test();

private:
    MPI_Request req_;
};

} // namespace koo::parallel
```

**구현 내용**:
- RAII 패턴으로 MPI 초기화/종료
- 에러 핸들링
- 비동기 통신 래퍼

#### 1.2.2: 경계 교환 (Boundary Exchange) (16시간)
```cpp
// 파일: include/koo/parallel/BoundaryExchange.h
class BoundaryExchange {
public:
    BoundaryExchange(const DomainDecomposition& decomp);

    // 동기 교환
    void exchangeSync(std::vector<double>& data);

    // 비동기 교환 (오버랩 가능)
    MPIRequest exchangeAsync(std::vector<double>& data);

    // 특화된 교환 패턴
    void exchangeHalo1D(std::vector<double>& data);
    void exchangeHalo2D(std::vector<std::vector<double>>& data);

private:
    void packSendBuffer(const std::vector<double>& data, Direction dir);
    void unpackRecvBuffer(std::vector<double>& data, Direction dir);

    std::map<Direction, std::vector<double>> send_buffers_;
    std::map<Direction, std::vector<double>> recv_buffers_;
};
```

**구현 내용**:
- 헤일로 셀 교환 (ghost cell communication)
- 동기/비동기 통신
- 통신-계산 오버랩 최적화

#### 1.2.3: 집합 통신 (Collective Operations) (16시간)
```cpp
// 파일: include/koo/parallel/CollectiveOps.h
namespace koo::parallel {

// 전역 감소 연산
template<typename T>
T allReduce(T local_value, MPI_Op op = MPI_SUM);

// 전역 합계
template<typename T>
T globalSum(T local_value);

// 전역 최대/최소
template<typename T>
T globalMax(T local_value);

template<typename T>
T globalMin(T local_value);

// 벡터 수집
template<typename T>
std::vector<T> gather(const std::vector<T>& local_data, int root = 0);

// 벡터 분산
template<typename T>
std::vector<T> scatter(const std::vector<T>& global_data, int root = 0);

} // namespace koo::parallel
```

**구현 내용**:
- MPI_Reduce, MPI_Allreduce 래퍼
- 벡터/배열 집합 통신
- 템플릿 기반 타입 안전성

---

### D1.3: MPI 솔버 구현

**작업 시간**: 1주 (40시간)

#### 1.3.1: MPI 확산 솔버 (16시간)
```cpp
// 파일: include/koo/parallel/MPIDiffusionSolver.h
class MPIDiffusionSolver {
public:
    MPIDiffusionSolver(const DomainDecomposition& decomp,
                       const BoundaryExchange& exchange);

    // 초기 조건 설정
    void setInitialCondition(std::function<double(double)> ic);

    // 시간 적분
    void step(double dt);

    // 결과 수집 (루트 프로세스)
    std::vector<double> gatherSolution(int root = 0);

private:
    // 로컬 계산 (내부 노드)
    void computeInterior();

    // 경계 통신
    void communicateBoundaries();

    // 경계 노드 계산
    void computeBoundaries();

    std::vector<double> local_data_;
    DomainDecomposition decomp_;
    BoundaryExchange exchange_;
};
```

**구현 내용**:
- 1D/2D/3D 확산 방정식
- 통신-계산 오버랩
- 강한 스케일링 (같은 문제, 더 많은 프로세스)

#### 1.3.2: MPI 반응-확산 솔버 (16시간)
```cpp
// 파일: include/koo/parallel/MPIReactionDiffusionSolver.h
class MPIReactionDiffusionSolver {
public:
    // 다중 종 시스템
    void addSpecies(const std::string& name, double diffusivity);
    void addReaction(const Reaction& reaction);

    // 연산자 분리 (Operator Splitting)
    void stepReaction(double dt);   // 로컬 계산 (통신 없음)
    void stepDiffusion(double dt);  // MPI 통신 필요

private:
    std::map<std::string, std::vector<double>> species_data_;
    std::vector<Reaction> reactions_;
};
```

**구현 내용**:
- 다중 종 반응-확산
- 연산자 분리 (반응은 로컬, 확산은 MPI)
- 효율적인 통신 패턴

#### 1.3.3: 병렬 I/O (8시간)
```cpp
// 파일: include/koo/parallel/ParallelIO.h
class ParallelIO {
public:
    // MPI-IO를 사용한 병렬 쓰기
    void writeVTK(const std::string& filename,
                  const DomainDecomposition& decomp,
                  const std::vector<double>& local_data);

    // HDF5 병렬 I/O
    void writeHDF5(const std::string& filename,
                   const std::map<std::string, std::vector<double>>& datasets);

    // 체크포인트
    void writeCheckpoint(const std::string& filename,
                         const SimulationState& state);

    void readCheckpoint(const std::string& filename,
                        SimulationState& state);
};
```

**구현 내용**:
- MPI-IO 기반 병렬 파일 쓰기
- VTK parallel format (pvtu)
- HDF5 병렬 I/O

---

### D1.4: 성능 최적화 및 벤치마킹

**작업 시간**: 3-5일 (24-40시간)

#### 1.4.1: 통신 최적화 (8시간)
- **비블로킹 통신**: MPI_Isend/MPI_Irecv
- **통신-계산 오버랩**: 내부 노드 먼저 계산, 통신 대기 중 경계 계산
- **메시지 집합**: 여러 작은 메시지 → 하나의 큰 메시지

```cpp
// 최적화 전
exchangeHalo();  // 블로킹
computeInterior();

// 최적화 후
auto req = exchangeHaloAsync();  // 비블로킹
computeInterior();               // 통신 중 계산
req.wait();                      // 통신 완료 대기
computeBoundaries();             // 경계 계산
```

#### 1.4.2: 스케일링 벤치마크 (8시간)
```cpp
// 파일: benchmarks/mpi_scaling_benchmark.cpp
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    // 강한 스케일링 (문제 크기 고정)
    benchmarkStrongScaling();

    // 약한 스케일링 (프로세스당 문제 크기 고정)
    benchmarkWeakScaling();

    // 통신 오버헤드 측정
    benchmarkCommunicationOverhead();

    MPI_Finalize();
    return 0;
}
```

**측정 항목**:
- **강한 스케일링**: 이상적으로 N배 빠름 (N = 프로세스 수)
- **약한 스케일링**: 이상적으로 시간 일정
- **병렬 효율성**: Speedup / N
- **통신 오버헤드**: 통신 시간 / 전체 시간

#### 1.4.3: 프로파일링 (8시간)
- **MPI 프로파일러 통합**: mpiP, TAU, Vampir
- **통신 패턴 분석**: 어느 부분에서 통신이 병목?
- **부하 불균형 감지**: 프로세스별 작업 시간 비교

---

### D1.5: 문서화 및 예제

**작업 시간**: 2-3일 (16-24시간)

#### 파일 생성
1. **MPI_GUIDE.md** (8시간)
   - MPI 설치 및 설정
   - 컴파일 방법 (`mpicc`, `mpic++`)
   - 실행 방법 (`mpirun`, `mpiexec`)
   - SLURM/PBS 스크립트
   - 성능 튜닝 팁
   - 문제 해결 가이드

2. **examples/mpi_diffusion_example.cpp** (4시간)
   - 간단한 1D 확산 예제
   - 도메인 분해 시각화
   - 성능 측정

3. **examples/mpi_reaction_diffusion_example.cpp** (4시간)
   - 2D Gray-Scott 모델
   - 패턴 형성 시뮬레이션
   - VTK 출력

---

### D1.6: 테스트

**작업 시간**: 3-4일 (24-32시간)

#### 테스트 케이스
```cpp
// tests/parallel/test_mpi_diffusion.cpp
TEST(MPIDiffusion, StrongScaling) {
    // 1, 2, 4, 8 프로세스로 같은 문제 실행
    // 결과가 동일한지 확인
}

TEST(MPIDiffusion, WeakScaling) {
    // 프로세스당 같은 크기 문제 실행
    // 시간이 일정한지 확인
}

TEST(BoundaryExchange, Correctness) {
    // 경계 교환 후 헤일로 셀 값 검증
}

TEST(DomainDecomposition, LoadBalance) {
    // 각 프로세스가 비슷한 크기 도메인을 받는지
}
```

---

### D1 요약

| 작업 | 시간 | 우선순위 |
|------|------|----------|
| 도메인 분해 | 40h | ⭐⭐⭐ |
| MPI 통신 계층 | 40h | ⭐⭐⭐ |
| MPI 솔버 | 40h | ⭐⭐⭐ |
| 성능 최적화 | 24-40h | ⭐⭐ |
| 문서화 | 16-24h | ⭐⭐ |
| 테스트 | 24-32h | ⭐⭐ |
| **총계** | **184-216h** | **2.5-3주** |

---

## 🧠 D2: PINN (Physics-Informed Neural Networks) 통합

### 개요
**목표**: 신경망을 사용하여 PDE 해를 근사하고 물리 법칙을 손실 함수에 포함
**난이도**: 🔴🔴🔴 매우 어려움
**예상 시간**: 2-3주 (80-120시간)
**우선순위**: ⭐⭐ 중간

---

### D2.1: 딥러닝 프레임워크 통합

**작업 시간**: 3-4일 (24-32시간)

#### 2.1.1: PyTorch C++ API (LibTorch) 통합 (16시간)
```cpp
// 파일: include/koo/ml/TorchInterface.h
#include <torch/torch.h>

namespace koo::ml {

class NeuralNetwork : public torch::nn::Module {
public:
    NeuralNetwork(int input_dim, int output_dim,
                  std::vector<int> hidden_layers);

    torch::Tensor forward(torch::Tensor x);

private:
    torch::nn::Sequential network_;
};

} // namespace koo::ml
```

**구현 내용**:
- CMake에 LibTorch 통합
- 기본 신경망 구조 (fully connected)
- GPU 지원 (CUDA)

#### 2.1.2: Python-C++ 인터페이스 (8시간)
```python
# 파일: python/koolab/ml/pinn.py
import torch
import torch.nn as nn

class PINN(nn.Module):
    def __init__(self, layers):
        super().__init__()
        self.network = self._build_network(layers)

    def forward(self, x, t):
        inputs = torch.cat([x, t], dim=1)
        return self.network(inputs)

    def physics_loss(self, x, t):
        # PDE 잔차 계산
        u = self.forward(x, t)
        u_x = torch.autograd.grad(u, x, ...)[0]
        u_t = torch.autograd.grad(u, t, ...)[0]
        u_xx = torch.autograd.grad(u_x, x, ...)[0]

        # 확산 방정식: u_t = D * u_xx
        pde_residual = u_t - self.D * u_xx
        return torch.mean(pde_residual ** 2)
```

---

### D2.2: PINN 솔버 구현

**작업 시간**: 1.5주 (60시간)

#### 2.2.1: 1D 확산 PINN (20시간)
```python
# 파일: python/koolab/ml/diffusion_pinn.py
class DiffusionPINN(PINN):
    def __init__(self, diffusivity):
        super().__init__(layers=[2, 50, 50, 50, 1])
        self.D = diffusivity

    def loss_function(self, x_pde, t_pde, x_ic, u_ic, x_bc, u_bc):
        # 물리 손실 (PDE 잔차)
        loss_pde = self.physics_loss(x_pde, t_pde)

        # 초기 조건 손실
        u_pred_ic = self.forward(x_ic, torch.zeros_like(x_ic))
        loss_ic = torch.mean((u_pred_ic - u_ic) ** 2)

        # 경계 조건 손실
        u_pred_bc = self.forward(x_bc, t_pde)
        loss_bc = torch.mean((u_pred_bc - u_bc) ** 2)

        # 가중 조합
        return loss_pde + loss_ic + loss_bc

    def train(self, epochs=10000, lr=1e-3):
        optimizer = torch.optim.Adam(self.parameters(), lr=lr)

        for epoch in range(epochs):
            optimizer.zero_grad()
            loss = self.loss_function(...)
            loss.backward()
            optimizer.step()

            if epoch % 100 == 0:
                print(f"Epoch {epoch}: Loss = {loss.item()}")
```

**구현 내용**:
- 1D 확산 방정식
- 초기 조건 + 경계 조건 강제
- Adam 최적화
- 자동 미분 (autograd)

#### 2.2.2: 2D 확산 PINN (20시간)
```python
# 파일: python/koolab/ml/diffusion_2d_pinn.py
class Diffusion2DPINN(PINN):
    def __init__(self):
        super().__init__(layers=[3, 100, 100, 100, 1])  # (x, y, t) -> u

    def physics_loss(self, x, y, t):
        u = self.forward(x, y, t)

        # 1차 도함수
        u_x = autograd.grad(u, x, ...)[0]
        u_y = autograd.grad(u, y, ...)[0]
        u_t = autograd.grad(u, t, ...)[0]

        # 2차 도함수
        u_xx = autograd.grad(u_x, x, ...)[0]
        u_yy = autograd.grad(u_y, y, ...)[0]

        # 2D 확산: u_t = D * (u_xx + u_yy)
        pde_residual = u_t - self.D * (u_xx + u_yy)
        return torch.mean(pde_residual ** 2)
```

#### 2.2.3: 반응-확산 PINN (20시간)
```python
# 파일: python/koolab/ml/reaction_diffusion_pinn.py
class ReactionDiffusionPINN(PINN):
    def __init__(self):
        super().__init__(layers=[2, 50, 50, 50, 2])  # (x, t) -> (u, v)

    def physics_loss(self, x, t):
        uv = self.forward(x, t)
        u = uv[:, 0:1]
        v = uv[:, 1:2]

        # 도함수
        u_t = autograd.grad(u, t, ...)[0]
        v_t = autograd.grad(v, t, ...)[0]
        u_xx = autograd.grad(autograd.grad(u, x, ...)[0], x, ...)[0]
        v_xx = autograd.grad(autograd.grad(v, x, ...)[0], x, ...)[0]

        # Gray-Scott 모델
        uvv = u * v * v
        f_u = self.Du * u_xx - uvv + self.F * (1 - u)
        f_v = self.Dv * v_xx + uvv - (self.F + self.k) * v

        residual_u = u_t - f_u
        residual_v = v_t - f_v

        return torch.mean(residual_u ** 2 + residual_v ** 2)
```

---

### D2.3: 고급 PINN 기법

**작업 시간**: 1주 (40시간)

#### 2.3.1: 적응형 샘플링 (Adaptive Sampling) (12시간)
```python
class AdaptivePINN(PINN):
    def compute_residual_importance(self, x, t):
        # PDE 잔차가 큰 곳에 더 많은 샘플링 포인트
        with torch.no_grad():
            residual = self.physics_loss(x, t, reduction='none')
        return residual.abs()

    def adaptive_resample(self, n_points):
        # 잔차 기반 확률 분포로 재샘플링
        importance = self.compute_residual_importance(self.x_pde, self.t_pde)
        prob = importance / importance.sum()
        indices = torch.multinomial(prob, n_points, replacement=True)
        return self.x_pde[indices], self.t_pde[indices]
```

#### 2.3.2: 역문제 (Inverse Problems) (16시간)
```python
class InversePINN(PINN):
    def __init__(self):
        super().__init__(...)
        # 물리 파라미터도 학습 가능하게
        self.D = nn.Parameter(torch.tensor([0.1]))  # 확산 계수 추정
        self.k = nn.Parameter(torch.tensor([0.05])) # 반응 속도 추정

    def train_inverse(self, x_data, t_data, u_data):
        # 측정 데이터로부터 파라미터 추정
        optimizer = torch.optim.Adam(self.parameters(), lr=1e-3)

        for epoch in range(epochs):
            # 데이터 피팅 손실
            u_pred = self.forward(x_data, t_data)
            loss_data = torch.mean((u_pred - u_data) ** 2)

            # 물리 손실
            loss_pde = self.physics_loss(x_pde, t_pde)

            # 총 손실
            loss = loss_data + lambda_pde * loss_pde

            loss.backward()
            optimizer.step()
```

#### 2.3.3: 멀티피델리티 PINN (12시간)
```python
class MultiFidelityPINN:
    def __init__(self):
        self.low_fidelity_model = SimplePINN(layers=[2, 20, 20, 1])
        self.high_fidelity_model = ComplexPINN(layers=[2, 100, 100, 100, 1])

    def train_multifidelity(self):
        # 1단계: 저정밀 모델 빠르게 학습
        self.low_fidelity_model.train(epochs=1000)

        # 2단계: 저정밀 결과를 초기화로 사용
        self.high_fidelity_model.load_pretrained(self.low_fidelity_model)

        # 3단계: 고정밀 모델 미세 조정
        self.high_fidelity_model.train(epochs=5000)
```

---

### D2.4: C++ 통합 및 하이브리드 솔버

**작업 시간**: 4-5일 (32-40시간)

#### 2.4.1: C++에서 PINN 모델 로드 (16시간)
```cpp
// 파일: include/koo/ml/PINNSolver.h
#include <torch/script.h>

namespace koo::ml {

class PINNSolver {
public:
    // Python에서 학습한 모델 로드
    void loadModel(const std::string& model_path);

    // 예측
    std::vector<double> predict(const std::vector<double>& x,
                                 const std::vector<double>& t);

    // 배치 예측 (효율적)
    torch::Tensor predictBatch(torch::Tensor x, torch::Tensor t);

private:
    torch::jit::script::Module model_;
    torch::Device device_;
};

} // namespace koo::ml
```

#### 2.4.2: 하이브리드 솔버 (16시간)
```cpp
// 파일: include/koo/ml/HybridSolver.h
class HybridSolver {
public:
    HybridSolver(std::shared_ptr<PDESolver> pde_solver,
                 std::shared_ptr<PINNSolver> pinn_solver);

    // 거친 그리드에서 PINN으로 빠른 근사
    void coarseApproximation(PINNSolver& pinn);

    // 세밀한 그리드에서 PDE 솔버로 정확한 계산
    void fineRefinement(PDESolver& pde);

    // 멀티스케일: PINN(거시) + PDE(미시)
    void multiscaleSolve();

private:
    std::shared_ptr<PDESolver> pde_solver_;
    std::shared_ptr<PINNSolver> pinn_solver_;
};
```

---

### D2.5: 문서화 및 예제

**작업 시간**: 3-4일 (24-32시간)

#### 파일 생성
1. **PINN_GUIDE.md** (12시간)
   - PINN 이론 소개
   - PyTorch/LibTorch 설치
   - 학습 방법
   - 하이퍼파라미터 튜닝
   - 역문제 예제
   - C++ 통합 방법

2. **notebooks/05_pinn_diffusion.ipynb** (4시간)
   - 1D 확산 PINN 학습
   - 전통적 수치 해법과 비교
   - 시각화

3. **notebooks/06_pinn_inverse_problem.ipynb** (4시간)
   - 확산 계수 추정
   - 노이즈 있는 데이터 처리
   - 불확실성 정량화

4. **examples/pinn_hybrid_example.cpp** (4시간)
   - C++에서 PINN 모델 로드
   - PDE 솔버와 결합
   - 성능 비교

---

### D2.6: 테스트 및 검증

**작업 시간**: 3-4일 (24-32시간)

#### 테스트 케이스
```python
# tests/ml/test_pinn.py
def test_pinn_diffusion_accuracy():
    # PINN 해가 해석해와 일치하는지
    pinn = DiffusionPINN(D=0.1)
    pinn.train(epochs=5000)

    # 해석해
    u_exact = analytical_solution(x, t)
    u_pred = pinn.forward(x, t)

    error = torch.mean((u_pred - u_exact) ** 2)
    assert error < 1e-3

def test_inverse_problem():
    # 알려진 파라미터로 데이터 생성
    true_D = 0.1
    data = generate_data(true_D)

    # PINN으로 파라미터 추정
    pinn = InversePINN()
    pinn.train_inverse(data)

    # 추정값 검증
    assert abs(pinn.D.item() - true_D) < 0.01
```

---

### D2 요약

| 작업 | 시간 | 우선순위 |
|------|------|----------|
| PyTorch 통합 | 24-32h | ⭐⭐⭐ |
| PINN 솔버 구현 | 60h | ⭐⭐⭐ |
| 고급 PINN 기법 | 40h | ⭐⭐ |
| C++ 통합 | 32-40h | ⭐⭐ |
| 문서화 | 24-32h | ⭐⭐ |
| 테스트 | 24-32h | ⭐⭐ |
| **총계** | **204-236h** | **2.5-3주** |

---

## 📊 Priority D 전체 일정

### 총 예상 시간
- **D1 (MPI)**: 184-216시간 (2.5-3주)
- **D2 (PINN)**: 204-236시간 (2.5-3주)
- **총계**: **388-452시간** (5-6주)

### 추천 진행 순서

#### 옵션 1: 순차 진행 (안전)
```
주 1-3:  D1 (MPI) 완료
주 4-6:  D2 (PINN) 완료
```

#### 옵션 2: 병렬 진행 (빠름, 리소스 2배)
```
주 1-3:  D1 + D2 동시 진행
```

#### 옵션 3: 단계별 진행 (추천)
```
주 1-2:  D1.1-D1.3 (MPI 핵심 기능)
주 3:    D2.1-D2.2 (PINN 기본 구현)
주 4:    D1.4-D1.5 (MPI 최적화 + 문서)
주 5:    D2.3-D2.4 (PINN 고급 기능 + C++ 통합)
주 6:    D1.6 + D2.6 (테스트 및 검증)
```

---

## 🎯 마일스톤

### 마일스톤 1: MPI 기본 기능 (2주)
- ✅ 도메인 분해
- ✅ 경계 교환
- ✅ MPI 확산 솔버
- ✅ 기본 예제

### 마일스톤 2: PINN 기본 구현 (1주)
- ✅ PyTorch 통합
- ✅ 1D/2D 확산 PINN
- ✅ 학습 파이프라인

### 마일스톤 3: 고급 기능 (2주)
- ✅ MPI 성능 최적화
- ✅ PINN 고급 기법
- ✅ C++ 통합

### 마일스톤 4: 검증 및 문서화 (1주)
- ✅ 전체 테스트
- ✅ 성능 벤치마크
- ✅ 완전한 문서

---

## 📝 생성될 파일 목록

### D1 (MPI) - 약 20개 파일
**헤더 파일**:
- `include/koo/parallel/DomainDecomposition1D.h`
- `include/koo/parallel/DomainDecomposition2D.h`
- `include/koo/parallel/DomainDecomposition3D.h`
- `include/koo/parallel/MPIWrapper.h`
- `include/koo/parallel/BoundaryExchange.h`
- `include/koo/parallel/CollectiveOps.h`
- `include/koo/parallel/LoadBalancer.h`
- `include/koo/parallel/MPIDiffusionSolver.h`
- `include/koo/parallel/MPIReactionDiffusionSolver.h`
- `include/koo/parallel/ParallelIO.h`

**소스 파일**:
- `src/parallel/*.cpp` (10개)

**예제**:
- `examples/mpi_diffusion_example.cpp`
- `examples/mpi_reaction_diffusion_example.cpp`

**벤치마크**:
- `benchmarks/mpi_scaling_benchmark.cpp`

**문서**:
- `MPI_GUIDE.md`

**테스트**:
- `tests/parallel/test_*.cpp` (5-6개)

### D2 (PINN) - 약 15개 파일
**Python 모듈**:
- `python/koolab/ml/__init__.py`
- `python/koolab/ml/pinn.py`
- `python/koolab/ml/diffusion_pinn.py`
- `python/koolab/ml/diffusion_2d_pinn.py`
- `python/koolab/ml/reaction_diffusion_pinn.py`
- `python/koolab/ml/inverse_pinn.py`

**C++ 헤더**:
- `include/koo/ml/TorchInterface.h`
- `include/koo/ml/PINNSolver.h`
- `include/koo/ml/HybridSolver.h`

**C++ 소스**:
- `src/ml/*.cpp` (3개)

**노트북**:
- `notebooks/05_pinn_diffusion.ipynb`
- `notebooks/06_pinn_inverse_problem.ipynb`

**예제**:
- `examples/pinn_hybrid_example.cpp`

**문서**:
- `PINN_GUIDE.md`

**테스트**:
- `tests/ml/test_*.py` (4-5개)

---

## ✅ 성공 기준

### D1 (MPI)
- ✅ 4개 이상 프로세스에서 올바른 결과
- ✅ 강한 스케일링 효율 > 70% (8 프로세스)
- ✅ 약한 스케일링 효율 > 90%
- ✅ 통신 오버헤드 < 20%
- ✅ 1000+ 노드에서 테스트 (HPC 클러스터)

### D2 (PINN)
- ✅ 1D 확산: L2 오차 < 1e-3
- ✅ 2D 확산: L2 오차 < 5e-3
- ✅ 역문제: 파라미터 추정 오차 < 5%
- ✅ GPU 가속: CPU 대비 10x 이상
- ✅ C++에서 모델 로드 및 추론 성공

---

## 🚀 다음 단계

Priority D 작업을 시작할까요?

1. **D1 (MPI)부터 시작**: HPC 병렬화가 우선
2. **D2 (PINN)부터 시작**: 머신러닝이 우선
3. **동시 진행**: 두 작업 병렬로 (리소스 있을 경우)

어떤 것을 먼저 하시겠습니까?
