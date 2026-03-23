# BaM I/O Depth Block Benchmark - CMake 빌드 설정
# =================================================
# I/O depth를 조절하는 GPU-NVMe 블록 I/O 벤치마크 빌드 정의.
#
# 빌드 결과: build/bin/nvm-iodepth-block-bench
#
# block 벤치마크와 동일한 빌드 구조이지만 별도의 실행 파일로 생성된다.

cmake_minimum_required (VERSION 3.1)
project (libnvm-benchmarks)

# POSIX 스레드 라이브러리 (pthread)
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# CUDA 8.0 이상 필수
find_package (CUDA 8.0 REQUIRED)


# make_benchmark: main.cu를 컴파일하여 iodepth-block-bench 실행 파일 생성
make_benchmark (iodepth-block-benchmark-module iodepth-block-bench "main.cu;")

# SmartIO(DIS 클러스터) 지원 여부에 따라 빌드 타겟 선택
make_benchmark_choice (iodepth-block-benchmark iodepth-block-benchmark-smartio iodepth-block-benchmark-module)
