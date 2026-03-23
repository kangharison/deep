# BaM Block Benchmark - CMake 빌드 설정
# =======================================
# GPU-NVMe 블록 I/O 벤치마크 빌드 정의.
#
# make_benchmark: CUDA 소스를 컴파일하여 벤치마크 바이너리를 생성한다.
#   - block-benchmark-module: main.cu를 컴파일한 모듈
#   - block-bench: 생성되는 실행 파일 이름
#
# make_benchmark_choice: 빌드 환경에 따라 적절한 타겟을 선택한다.
#   - block-benchmark: 일반 빌드 타겟
#   - block-benchmark-smartio: Dolphin SmartIO 지원 빌드 타겟
#   - block-benchmark-module: 소스 모듈
#
# 빌드 결과: build/bin/nvm-block-bench

cmake_minimum_required (VERSION 3.1)
project (libnvm-benchmarks)

# POSIX 스레드 라이브러리를 찾는다 (pthread)
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# CUDA 8.0 이상을 필수로 요구한다 (GPU 커널 컴파일에 필요)
find_package (CUDA 8.0 REQUIRED)


# make_benchmark 매크로: CUDA 소스를 컴파일하여 벤치마크 모듈을 만든다
# 인자: (모듈이름, 실행파일이름, "소스파일;")
make_benchmark (block-benchmark-module block-bench "main.cu;")

# make_benchmark_choice 매크로: SmartIO(DIS 클러스터) 지원 여부에 따라
# block-benchmark 또는 block-benchmark-smartio 타겟 중 하나를 활성화한다
make_benchmark_choice (block-benchmark block-benchmark-smartio block-benchmark-module)
