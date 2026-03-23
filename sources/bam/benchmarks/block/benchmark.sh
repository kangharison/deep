#!/usr/bin/env bash
# BaM Block Benchmark - 자동화 스크립트
# ======================================
# NVMe 컨트롤러 수와 CUDA 스레드 수를 변화시키며 블록 I/O 벤치마크를 실행한다.
# 컨트롤러 스케일링(1~N개)과 스레드 스케일링(1K~33M)을 동시에 측정하여
# GPU-NVMe 직접 I/O의 확장성(scalability)을 평가할 수 있다.
#
# 사용법: ./benchmark.sh <NVMe컨트롤러수> <GPU번호> <CUDA블록크기>
# 예시:   ./benchmark.sh 4 0 64
#         → GPU 0번에서 블록크기 64로, 1~4개 컨트롤러를 순회하며 벤치마크 실행
#
# 고정 파라미터:
#   P=512      : 페이지(I/O 요청) 크기 = 512바이트
#   R=1        : 스레드당 요청 반복 횟수 = 1회
#   A=0        : 접근 유형 = READ (읽기 전용)
#   RT=50      : mixed 모드 읽기 비율 50% (현재 A=0이므로 미사용)
#   NB=128M    : backing 배열 블록 수 = 128M (512B 페이지 기준 약 64GB SSD 영역)
#   큐 깊이=1024, 큐 수=135: 높은 병렬성을 위한 설정
#
# 스레드 수 스케일링: 1K → 2K → 4K → ... → 33M (2의 거듭제곱 + 일부)
#   각 스레드가 하나의 512B I/O 요청을 발생시키므로 총 I/O 수 = 스레드 수

set -x

if [ $# -ne 3 ]
then
	echo Usage $0 numssd gpuid tbsize && exit 1
fi

set -eux
P=512                           # 페이지 크기(바이트): 512B = NVMe 최소 LBA 크기
R=1                             # 스레드당 요청 반복 횟수
B=$3                            # CUDA 블록 크기 (인자에서 받음)
G=$2                            # GPU 디바이스 번호 (인자에서 받음)
A=0                             # 접근 유형: 0=READ
RT=50                           # mixed 읽기 비율(%) - A=0이면 무시됨
NB=$((128*1024*1024))           # backing 블록 수: 128Mi = 134,217,728
CTRLS=$1                        # 최대 NVMe 컨트롤러 수 (인자에서 받음)

# 외부 루프: 컨트롤러 수를 1부터 CTRLS까지 증가
for ((C=1; C<=$CTRLS; C++))
do
    echo "++++++++++++++++++ $C Controllers ++++++++++++++++++"
    # 내부 루프: 스레드 수를 1K부터 33M까지 2배씩 증가
    # pages=threads로 설정하여 1:1 매핑 (각 스레드가 고유 캐시 슬롯 사용)
    for T in 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432
    do
        echo "------------------ $T Threads ------------------"
        ../../build/bin/nvm-block-bench --threads=$T --blk_size=$B --reqs=1 --pages=$T --queue_depth=1024 --num_queues=135 --page_size=$P --n_ctrls=$C --gpu=$G --num_blks=$NB --access_type=$A --random=$R
    done
done
