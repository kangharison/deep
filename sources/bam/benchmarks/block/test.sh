#!/usr/bin/env bash
# BaM Block Benchmark - 안정성 테스트 (스트레스 테스트)
# ====================================================
# 동일한 설정으로 벤치마크를 100,000회 반복 실행하여
# GPU-NVMe 직접 I/O 경로의 안정성과 메모리 누수 여부를 검증한다.
#
# 테스트 설정:
#   threads    = 1,048,576 (1M 스레드 = 1024*256*4)
#   blk_size   = 64 (CUDA 블록당 64 스레드 = warp 2개)
#   reqs       = 1 (스레드당 1회 I/O)
#   pages      = 1,048,576 (1M 페이지, 스레드와 1:1 매핑)
#   page_size  = 512 (바이트)
#   queue_depth= 1024
#   num_queues = 128
#   num_blks   = 2,097,152 (2M 블록)
#   gpu        = 0
#   n_ctrls    = 1 (NVMe 컨트롤러 1개)
#   random     = true (기본값, 랜덤 접근)
#
# 총 I/O: 1M 스레드 × 512B = 512MB per iteration
# 반복 100,000회 → 약 50TB 읽기

set -euo pipefail

for i in {1..100000}
do
    ../../build/bin/nvm-block-bench --threads=$((1024*256*4)) --blk_size=64 --reqs=1 --pages=$((256*1024*4)) --queue_depth=1024  --page_size=$((512)) --num_blks=$((2097152)) --gpu=0 --n_ctrls=1 --num_queues=128

    echo "******************** $i *********************"
done
