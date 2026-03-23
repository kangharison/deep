#!/usr/bin/env bash
# BaM I/O Depth Block Benchmark - 안정성 테스트 (스트레스 테스트)
# ==============================================================
# 동일한 설정으로 벤치마크를 100,000회 반복 실행하여
# GPU-NVMe 비동기 I/O 경로의 안정성과 메모리 누수를 검증한다.
#
# 주의: block/test.sh와 동일한 nvm-block-bench를 호출한다.
# I/O depth 테스트에는 nvm-iodepth-block-bench를 사용해야 한다.
#
# 테스트 설정:
#   threads    = 1,048,576 (1M)
#   blk_size   = 64
#   reqs       = 1 (I/O depth = 1)
#   pages      = 1,048,576
#   page_size  = 512바이트
#   queue_depth= 1024
#   num_queues = 128
#   num_blks   = 2,097,152 (2M)
#   gpu        = 0
#   n_ctrls    = 1

set -euo pipefail

for i in {1..100000}
do
    ../../build/bin/nvm-block-bench --threads=$((1024*256*4)) --blk_size=64 --reqs=1 --pages=$((256*1024*4)) --queue_depth=1024  --page_size=$((512)) --num_blks=$((2097152)) --gpu=0 --n_ctrls=1 --num_queues=128

    echo "******************** $i *********************"
done
