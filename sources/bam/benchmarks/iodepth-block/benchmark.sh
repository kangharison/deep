#!/usr/bin/env bash
# BaM I/O Depth Block Benchmark - 자동화 스크립트
# ================================================
# 지정된 NVMe 컨트롤러 수에서 스레드 수를 변화시키며 블록 I/O 벤치마크를 실행한다.
# block/benchmark.sh와 유사하지만 단일 컨트롤러 수만 테스트하며,
# 순차 접근(R=0)으로 고정되어 있다.
#
# 사용법: ./benchmark.sh <NVMe컨트롤러수>
# 예시:   ./benchmark.sh 4
#         → GPU 1번에서 4개 컨트롤러로 순차 접근 벤치마크 실행
#
# 고정 파라미터:
#   P=512      : 페이지 크기 = 512바이트
#   R=0        : access_type = READ (읽기 전용, 순차)
#   B=64       : CUDA 블록 크기 = 64
#   G=1        : GPU 디바이스 번호 = 1
#   A=0        : access_type = 0 (READ)
#   NB=128M    : backing 블록 수 = 128Mi
#   큐 깊이=1024, 큐 수=128
#
# 주의: 이 스크립트는 nvm-block-bench를 호출하고 있다 (iodepth 버전이 아님).
# I/O depth 테스트를 하려면 실행 파일을 nvm-iodepth-block-bench로 변경하고
# --reqs 옵션을 1~4로 설정해야 한다.

[ $# -ne 1 ] && echo Usage $0 numssd  && exit 1

set -eux
P=512                           # 페이지 크기(바이트)
R=0                             # 순차 접근 (access_type=READ)
B=64                            # CUDA 블록 크기
G=1                             # GPU 디바이스 번호
A=0                             # 접근 유형: READ
RT=50                           # mixed 읽기 비율(%) - 미사용
NB=$((128*1024*1024))           # backing 블록 수: 128Mi

# 지정된 컨트롤러 수에서 스레드 수를 1K~256K까지 변화
for C in $1
do
    echo "++++++++++++++++++ $C Controllers ++++++++++++++++++"
    for T in 1024 2048 4096 8192 16384 32768 65536 131072 262144
    do
        echo "------------------ $T Threads ------------------"
        ../../build/bin/nvm-block-bench --threads=$T --blk_size=$B --reqs=1 --pages=$T --queue_depth=1024 --num_queues=128 --page_size=$P --n_ctrls=$C --gpu=$G --num_blks=$NB --access_type=$R
    done

done
