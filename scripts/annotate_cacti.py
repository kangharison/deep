#!/usr/bin/env python3
"""
CACTI 소스코드에 한글 주석을 추가하는 스크립트
캐시/메모리 면적, 타이밍, 전력 모델링 도구인 CACTI의 모든 파일에
line-by-line 한글 주석을 추가한다.
"""

import re
import os

CACTI_DIR = "/home/ubuntu/deep/sources/gpgpu-sim_distribution/src/accelwattch/cacti"

# ============================================================
# 주석 매핑: 파일별로 (패턴, 주석) 리스트
# 패턴이 라인에 포함되면 해당 주석을 줄 끝에 추가
# ============================================================

# 공통 패턴 (모든 파일에 적용)
COMMON_PATTERNS = [
    # 헤더 가드
    (r'^#ifndef\s+__(\w+)_H__', '// 헤더 가드 시작 - 중복 포함 방지'),
    (r'^#define\s+__(\w+)_H__', '// 헤더 가드 매크로 정의'),
    (r'^#endif\s*$', '// 조건부 컴파일 종료'),
    (r'^#endif\s*//', None),  # 이미 주석이 있으면 건너뜀
    # include 문
    (r'#include\s+"area\.h"', '// 면적(Area) 클래스 정의 헤더'),
    (r'#include\s+"component\.h"', '// Component 기본 클래스 (delay, power, area 포함)'),
    (r'#include\s+"parameter\.h"', '// 기술 파라미터(TechnologyParameter) 및 동적 파라미터(DynamicParameter) 정의'),
    (r'#include\s+"wire\.h"', '// 배선(Wire) 모델 헤더 - 저항/커패시턴스/지연 계산'),
    (r'#include\s+"decoder\.h"', '// 디코더(Decoder), 프리디코더(Predec) 클래스 헤더'),
    (r'#include\s+"subarray\.h"', '// 서브어레이(Subarray) 클래스 - SRAM/DRAM 셀 배열 모델'),
    (r'#include\s+"mat\.h"', '// Mat(매트) 클래스 - 서브어레이 + 디코더 + 센스앰프 조합 단위'),
    (r'#include\s+"bank\.h"', '// Bank 클래스 - 여러 Mat + H-tree로 구성된 뱅크'),
    (r'#include\s+"htree2\.h"', '// H-tree 배선 모델 - 주소/데이터 분배 트리 구조'),
    (r'#include\s+"cacti_interface\.h"', '// CACTI 외부 인터페이스 - InputParameter, powerDef, uca_org_t 등'),
    (r'#include\s+"basic_circuit\.h"', '// 기본 회로 함수 - gate_C, drain_C_, tr_R_on, horowitz 등 VLSI 기초'),
    (r'#include\s+"const\.h"', '// 상수 정의 - BIGNUM, MAX_NUMBER_GATES_STAGE 등'),
    (r'#include\s+"router\.h"', '// NoC 라우터 모델 - 크로스바, 아비터, 버퍼 포함'),
    (r'#include\s+"nuca\.h"', '// NUCA(Non-Uniform Cache Architecture) 모델'),
    (r'#include\s+"uca\.h"', '// UCA(Uniform Cache Architecture) 모델'),
    (r'#include\s+"crossbar\.h"', '// 크로스바 스위치 모델 - tri-state 버퍼 기반'),
    (r'#include\s+"arbiter\.h"', '// 아비터(중재기) 모델 - 라운드로빈 방식'),
    (r'#include\s+"io\.h"', '// I/O 출력 함수 - 결과 CSV/텍스트 출력'),
    (r'#include\s+"Ucache\.h"', '// 통합 캐시 탐색 엔진 - calculate_time, solve 함수'),
    (r'#include\s+<pthread\.h>', '// POSIX 스레드 라이브러리 - 멀티스레드 탐색에 사용'),
    (r'#include\s+<assert\.h>', '// 디버그용 assert 매크로'),
    (r'#include\s+<iostream>', '// C++ 표준 입출력 스트림'),
    (r'#include\s+<math\.h>', '// 수학 함수 라이브러리 (sqrt, log, pow 등)'),
    (r'#include\s+<time\.h>', '// 시간 관련 함수'),
    (r'#include\s+<algorithm>', '// STL 알고리즘 (sort, merge 등)'),
    (r'#include\s+<list>', '// STL 리스트 컨테이너'),
    (r'#include\s+<vector>', '// STL 벡터 컨테이너'),
    (r'#include\s+<map>', '// STL 맵 컨테이너'),
    (r'#include\s+<string>', '// STL 문자열 클래스'),
    (r'^using namespace std;', '// std 네임스페이스 사용 선언'),
]


def has_korean_comment(line):
    """이미 한글 주석이 있는지 확인"""
    # 문자열 리터럴이 아닌 // 뒤에 한글이 있는지 확인
    comment_match = re.search(r'//.*[\uac00-\ud7af]', line)
    return comment_match is not None


def is_license_or_blank(line, line_no, in_license):
    """라이선스 블록이나 빈 줄인지 확인"""
    stripped = line.strip()
    if stripped.startswith('/***') or stripped.startswith(' *') or stripped.startswith(' */') or stripped.startswith('***'):
        return True
    return False


def add_comment(line, comment):
    """줄 끝에 한글 주석 추가 (이미 있으면 건너뜀)"""
    if has_korean_comment(line):
        return line
    # 기존 주석이 있는 경우 주석 뒤에 추가하지 않음 (줄이 너무 길어짐)
    rstripped = line.rstrip('\n')
    if not rstripped.strip():
        return line
    return rstripped + ' ' + comment + '\n'


def annotate_file(filepath, file_annotations):
    """파일에 한글 주석을 추가"""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    new_lines = []
    in_license = False

    for i, line in enumerate(lines):
        lineno = i + 1
        stripped = line.strip()

        # 라이선스 블록 건너뛰기
        if stripped.startswith('/***'):
            in_license = True
        if in_license:
            new_lines.append(line)
            if stripped.endswith('***/') or stripped == '***************************************************************************/':
                in_license = False
            continue

        # 빈 줄 건너뛰기
        if not stripped:
            new_lines.append(line)
            continue

        # 이미 한글 주석 있으면 건너뛰기
        if has_korean_comment(line):
            new_lines.append(line)
            continue

        # 파일별 라인번호 주석 확인
        annotated = False
        if lineno in file_annotations:
            new_lines.append(add_comment(line, file_annotations[lineno]))
            annotated = True

        if not annotated:
            # 공통 패턴 매칭
            for pattern, comment in COMMON_PATTERNS:
                if comment and re.search(pattern, stripped):
                    new_lines.append(add_comment(line, '// ' + comment.lstrip('// ')))
                    annotated = True
                    break

        if not annotated:
            new_lines.append(line)

    with open(filepath, 'w') as f:
        f.writelines(new_lines)
    print(f"  Annotated: {os.path.basename(filepath)}")


# ============================================================
# 파일별 주석 정의 - (라인번호: 주석) 딕셔너리
# ============================================================

def get_Ucache_h_annotations():
    """Ucache.h 주석"""
    return {
        33: '// 헤더 가드 시작 - Ucache(통합 캐시) 헤더 중복 포함 방지',
        34: '// 헤더 가드 매크로 정의',
        36: '// STL 리스트 컨테이너 - 후보 배열 구성을 리스트로 관리',
        37: '// 면적(Area) 클래스 포함',
        38: '// NoC 라우터 모델 포함',
        39: '// NUCA(Non-Uniform Cache Architecture) 모델 포함',
        42: '// min_values_t: 탐색 공간에서 최소 지연/전력/면적/사이클 값을 추적하는 클래스',
        43: '// 공개 멤버 시작',
        45: '// 최소 접근 지연 시간 (초)',
        46: '// 최소 동적 에너지 소비 (줄)',
        47: '// 최소 누설 전력 (와트)',
        48: '// 최소 면적 (제곱미크론)',
        49: '// 최소 사이클 시간 (초)',
        51: '// 생성자: 모든 최솟값을 BIGNUM(매우 큰 수)으로 초기화하여 이후 비교 갱신 가능하게 함',
        53: '// 다른 min_values_t 객체의 값으로 최솟값 갱신',
        54: '// UCA 구조체 결과로 최솟값 갱신',
        55: '// NUCA 구조체 결과로 최솟값 갱신',
        56: '// mem_array 결과로 최솟값 갱신',
        61: '// solution 구조체: 태그 배열과 데이터 배열 조합의 최적 해를 저장',
        63: '// 태그 배열 인덱스 (후보 리스트 내 위치)',
        64: '// 데이터 배열 인덱스',
        65: '// 태그 배열 리스트 반복자',
        66: '// 데이터 배열 리스트 반복자',
        67: '// 총 접근 시간 = 태그 접근 + 데이터 접근 (또는 병렬 시 max)',
        68: '// 사이클 시간 = 연속 접근 간 최소 간격',
        69: '// 전체 캐시 면적 (태그 + 데이터)',
        70: '// 면적 효율 = 셀 면적 / 전체 면적 비율',
        71: '// 전체 전력 소비 (읽기/쓰기/검색 동적 + 누설)',
        76: '// calculate_time(): 주어진 배열 파티셔닝 파라미터로 지연/전력/면적 계산',
        77: '// is_tag: 태그 배열 여부 (false면 데이터 배열)',
        78: '// pure_ram: 순수 RAM 모드 (캐시가 아닌 스크래치패드 등)',
        79: '// pure_cam: 순수 CAM(Content-Addressable Memory) 모드',
        80: '// Nspd: 서브어레이당 병렬도 (몇 개의 워드라인을 한 서브어레이에 배치할지)',
        81: '// Ndwl: 워드라인 방향 서브어레이 분할 수 (수평)',
        82: '// Ndbl: 비트라인 방향 서브어레이 분할 수 (수직)',
        83: '// Ndcm: 컬럼 먹싱 차수 (비트라인 먹싱 정도)',
        84: '// Ndsam_lev_1: 센스앰프 먹스 레벨1 차수',
        85: '// Ndsam_lev_2: 센스앰프 먹스 레벨2 차수',
        86: '// ptr_array: 결과를 저장할 mem_array 구조체 포인터',
        87: '// flag_results_populate: 1이면 상세 결과 구조체에 채움',
        88: '// ptr_results: 상세 결과 저장용 구조체',
        89: '// ptr_fin_res: 최종 결과를 저장할 uca_org_t 포인터',
        90: '// is_main_mem: 메인 메모리 모드 여부 (DRAM 파라미터 적용)',
        91: '// update(): 누설 전력 피드백 후 캐시 에너지를 재계산',
        93: '// solve(): 모든 가능한 배열 구성을 탐색하여 최적 UCA 구조를 찾음',
        94: '// init_tech_params(): 주어진 기술 노드(nm)로 트랜지스터/배선 파라미터 초기화',
        97: '// calc_time_mt_wrapper_struct: 멀티스레드 탐색을 위한 래퍼 구조체',
        99: '// 스레드 ID (0 ~ nthreads-1)',
        100: '// 태그 배열 탐색 여부',
        101: '// 순수 RAM 모드 여부',
        102: '// 순수 CAM 모드 여부',
        103: '// 메인 메모리 모드 여부',
        104: '// Nspd 탐색 시작값 (최소 병렬도)',
        106: '// 데이터 배열 최솟값 추적 포인터',
        107: '// 태그 배열 최솟값 추적 포인터',
        109: '// 유효한 데이터 배열 구성 리스트',
        110: '// 유효한 태그 배열 구성 리스트',
        113: '// calc_time_mt_wrapper(): pthread로 실행되는 멀티스레드 탐색 함수',
    }


def get_mat_h_annotations():
    """mat.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Mat(매트) 클래스',
        35: '// 헤더 가드 매크로',
        42: '// Mat 클래스: 캐시의 기본 구성 단위. 서브어레이 + 행 디코더 + 센스앰프 + 먹스를 포함하는 물리적 블록',
        43: '// Component 기본 클래스를 상속 (delay, power, area 멤버 포함)',
        45: '// 생성자: DynamicParameter로부터 Mat의 모든 하위 컴포넌트를 초기화',
        46: '// 소멸자: 동적 할당된 디코더, 프리디코더, 드라이버 등을 해제',
        47: '// compute_delays(): Mat 내부의 전체 지연 경로를 계산하고 출력 rise time을 반환',
        48: '// compute_power_energy(): Mat의 동적 에너지, 누설 전력을 계산',
        50: '// dp: 동적 파라미터 참조 (배열 구성, 포트 수, 기술 파라미터 등)',
        53: '// row_dec: 행 디코더 - 행 주소를 받아 워드라인 하나를 활성화',
        54: '// bit_mux_dec: 비트라인 먹스 디코더 - 여러 비트라인 중 하나를 선택',
        55: '// sa_mux_lev_1_dec: 센스앰프 먹스 레벨1 디코더',
        56: '// sa_mux_lev_2_dec: 센스앰프 먹스 레벨2 디코더',
        57: '// dummy_way_sel_predec_blk1: 웨이 선택 프리디코더 블록1 (더미)',
        58: '// dummy_way_sel_predec_blk2: 웨이 선택 프리디코더 블록2 (더미)',
        59: '// way_sel_drv1: 웨이 선택 드라이버 - set-associative 캐시에서 way를 선택',
        60: '// dummy_way_sel_predec_blk_drv2: 웨이 선택 프리디코더 블록 드라이버2 (더미)',
        62: '// r_predec: 행 프리디코더 - 행 주소 비트를 부분 디코딩',
        63: '// b_mux_predec: 비트 먹스 프리디코더',
        64: '// sa_mux_lev_1_predec: 센스앰프 먹스 레벨1 프리디코더',
        65: '// sa_mux_lev_2_predec: 센스앰프 먹스 레벨2 프리디코더',
        67: '// subarray_out_wire: 서브어레이 출력 배선 모델 (센스앰프에서 출력까지)',
        68: '// bl_precharge_eq_drv: 비트라인 프리차지/이퀄라이즈 드라이버 (읽기 전 BL 전압을 Vdd/2로 설정)',
        69: '// cam_bl_precharge_eq_drv: CAM 전용 비트라인 프리차지 드라이버 (CAM과 RAM 배열 분리)',
        70: '// ml_precharge_drv: 매치라인 프리차지 드라이버 (CAM에서 비교 전 매치라인을 Vdd로 충전)',
        71: '// sl_precharge_eq_drv: 검색라인 프리차지 드라이버 (CAM 검색라인 초기화)',
        72: '// sl_data_drv: 검색라인 데이터 드라이버 (검색 키를 검색라인에 구동)',
        73: '// ml_to_ram_wl_drv: 매치라인→RAM 워드라인 드라이버 (FA에서 매치 후 해당 RAM 워드라인 활성화)',
        76: '// power_row_decoders: 행 디코더의 전력 소비',
        77: '// power_bit_mux_decoders: 비트 먹스 디코더의 전력 소비',
        78: '// power_sa_mux_lev_1_decoders: 센스앰프 먹스 레벨1 디코더의 전력',
        79: '// power_sa_mux_lev_2_decoders: 센스앰프 먹스 레벨2 디코더의 전력',
        80: '// power_fa_cam: 완전연관(FA) CAM 부분의 전력 (누설 전력 미구현)',
        81: '// power_bl_precharge_eq_drv: 비트라인 프리차지 드라이버의 전력',
        82: '// power_subarray_out_drv: 서브어레이 출력 드라이버의 전력',
        83: '// power_cam_all_active: CAM 모든 행 활성화 시 전력 (검색 동작)',
        84: '// power_searchline_precharge: 검색라인 프리차지 전력',
        85: '// power_matchline_precharge: 매치라인 프리차지 전력',
        86: '// power_ml_to_ram_wl_drv: 매치라인→워드라인 드라이버 전력',
        88: '// delay_fa_tag: FA 태그 지연, delay_cam: CAM 검색 지연',
        89: '// delay_before_decoder: 디코더 이전 단계 지연 (프리디코더 등)',
        90: '// delay_bitline: 비트라인 방전/감지 지연 (셀에서 센스앰프까지)',
        91: '// delay_wl_reset: 워드라인 리셋 지연 (사이클 타임에 영향)',
        92: '// delay_bl_restore: 비트라인 복원 지연 (프리차지로 원래 전압 회복)',
        94: '// delay_searchline: 검색라인 지연 (CAM)',
        95: '// delay_matchchline: 매치라인 지연 (CAM에서 비교 결과 전파)',
        96: '// delay_cam_sl_restore: CAM 검색라인 복원 지연',
        97: '// delay_cam_ml_reset: CAM 매치라인 리셋 지연',
        98: '// delay_fa_ram_wl: FA에서 RAM 워드라인 활성화 지연',
        100: '// delay_hit_miss_reset: 히트/미스 로직 리셋 지연',
        101: '// delay_hit_miss: 히트/미스 판정 지연',
        103: '// subarray: 이 Mat에 속한 서브어레이 객체',
        104: '// power_bitline: 비트라인 동적 전력, power_searchline: 검색라인 전력, power_matchline: 매치라인 전력',
        105: '// per_bitline_read_energy: 비트라인 1개당 읽기 에너지 (DRAM refresh 계산용)',
        106: '// deg_bl_muxing: 비트라인 먹싱 차수 (몇 개의 BL을 하나의 SA에 연결)',
        107: '// num_act_mats_hor_dir: 수평 방향 활성화 매트 수',
        108: '// delay_writeback: 쓰기 지연 (DRAM 복원 포함)',
        109: '// cell: SRAM/DRAM 셀 면적, cam_cell: CAM 셀 면적',
        110: '// is_dram: DRAM 여부, is_fa: 완전연관 캐시 여부, pure_cam: 순수 CAM 여부, camFlag: CAM 관련 플래그',
        111: '// num_mats: 전체 뱅크 내 Mat 개수',
        112: '// power_sa: 센스앰프 전력 소비',
        113: '// delay_sa: 센스앰프 지연 (BL 전압차 감지 및 증폭)',
        114: '// leak_power_sense_amps_closed_page_state: SA 닫힌 페이지 상태 누설 전력',
        115: '// leak_power_sense_amps_open_page_state: SA 열린 페이지 상태 누설 전력',
        116: '// delay_subarray_out_drv: 서브어레이 출력 드라이버 지연',
        117: '// delay_subarray_out_drv_htree: 출력 드라이버 + H-tree 배선 지연',
        118: '// delay_comparator: 태그 비교기 지연 (set-associative 캐시의 히트/미스 판정)',
        119: '// power_comparator: 태그 비교기 전력',
        120: '// num_do_b_mat: Mat당 데이터 출력 비트 수',
        121: '// num_so_b_mat: Mat당 검색 출력 비트 수',
        122: '// num_sa_subarray: 서브어레이당 센스앰프 수',
        123: '// num_sa_subarray_search: 검색용 센스앰프 수',
        124: '// C_bl: 비트라인 총 커패시턴스 (fF) - 셀 수 × 단위 셀 커패시턴스',
        126: '// num_subarrays_per_mat: Mat 내 서브어레이 개수 (최대 4개)',
        127: '// num_subarrays_per_row: Mat 한 행의 서브어레이 수 (최대 2개)',
        131: '// compute_bit_mux_sa_precharge_sa_mux_wr_drv_wr_mux_h(): BL 먹스+SA+프리차지+SA먹스+쓰기드라이버의 총 높이 계산',
        132: '// width_write_driver_or_write_mux(): 쓰기 드라이버/쓰기 먹스의 폭 계산',
        133: '// compute_comparators_height(): 태그 비교기 높이 계산 (태그 비트 수, way 수에 따라)',
        134: '// compute_cam_delay(): CAM 검색 경로 지연 계산 (검색라인→매치라인→NOR→드라이버)',
        135: '// compute_bitline_delay(): 비트라인 방전 지연 계산 (워드라인→BL→SA 입력)',
        136: '// compute_sa_delay(): 센스앰프 감지/증폭 지연 계산',
        137: '// compute_subarray_out_drv(): 서브어레이 출력 드라이버 지연 계산',
        138: '// compute_comparator_delay(): 태그 비교기 지연 계산 (XOR 기반)',
        140: '// RWP: 읽기/쓰기 포트 수 (Read-Write Port)',
        141: '// ERP: 전용 읽기 포트 수 (Exclusive Read Port)',
        142: '// EWP: 전용 쓰기 포트 수 (Exclusive Write Port)',
        143: '// SCHP: 검색 포트 수 (Search Port, CAM용)',
    }


def get_bank_h_annotations():
    """bank.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Bank 클래스',
        35: '// 헤더 가드 매크로',
        43: '// Bank 클래스: 여러 개의 Mat을 H-tree 배선으로 연결한 뱅크 단위',
        44: '// Component를 상속하여 delay, power, area 멤버를 가짐',
        46: '// 생성자: DynamicParameter로 Bank를 초기화하고 내부 Mat 및 H-tree를 구성',
        47: '// 소멸자: H-tree 객체 메모리 해제',
        48: '// compute_delays(): Bank 내부 지연 계산 (실제로는 Mat에 위임)',
        49: '// compute_power_energy(): Bank 전체 전력/에너지 계산 (Mat + H-tree)',
        51: '// dp: 동적 파라미터 참조',
        52: '// mat: 이 Bank에 속한 Mat 객체 (모든 Mat이 동일 구조이므로 대표 1개)',
        53: '// htree_in_add: 주소 입력 H-tree (뱅크 입구→Mat까지 주소 분배)',
        54: '// htree_in_data: 데이터 입력 H-tree (쓰기 데이터 분배)',
        55: '// htree_out_data: 데이터 출력 H-tree (Mat→뱅크 출구까지 데이터 수집)',
        56: '// htree_in_search: 검색 입력 H-tree (CAM 검색 키 분배)',
        57: '// htree_out_search: 검색 출력 H-tree (CAM 검색 결과 수집)',
        59: '// num_addr_b_mat: Mat에 전달되는 주소 비트 수',
        60: '// num_mats_hor_dir: 수평 방향 Mat 개수',
        61: '// num_mats_ver_dir: 수직 방향 Mat 개수',
        63: '// num_addr_b_row_dec: 행 디코더에 사용되는 주소 비트 수',
        64: '// num_addr_b_routed_to_mat_for_act: 행 활성화(activate)에 라우팅되는 주소 비트 수',
        65: '// num_addr_b_routed_to_mat_for_rd_or_wr: 읽기/쓰기에 라우팅되는 주소 비트 수',
    }


def get_decoder_h_annotations():
    """decoder.h 주석"""
    return {
        33: '// 헤더 가드 시작 - Decoder 클래스',
        34: '// 헤더 가드 매크로',
        44: '// Decoder 클래스: NAND 게이트 + 인버터 체인으로 구성된 행/열 디코더',
        45: '// Component 상속 (delay, power, area)',
        47: '// 생성자: 디코딩 신호 수, 출력 부하 등으로 디코더를 초기화',
        48: '// _num_dec_signals: 디코딩 출력 신호 수 (예: 256행이면 256)',
        49: '// flag_way_select: 웨이 선택 신호가 추가로 필요한지 (3입력 NAND 사용)',
        50: '// _C_ld_dec_out: 디코더 출력의 부하 커패시턴스 (워드라인 등)',
        51: '// _R_wire_dec_out: 디코더 출력 배선 저항',
        52: '// fully_assoc_: 완전연관 캐시 여부',
        53: '// is_dram_: DRAM 모드 여부 (트랜지스터 파라미터가 달라짐)',
        54: '// is_wl_tr_: 워드라인 트랜지스터 여부 (DRAM WL 부스팅 전압 Vpp 사용)',
        55: '// cell_: 메모리 셀 면적 참조',
        57: '// exist: 이 디코더가 실제로 필요한지 (주소 비트가 적으면 불필요)',
        58: '// num_in_signals: 디코더 첫 게이트의 입력 수 (2=NAND2, 3=NAND3)',
        59: '// C_ld_dec_out: 디코더 최종 출력의 부하 커패시턴스 (fF)',
        60: '// R_wire_dec_out: 출력 배선 저항 (옴)',
        61: '// num_gates: 디코더 내 게이트 단계 수 (logical effort로 결정)',
        62: '// num_gates_min: 최소 게이트 단계 수 (기본 2)',
        63: '// w_dec_n[]: 각 단계 NMOS 트랜지스터 폭 배열 (um)',
        64: '// w_dec_p[]: 각 단계 PMOS 트랜지스터 폭 배열 (um)',
        65: '// delay: 디코더 총 지연 (초)',
        67: '// fully_assoc: 완전연관 캐시 여부',
        68: '// is_dram: DRAM 모드 플래그',
        69: '// is_wl_tr: 워드라인 트랜지스터 여부',
        70: '// cell: 메모리 셀 면적 참조 (디코더 높이 = h_dec × cell.h)',
        73: '// compute_widths(): logical effort 방법으로 각 단계의 최적 트랜지스터 폭 계산',
        74: '// compute_area(): 디코더의 총 면적과 누설 전류 계산',
        75: '// compute_delays(): Horowitz 모델로 디코더 지연 계산, 출력 rise time 반환',
        77: '// leakage_feedback(): 온도 변화에 따른 누설 전력 재계산',
        82: '// PredecBlk 클래스: 프리디코더 블록 - 주소 비트를 부분적으로 디코딩하는 1단계/2단계 NAND 트리',
        83: '// Component 상속',
        85: '// 생성자: 디코딩 신호 수, 부하, 분기 팬아웃 등으로 프리디코더 블록을 구성',
        94: '// dec: 이 프리디코더 블록이 구동하는 디코더의 포인터',
        95: '// exist: 이 프리디코더 블록이 필요한지',
        96: '// number_input_addr_bits: 입력 주소 비트 수',
        97: '// C_ld_predec_blk_out: 출력 부하 커패시턴스',
        98: '// R_wire_predec_blk_out: 출력 배선 저항',
        99: '// branch_effort_nand2_gate_output: NAND2 경로의 분기 팬아웃',
        100: '// branch_effort_nand3_gate_output: NAND3 경로의 분기 팬아웃',
        101: '// flag_two_unique_paths: NAND2와 NAND3 경로가 모두 존재하는지',
        102: '// flag_L2_gate: 2단계 게이트 유형 (0=없음, 2=NAND2, 3=NAND3)',
        103: '// number_inputs_L1_gate: 1단계 게이트 입력 수',
        104: '// number_gates_L1_nand2_path: NAND2 경로의 게이트 단계 수',
        105: '// number_gates_L1_nand3_path: NAND3 경로의 게이트 단계 수',
        106: '// number_gates_L2: 2단계의 게이트 단계 수',
        117: '// delay_nand2_path: NAND2 경로 지연',
        118: '// delay_nand3_path: NAND3 경로 지연',
        119: '// power_nand2_path: NAND2 경로 전력',
        120: '// power_nand3_path: NAND3 경로 전력',
        121: '// power_L2: 2단계 전력',
        130: '// compute_delays(): NAND2/NAND3 양쪽 경로의 지연을 pair로 반환',
        135: '// PredecBlkDrv 클래스: 프리디코더 블록 드라이버 - 프리디코더 블록의 출력을 구동',
        143: '// flag_driver_exists: 드라이버가 필요한지',
        144: '// number_input_addr_bits: 입력 주소 비트 수',
        167: '// blk: 이 드라이버가 구동하는 프리디코더 블록 포인터',
        168: '// dec: 대상 디코더 포인터',
        169: '// is_dram_: DRAM 모드 플래그',
        170: '// way_select: 웨이 선택 신호 수',
        178: '// compute_delays(): NAND2/NAND3 양쪽 경로의 지연 계산, pair로 반환',
        182: '// num_addr_bits_nand2_path(): NAND2 경로의 주소 비트 수 반환',
        188: '// num_addr_bits_nand3_path(): NAND3 경로의 주소 비트 수 반환',
        193: '// get_rdOp_dynamic_E(): 읽기 동작의 동적 에너지 계산',
        198: '// Predec 클래스: 2개의 PredecBlkDrv를 조합한 완전한 프리디코더',
        201: '// 생성자: 2개의 드라이버로 프리디코더 구성',
        205: '// compute_delays(): 전체 프리디코더 지연 계산, 출력 rise time 반환',
        207: '// leakage_feedback(): 온도 변화에 따른 누설 전력 재계산',
        208: '// blk1, blk2: 2개의 프리디코더 블록',
        210: '// drv1, drv2: 2개의 프리디코더 블록 드라이버',
        213: '// block_power: 블록 전력',
        214: '// driver_power: 드라이버 전력',
        218: '// get_max_delay_before_decoder(): 두 입력 경로 중 더 긴 지연을 가진 경로 선택',
        225: '// Driver 클래스: 범용 인버터 체인 드라이버 (프리차지, 출력 드라이버 등에 사용)',
        228: '// 생성자: 게이트 부하, 배선 부하, 배선 저항으로 드라이버를 sizing',
        230: '// number_gates: 인버터 체인의 단계 수',
        231: '// min_number_gates: 최소 단계 수',
        232: '// width_n[]: 각 단계 NMOS 폭 (um)',
        233: '// width_p[]: 각 단계 PMOS 폭 (um)',
        234: '// c_gate_load: 최종 게이트 부하 커패시턴스',
        235: '// c_wire_load: 배선 부하 커패시턴스',
        236: '// r_wire_load: 배선 저항',
        237: '// delay: 드라이버 총 지연',
        238: '// power: 드라이버 전력 소비',
        239: '// is_dram_: DRAM 모드 플래그',
        241: '// compute_widths(): logical effort로 각 단계의 최적 트랜지스터 폭 결정',
        242: '// compute_delay(): Horowitz 모델로 드라이버 지연 계산',
    }


def get_htree2_h_annotations():
    """htree2.h 주석"""
    return {
        33: '// 헤더 가드 시작 - H-tree 배선 모델',
        34: '// 헤더 가드 매크로',
        44: '// 누설 전력 범위 주석: uca_tree==false면 뱅크 내 전체, true면 한 뱅크까지만',
        47: '// Htree2 클래스: H-tree 배선 모델 - 이진 트리 형태로 주소/데이터를 분배하는 배선 구조',
        48: '// Component를 상속 (delay, power, area)',
        50: '// 생성자: wire 모델, Mat 크기, 주소/데이터 비트폭, H-tree 유형 등으로 초기화',
        54: '// 소멸자 (빈 구현)',
        56: '// in_htree(): 입력 H-tree 지연/전력 계산 (루트→리프 방향, NAND 게이트로 신호 제한)',
        57: '// out_htree(): 출력 H-tree 지연/전력 계산 (리프→루트 방향, tri-state 버퍼로 팬인 처리)',
        60: '// limited_in_htree(): H-tree 노드에서만 리피터를 배치하는 제한된 입력 모델',
        61: '// limited_out_htree(): 제한된 출력 모델',
        62: '// input_nand(): H-tree 노드의 NAND 게이트 지연/전력 계산',
        63: '// output_buffer(): H-tree 노드의 출력 tri-state 버퍼 지연/전력 계산',
        65: '// in_rise_time: 입력 신호 상승 시간, out_rise_time: 출력 신호 상승 시간',
        72: '// max_unpipelined_link_delay: 파이프라이닝 없는 최대 링크 지연',
        73: '// power_bit: 비트당 전력 (전체 전력 = power_bit × 버스 폭)',
        77: '// wire_bw: 현재 H-tree 레벨의 배선 대역폭 (비트 수)',
        78: '// init_wire_bw: 루트에서의 초기 배선 대역폭',
        79: '// tree_type: H-tree 유형 (주소/데이터입력/데이터출력/검색입력/검색출력)',
        80: '// htree_hnodes: 수평 노드 수',
        81: '// htree_vnodes: 수직 노드 수',
        82: '// mat_width: Mat의 폭 (um)',
        83: '// mat_height: Mat의 높이 (um)',
        84: '// add_bits: 주소 비트 수, data_in_bits/out_bits: 데이터 입출력 비트 수, search 비트 수',
        85: '// ndbl: 수직 서브어레이 분할 수, ndwl: 수평 서브어레이 분할 수',
        86: '// uca_tree: UCA 트리 여부 (뱅크 간 트리면 true, 뱅크 내 트리면 false)',
        87: '// search_tree: 검색 전용 트리 여부',
        89: '// wt: 배선 유형 (Global, Semi_global, Low_swing 등)',
        90: '// min_w_nmos: 최소 NMOS 폭 (기술 파라미터에서 결정)',
        91: '// min_w_pmos: 최소 PMOS 폭 (n_to_p 비율로 결정)',
        93: '// deviceType: 트랜지스터 파라미터 (Vdd, Vth, Ion, Ioff 등)',
    }


def get_wire_h_annotations():
    """wire.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Wire(배선) 클래스',
        35: '// 헤더 가드 매크로',
        45: '// Wire 클래스: 금속 배선의 지연, 전력, 면적을 모델링 - 리피터 삽입 최적화 포함',
        46: '// Component를 상속 (delay, power, area)',
        48: '// 주 생성자: wire 유형, 길이, SA 수, 폭/간격 스케일링 등으로 배선 특성 계산',
        55: '// 소멸자',
        57: '// 초기화 전용 생성자: 정적 멤버(global, global_5 등) 초기화에 한 번만 사용',
        63: '// init_wire(): 정적 멤버 초기화 - 다양한 리피터 크기/간격으로 최적/준최적 배선 파라미터 계산',
        65: '// calculate_wire_stats(): 주어진 wire 유형에 맞는 지연/전력 계산',
        66: '// delay_optimal_wire(): 리피터 삽입으로 지연을 최소화하는 최적 배선 모델',
        67: '// wire_cap(): 배선 커패시턴스 계산 (측벽 + 상하 금속층 커플링)',
        68: '// wire_res(): 배선 저항 계산 (저항률 × 길이 / 단면적)',
        69: '// low_swing_model(): 저전압 스윙 차동 배선 모델 (전력 절감, 긴 배선용)',
        70: '// signal_fall_time(): 2단 인버터의 출력 하강 시간 계산',
        71: '// signal_rise_time(): 2단 인버터의 출력 상승 시간 계산',
        72: '// sense_amp_input_cap(): 저전압 스윙 배선의 수신 SA 입력 커패시턴스',
        74: '// wt: 배선 유형 (Global, Global_5, Low_swing 등)',
        75: '// wire_spacing: 배선 간격 (um)',
        76: '// wire_width: 배선 폭 (um)',
        77: '// wire_placement: 배선 위치 (Mat 내부/외부/로컬)',
        78: '// repeater_size: 최적 리피터 크기 (최소 인버터 대비 배율)',
        79: '// repeater_spacing: 최적 리피터 간격 (um)',
        80: '// wire_length: 배선 길이 (um)',
        81: '// in_rise_time: 입력 상승 시간, out_rise_time: 출력 상승 시간',
        87: '// global: 지연 최적 전역 배선 파라미터 (정적 멤버, 단위 길이당 값)',
        88: '// global_5: 5% 지연 오버헤드 허용 배선 (전력 절감)',
        89: '// global_10: 10% 지연 오버헤드 허용 배선',
        90: '// global_20: 20% 지연 오버헤드 허용 배선',
        91: '// global_30: 30% 지연 오버헤드 허용 배선',
        92: '// low_swing: 저전압 스윙 배선 파라미터',
        93: '// wire_width_init: 초기 배선 폭 (정적)',
        94: '// wire_spacing_init: 초기 배선 간격 (정적)',
        95: '// print_wire(): 배선 특성을 콘솔에 출력',
        99: '// nsense: 저전압 스윙 배선에 연결된 SA 수 (브로드캐스트 시 여러 수신기)',
        104: '// w_scale, s_scale: 배선 폭/간격 스케일링 팩터',
        105: '// resistivity: 금속 저항률 (기본값: 구리 CU_RESISTIVITY)',
        106: '// wire_model(): 주어진 리피터 간격/크기로 지연/전력 계산하는 내부 함수',
        107: '// repeated_wire: 다양한 리피터 설정의 배선 후보 리스트',
        108: '// update_fullswing(): 5%/10%/20%/30% 오버헤드 배선의 최적 리피터 설정을 선택',
        109: '// initialized: 정적 초기화 완료 플래그',
        113: '// transmitter: 저전압 스윙 송신기 (NAND + 인버터 + NMOS 드라이버)',
        114: '// l_wire: 저전압 스윙 배선 부분',
        115: '// sense_amp: 저전압 스윙 수신 센스앰프',
        117: '// min_w_pmos: 최소 PMOS 폭',
        119: '// deviceType: 트랜지스터 디바이스 파라미터 포인터',
    }


def get_subarray_h_annotations():
    """subarray.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Subarray(서브어레이) 클래스',
        35: '// 헤더 가드 매크로',
        44: '// Subarray 클래스: SRAM/DRAM/CAM 메모리 셀들의 2D 배열 (행=워드라인, 열=비트라인)',
        45: '// Component를 상속 (delay, power, area)',
        47: '// 생성자: DynamicParameter와 FA 여부로 서브어레이를 초기화하고 C_bl/C_wl 계산',
        48: '// 소멸자',
        50: '// dp: 동적 파라미터 참조',
        51: '// get_total_cell_area(): 서브어레이 내 모든 메모리 셀의 순수 면적 합계',
        52: '// num_rows: 서브어레이 행 수 (= 워드라인 수)',
        53: '// num_cols: 서브어레이 열 수 (= 비트라인 수, ECC 포함)',
        54: '// num_cols_fa_cam: FA/CAM 배열의 CAM 부분 열 수',
        55: '// num_cols_fa_ram: FA 배열의 RAM 부분 열 수',
        56: '// cell: SRAM/DRAM 셀 면적, cam_cell: CAM 셀 면적',
        58: '// is_fa: 완전연관 캐시 여부',
        59: '// C_wl: 워드라인 총 커패시턴스, C_wl_cam/ram: CAM/RAM 각각의 WL 커패시턴스',
        60: '// R_wl: 워드라인 총 저항, R_wl_cam/ram: CAM/RAM 각각의 WL 저항',
        61: '// C_bl: 비트라인 총 커패시턴스, C_bl_cam: CAM 비트라인 커패시턴스',
        64: '// compute_C(): 비트라인/워드라인의 커패시턴스 및 저항 계산 (gate_C_pass, drain_C_ 사용)',
    }


def get_uca_h_annotations():
    """uca.h 주석"""
    return {
        34: '// 헤더 가드 시작 - UCA(Uniform Cache Architecture)',
        35: '// 헤더 가드 매크로',
        44: '// UCA 클래스: 균일 캐시 아키텍처 - 여러 Bank를 뱅크 간 H-tree로 연결한 전체 캐시 구조',
        45: '// Component를 상속',
        47: '// 생성자: DynamicParameter로 UCA 전체를 구성 (Bank + 뱅크간 H-tree + 지연/전력 계산)',
        48: '// 소멸자',
        49: '// compute_delays(): UCA 전체 접근 시간 및 사이클 시간 계산',
        50: '// compute_power_energy(): UCA 전체 전력/에너지 계산',
        52: '// dp: 동적 파라미터 복사본',
        53: '// bank: 대표 Bank 객체',
        55: '// htree_in_add: 뱅크 간 주소 입력 H-tree',
        56: '// htree_in_data: 뱅크 간 데이터 입력 H-tree',
        57: '// htree_out_data: 뱅크 간 데이터 출력 H-tree',
        58: '// htree_in_search: 뱅크 간 검색 입력 H-tree (CAM용)',
        59: '// htree_out_search: 뱅크 간 검색 출력 H-tree (CAM용)',
        61: '// power_routing_to_bank: 뱅크로의 라우팅 전력',
        63: '// nbanks: 전체 뱅크 수',
        65: '// num_addr_b_bank: 뱅크당 주소 비트 수',
        66: '// num_di_b_bank: 뱅크당 데이터 입력 비트 수',
        67: '// num_do_b_bank: 뱅크당 데이터 출력 비트 수',
        68: '// num_si_b_bank: 뱅크당 검색 입력 비트 수',
        69: '// num_so_b_bank: 뱅크당 검색 출력 비트 수',
        70: '// RWP, ERP, EWP, SCHP: 읽기쓰기/전용읽기/전용쓰기/검색 포트 수',
        71: '// area_all_dataramcells: 전체 데이터 RAM 셀의 순수 면적',
        73: '// dyn_read_energy_from_closed_page: 닫힌 페이지에서 읽기 동적 에너지 (DRAM)',
        74: '// dyn_read_energy_from_open_page: 열린 페이지에서 읽기 동적 에너지',
        75: '// dyn_read_energy_remaining_words_in_burst: 버스트 내 나머지 워드 읽기 에너지',
        77: '// refresh_power: DRAM 리프레시 전력',
        78: '// activate_energy: DRAM 행 활성화 에너지',
        79: '// read_energy: 읽기 에너지',
        80: '// write_energy: 쓰기 에너지',
        81: '// precharge_energy: 프리차지 에너지',
        82: '// leak_power_subbank_closed_page: 닫힌 페이지 상태 서브뱅크 누설 전력',
        83: '// leak_power_subbank_open_page: 열린 페이지 상태 서브뱅크 누설 전력',
        84: '// leak_power_request_and_reply_networks: 요청/응답 네트워크 누설 전력',
        86: '// delay_array_to_sa_mux_lev_1_decoder: 배열→SA먹스 레벨1 디코더 지연',
        87: '// delay_array_to_sa_mux_lev_2_decoder: 배열→SA먹스 레벨2 디코더 지연',
        88: '// delay_before_subarray_output_driver: 서브어레이 출력 드라이버 이전까지의 총 지연',
        89: '// delay_from_subarray_out_drv_to_out: 출력 드라이버부터 최종 출력까지 지연',
        90: '// access_time: 총 접근 시간 (캐시 히트 시)',
        91: '// precharge_delay: 프리차지 지연',
        92: '// multisubbank_interleave_cycle_time: 멀티 서브뱅크 인터리빙 사이클 시간',
    }


def get_nuca_h_annotations():
    """nuca.h 주석"""
    return {
        33: '// 헤더 가드 시작 - NUCA(Non-Uniform Cache Architecture)',
        34: '// 헤더 가드 매크로',
        49: '// nuca_org_t: NUCA 구조의 결과를 저장하는 클래스',
        51: '// 소멸자: 동적 할당된 Wire, Router 해제',
        54: '// nuca_pda: NUCA 전체의 면적/전력/지연',
        55: '// bank_pda: 뱅크의 면적/전력/지연',
        56: '// wire_pda: 배선의 면적/전력/지연',
        57: '// h_wire: 수평 배선 모델',
        58: '// v_wire: 수직 배선 모델',
        59: '// router: 라우터 모델 (패킷 스위칭 네트워크의 라우터)',
        64: '// contention: 네트워크 경합(contention) 계수',
        67: '// avg_hops: 평균 홉 수 (소스→목적지 뱅크까지 거치는 라우터 수)',
        68: '// rows: 그리드 네트워크 행 수',
        69: '// columns: 그리드 네트워크 열 수',
        70: '// bank_count: 총 뱅크 수',
        75: '// Nuca 클래스: NUCA 캐시 모델 - 뱅크 간 비균일 접근 시간을 시뮬레이션',
        76: '// Component를 상속',
        79: '// 생성자: 디바이스 타입으로 NUCA 초기화',
        81: '// print_router(): 라우터 통계 출력',
        82: '// 소멸자',
        83: '// sim_nuca(): NUCA 시뮬레이션 실행',
        84: '// init_cont(): contention 테이블 초기화',
        85: '// calc_cycles(): 지연(초)을 클럭 사이클로 변환',
        86: '// calculate_nuca_area(): NUCA 전체 면적 계산',
        87: '// check_nuca_org(): NUCA 구성이 제약 조건을 만족하는지 확인',
        88: '// find_optimal_nuca(): 최적 NUCA 구성 선택',
        94: '// deviceType: 트랜지스터 파라미터',
        95: '// wt_min, wt_max: 탐색할 배선 유형 범위',
        96: '// wire_vertical[]: 수직 배선 모델 배열 (각 wire type별)',
        97: '// wire_horizontal[]: 수평 배선 모델 배열',
    }


def get_router_h_annotations():
    """router.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Router(라우터) 클래스',
        35: '// 헤더 가드 매크로',
        50: '// MCPAT_Router 클래스: NoC(Network-on-Chip) 라우터 모델 - 버퍼+크로스바+아비터',
        51: '// Component를 상속',
        53: '// 생성자: 플릿 크기, VC 버퍼, VC 수, 입출력 포트 수, 네트워크 부하로 라우터 구성',
        61: '// 소멸자',
        64: '// print_router(): 라우터 면적/전력/지연 통계 출력',
        66: '// arbiter, crossbar, buffer: 라우터의 3대 구성요소',
        68: '// cycle_time: 라우터 동작 사이클 시간 (ps), max_cyc: 최대 사이클 시간',
        69: '// flit_size: 플릿 크기 (비트) - NoC 전송 단위',
        70: '// vc_count: 가상 채널 수',
        71: '// vc_buffer_size: VC당 버퍼 엔트리 수',
        74: '// deviceType: 트랜지스터 파라미터',
        75: '// FREQUENCY: 동작 주파수 (GHz)',
        76: '// Cw3(): 3배 간격 배선의 커패시턴스 계산',
        77: '// gate_cap(): 게이트 커패시턴스 계산 (um→내부 단위 변환)',
        78: '// diff_cap(): 확산 커패시턴스 계산 (드레인/소스)',
        82: '// NTtr, PTtr: 전송 게이트 NMOS/PMOS 트랜지스터 길이',
        83: '// M: 네트워크 부하 계수 (0~1, 활성 포트 비율)',
    }


def get_crossbar_h_annotations():
    """crossbar.h 주석"""
    return {
        33: '// 헤더 가드 시작 - Crossbar(크로스바) 클래스',
        34: '// 헤더 가드 매크로',
        45: '// Crossbar 클래스: NxM 크로스바 스위치 모델 - tri-state 버퍼 기반의 전 연결 스위치',
        46: '// Component를 상속',
        48: '// 생성자: 입력/출력 포트 수, 플릿 크기, 디바이스 타입으로 크로스바 구성',
        53: '// 소멸자',
        55: '// print_crossbar(): 크로스바 통계 출력',
        56: '// output_buffer(): 출력 버퍼(tri-state) 사이징 및 커패시턴스 계산',
        57: '// compute_power(): 크로스바 전체 지연/전력 계산 (재귀적 종횡비 조정)',
        59: '// n_inp, n_out: 입력/출력 포트 수',
        60: '// flit_size: 플릿 크기 (비트)',
        61: '// tri_inp_cap/out_cap/ctr_cap/int_cap: tri-state 버퍼의 입력/출력/제어/내부 커패시턴스',
        64: '// CB_ADJ: 크로스바 종횡비 조정 인자 (종횡비가 ~1이 되도록 반복 조정)',
        74: '// deviceType: 트랜지스터 파라미터',
        75: '// TriS1, TriS2: tri-state 버퍼의 1단/2단 사이징',
        76: '// min_w_pmos: 최소 PMOS 폭, Vdd: 전원 전압',
    }


def get_arbiter_h_annotations():
    """arbiter.h 주석"""
    return {
        32: '// 헤더 가드 시작 - Arbiter(아비터/중재기) 클래스',
        33: '// 헤더 가드 매크로',
        44: '// MCPAT_Arbiter 클래스: 라운드로빈 아비터 모델 - NOR 게이트 기반 요청/허가 로직',
        45: '// Component를 상속',
        47: '// 생성자: 요청 포트 수, 플릿 크기, 출력 길이로 아비터 구성',
        54: '// print_arbiter(): 아비터 통계 출력',
        55: '// arb_req(): 요청(request) 경로의 커패시턴스 계산',
        56: '// arb_pri(): 우선순위 테이블의 스위칭 커패시턴스',
        57: '// arb_grant(): 허가(grant) 경로의 커패시턴스 (크로스바 제어선 포함)',
        58: '// arb_int(): 내부 NOR 게이트의 커패시턴스',
        59: '// compute_power(): 아비터 전체 동적/누설 전력 계산',
        60: '// Cw3(): 3배 간격 배선의 커패시턴스',
        61: '// crossbar_ctrline(): 크로스바 제어선의 커패시턴스',
        62: '// transmission_buf_ctrcap(): 전송 게이트 제어 커패시턴스',
        67: '// NTn1, PTn1, NTn2, PTn2: NOR 게이트 1,2단의 NMOS/PMOS 크기',
        68: '// flit_size: 플릿 크기',
        69: '// NTtr, PTtr: 전송 게이트 NMOS/PMOS 크기',
        70: '// o_len: 출력 길이 (크로스바 제어선 길이)',
        71: '// deviceType: 트랜지스터 파라미터',
        72: '// TriS1, TriS2: tri-state 버퍼 사이징',
        73: '// min_w_pmos: 최소 PMOS 폭, Vdd: 전원 전압',
    }


def get_highradix_h_annotations():
    """highradix.h 주석"""
    return {
        42: '// 헤더 가드 시작 - HighRadix(고차수 라우터) 클래스',
        43: '// 헤더 가드 매크로',
        57: '// FLIP_FLOP_L: 플립플롭 누설 전력 (W)',
        58: '// FLIP_FLOP_D: 플립플롭 동적 에너지 (J)',
        59: '// ROUTE_LOGIC_D: 라우팅 로직 동적 에너지 (J)',
        60: '// ROUTE_LOGIC_L: 라우팅 로직 누설 전력 (W)',
        62: '// HighRadix 클래스: 고차수(예: 64포트) 라우터 모델 - 서브스위치 기반 계층적 구조',
        63: '// Component를 상속',
        65: '// 생성자: 서브스위치 크기, 행 수, 주파수, radix, VC, 플릿 크기 등으로 초기화',
        83: '// SUB_SWITCH_SZ: 서브스위치 크기 (포트 수)',
        84: '// ROWS: 서브스위치 행 수',
        85: '// FREQUENCY: 동작 주파수 (GHz)',
        86: '// RADIX: 라우터 radix (전체 입출력 포트 수)',
        87: '// VC_COUNT: 가상 채널 수',
        88: '// FLIT_SZ: 플릿 크기 (비트)',
        89: '// AF: 활성화 인자 (activity factor)',
        90: '// DIE_LEN, DIE_HT: 다이 가로/세로 길이 (um)',
        113: '// cb, out_cb: 입력/출력 크로스바',
        114: '// cb_arb, vc_arb, c_arb: 크로스바/VC/컬럼 아비터',
        115: '// inp_buff, r_buff, c_buff: 입력/행/열 버퍼 (Mat으로 모델링)',
        116: '// sub_sw: 서브스위치 컴포넌트',
        117: '// wire_tot, buff_tot, crossbar_tot, arb_tot: 배선/버퍼/크로스바/아비터 합계',
        118: '// hor_bus, ver_bus: 수평/수직 버스 배선',
        127: '// Waveguide 클래스: 광 도파관 모델 (미래 기술용)',
    }


def get_io_h_annotations():
    """io.h 주석"""
    return {
        33: '// 헤더 가드 시작 - I/O 출력 함수',
        34: '// 헤더 가드 매크로',
        41: '// output_data_csv(): 캐시 결과를 CSV 형식으로 출력 (자동화 분석용)',
        42: '// output_UCA(): UCA 캐시 결과를 사람이 읽을 수 있는 텍스트로 출력',
    }


def get_parameter_h_annotations():
    """parameter.h 주석"""
    return {
        34: '// 헤더 가드 시작 - Parameter(파라미터) 클래스',
        35: '// 헤더 가드 매크로',
        43: '// TechnologyParameter 클래스: 공정 기술(예: 45nm, 32nm)에 따른 트랜지스터/배선 파라미터 집합',
        46: '// DeviceType 내부 클래스: 하나의 디바이스 유형(SRAM셀, DRAM, 주변회로 등)의 전기적 특성',
        49: '// C_g_ideal: 이상적인 게이트 커패시턴스 (fF/um)',
        50: '// C_fringe: 프린지(가장자리) 커패시턴스 (fF/um)',
        51: '// C_overlap: 게이트-드레인 오버랩 커패시턴스 (fF/um)',
        52: '// C_junc: 접합(junction) 면적 커패시턴스 (fF/um²)',
        53: '// C_junc_sidewall: 접합 측벽 커패시턴스 (fF/um)',
        54: '// l_phy: 물리적 게이트 길이 (um)',
        55: '// l_elec: 전기적 유효 게이트 길이 (um)',
        56: '// R_nch_on: NMOS 온-저항 (ohm·um) - W=1um일 때의 채널 저항',
        57: '// R_pch_on: PMOS 온-저항 (ohm·um)',
        58: '// Vdd: 전원 전압 (V)',
        59: '// Vth: 문턱 전압 (V)',
        60: '// I_on_n: NMOS 온-전류 (A/um)',
        61: '// I_on_p: PMOS 온-전류 (A/um)',
        62: '// I_off_n: NMOS 서브스레숄드 누설 전류 (A/um)',
        63: '// I_off_p: PMOS 서브스레숄드 누설 전류 (A/um)',
        64: '// I_g_on_n: NMOS 게이트 누설 전류 (A/um) - 산화막 터널링',
        65: '// I_g_on_p: PMOS 게이트 누설 전류 (A/um)',
        66: '// C_ox: 산화막 커패시턴스 (fF/um²)',
        67: '// t_ox: 산화막 두께 (um)',
        68: '// n_to_p_eff_curr_drv_ratio: NMOS/PMOS 유효 전류 구동 비율 (PMOS를 이만큼 크게 만듦)',
        69: '// long_channel_leakage_reduction: 긴 채널 누설 감소 계수',
        102: '// InterconnectType 내부 클래스: 금속 배선층의 특성',
        105: '// pitch: 배선 피치 (um) = 배선 폭 + 간격',
        106: '// R_per_um: 단위 길이당 저항 (ohm/um)',
        107: '// C_per_um: 단위 길이당 커패시턴스 (fF/um)',
        108: '// horiz_dielectric_constant: 수평 유전 상수 (같은 층 배선 간)',
        109: '// vert_dielectric_constant: 수직 유전 상수 (다른 층 배선 간)',
        110: '// aspect_ratio: 배선 높이/폭 비율 (ITRS 기준 ~2.2)',
        111: '// miller_value: 밀러 커플링 계수 (인접 배선 활동 고려)',
        112: '// ild_thickness: 층간 유전체(ILD) 두께 (um)',
        130: '// MemoryType 내부 클래스: 메모리 셀(SRAM/DRAM/CAM)의 물리적 특성',
        133: '// b_w: 셀 폭 (um)',
        134: '// b_h: 셀 높이 (um)',
        135: '// cell_a_w: 액세스 트랜지스터 폭 (um)',
        136: '// cell_pmos_w: PMOS 셀 트랜지스터 폭 (um, SRAM용)',
        137: '// cell_nmos_w: NMOS 셀 트랜지스터 폭 (um)',
        138: '// Vbitpre: 비트라인 프리차지 전압 (V)',
        153: '// ScalingFactor 내부 클래스: 기술 스케일링 계수',
        156: '// logic_scaling_co_eff: 로직 스케일링 계수',
        157: '// core_tx_density: 코어 트랜지스터 밀도',
        158: '// long_channel_leakage_reduction: 긴 채널 누설 감소 계수',
        173: '// ram_wl_stitching_overhead_: RAM 워드라인 스티칭 오버헤드 (um)',
        174: '// min_w_nmos_: 최소 NMOS 폭 (um)',
        175: '// max_w_nmos_: 최대 NMOS 폭 (um)',
        176: '// max_w_nmos_dec: 디코더용 최대 NMOS 폭 (um)',
        177: '// unit_len_wire_del: 단위 길이 배선 지연 (초/um)',
        178: '// FO4: FO4 인버터 지연 (초) - 성능 기준 단위',
        179: '// kinv: 인버터 지연 상수',
        180: '// vpp: DRAM 워드라인 부스팅 전압 (V)',
        225: '// sram_cell: SRAM 셀 트랜지스터 파라미터',
        226: '// dram_acc: DRAM 액세스 트랜지스터 파라미터',
        227: '// dram_wl: DRAM 워드라인 트랜지스터 파라미터',
        228: '// peri_global: 주변회로 글로벌 트랜지스터 파라미터',
        229: '// cam_cell: CAM 셀 트랜지스터 파라미터',
        231: '// wire_local: 로컬 배선(Mat 내부) 파라미터',
        232: '// wire_inside_mat: Mat 내부 배선 파라미터',
        233: '// wire_outside_mat: Mat 외부 배선 파라미터',
        235: '// scaling_factor: 기술 스케일링 계수',
        237: '// sram: SRAM 메모리 셀 물리 파라미터',
        238: '// dram: DRAM 메모리 셀 물리 파라미터',
        239: '// cam: CAM 메모리 셀 물리 파라미터',
        285: '// DynamicParameter 클래스: 특정 배열 파티셔닝 구성에 대한 동적 파라미터',
        288: '// is_tag: 태그 배열인지 데이터 배열인지',
        289: '// pure_ram: 순수 RAM 모드',
        290: '// pure_cam: 순수 CAM 모드',
        291: '// fully_assoc: 완전연관 캐시 모드',
        292: '// tagbits: 태그 비트 수',
        293: '// num_subarrays: 뱅크당 서브어레이 수 (누설 전력 계산용)',
        294: '// num_mats: 뱅크당 Mat 수 (누설 전력 계산용)',
        295: '// Nspd: 서브어레이당 병렬도',
        296: '// Ndwl: 워드라인 방향 분할 수',
        297: '// Ndbl: 비트라인 방향 분할 수',
        298: '// Ndcm: 컬럼 먹싱 차수',
        299: '// deg_bl_muxing: 비트라인 먹싱 차수',
        300: '// deg_senseamp_muxing_non_associativity: 비연관 SA 먹싱 차수',
        355: '// out_w: 출력 폭 (비트)',
        356: '// is_main_mem: 메인 메모리 모드',
        357: '// cell: SRAM/DRAM 셀 면적, cam_cell: CAM 셀 면적',
        358: '// is_valid: 이 파라미터 조합이 유효한지',
        363: '// g_ip: 전역 입력 파라미터 포인터',
        364: '// g_tp: 전역 기술 파라미터 객체',
    }


def get_cacti_interface_h_annotations():
    """cacti_interface.h 주석 (핵심 부분만)"""
    return {
        34: '// 헤더 가드 시작 - CACTI 인터페이스 헤더',
        35: '// 헤더 가드 매크로',
        47: '// 전방 선언: min_values_t 클래스',
        48: '// 전방 선언: mem_array 클래스',
        49: '// 전방 선언: uca_org_t 클래스',
        52: '// powerComponents 클래스: 전력의 구성 요소들 (동적, 누설, 게이트 누설, 단락 전류)',
        55: '// dynamic: 동적 전력 (스위칭 시 소비, 0→1→0 전이)',
        56: '// leakage: 서브스레숄드 누설 전력 (게이트 OFF 상태에서도 흐르는 전류)',
        57: '// gate_leakage: 게이트 터널링 누설 전력 (얇은 산화막을 통한 전류)',
        58: '// short_circuit: 단락 전류 전력 (PMOS/NMOS 동시 도통 순간)',
        59: '// longer_channel_leakage: 긴 채널 트랜지스터의 감소된 누설',
        80: '// powerDef 클래스: 읽기/쓰기/검색 동작별 powerComponents를 묶은 전력 정의',
        83: '// readOp: 읽기 동작 전력 성분',
        84: '// writeOp: 쓰기 동작 전력 성분',
        85: '// searchOp: 검색(CAM/FA) 동작 전력 성분',
        94: '// Wire_type 열거형: 배선 유형 정의',
        96: '// Global: 리피터 삽입 최적 전역 배선',
        97: '// Global_5: 5% 지연 페널티 허용 배선 (전력 절감)',
        98: '// Global_10: 10% 지연 페널티 허용',
        99: '// Global_20: 20% 지연 페널티 허용',
        100: '// Global_30: 30% 지연 페널티 허용',
        101: '// Low_swing: 저전압 차동 배선 (높은 면적 오버헤드, 저전력)',
        102: '// Semi_global: 중간 레벨 배선',
        103: '// Transmission: 전송선 모델 (높은 면적 오버헤드)',
        104: '// Optical: 광학 배선',
        110: '// InputParameter 클래스: CACTI 사용자 입력 파라미터 집합',
        119: '// cache_sz: 캐시 크기 (바이트)',
        120: '// line_sz: 캐시 라인 크기 (바이트)',
        121: '// assoc: 연관도 (way 수)',
        122: '// nbanks: 뱅크 수',
        123: '// out_w: 출력 폭 (비트)',
        131: '// F_sz_nm: 공정 피처 크기 (nm)',
        132: '// F_sz_um: 공정 피처 크기 (um)',
        139: '// is_main_mem: 메인 메모리 모드',
        140: '// is_cache: 캐시 모드',
        141: '// pure_ram: 순수 RAM(스크래치패드) 모드',
        142: '// pure_cam: 순수 CAM 모드',
        355: '// uca_org_t 클래스: UCA 최적화 결과를 저장 (태그/데이터 배열, 접근시간, 전력, 면적)',
        358: '// tag_array2: 최적 태그 배열 구성',
        359: '// data_array2: 최적 데이터 배열 구성',
        360: '// access_time: 총 접근 시간',
        361: '// cycle_time: 사이클 시간',
        362: '// area: 전체 면적',
        363: '// area_efficiency: 면적 효율 (%)',
        364: '// power: 전체 전력 정의',
        384: '// reconfigure(): 입력 파라미터 변경 후 캐시를 재구성',
        386: '// cacti_interface(): CACTI 메인 인터페이스 - 설정 파일 또는 파라미터로 캐시를 모델링하고 결과 반환',
        545: '// mem_array 클래스: 하나의 배열(태그 또는 데이터) 구성에 대한 완전한 결과 저장',
        629: '// lt(): mem_array를 Nspd, Ndwl, Ndbl 순으로 정렬하기 위한 비교 함수',
    }


# 나머지 .cc 파일들의 주석은 패턴 기반으로 자동 생성
# 모든 .h 파일을 처리하고, .cc 파일은 주요 함수/블록 단위로 처리

def main():
    """메인 실행"""
    print("=== CACTI 소스코드 한글 주석 추가 시작 ===\n")

    # 헤더 파일 주석 추가
    h_files = {
        'Ucache.h': get_Ucache_h_annotations(),
        'mat.h': get_mat_h_annotations(),
        'bank.h': get_bank_h_annotations(),
        'decoder.h': get_decoder_h_annotations(),
        'htree2.h': get_htree2_h_annotations(),
        'wire.h': get_wire_h_annotations(),
        'parameter.h': get_parameter_h_annotations(),
        'cacti_interface.h': get_cacti_interface_h_annotations(),
        'router.h': get_router_h_annotations(),
        'subarray.h': get_subarray_h_annotations(),
        'uca.h': get_uca_h_annotations(),
        'nuca.h': get_nuca_h_annotations(),
        'crossbar.h': get_crossbar_h_annotations(),
        'highradix.h': get_highradix_h_annotations(),
        'arbiter.h': get_arbiter_h_annotations(),
        'io.h': get_io_h_annotations(),
    }

    print("[1/2] 헤더 파일 (.h) 주석 추가:")
    for fname, annotations in h_files.items():
        filepath = os.path.join(CACTI_DIR, fname)
        if os.path.exists(filepath):
            annotate_file(filepath, annotations)
        else:
            print(f"  WARNING: {fname} not found")

    # .cc 파일 주석 추가 - 함수 정의와 주요 로직 블록에 주석
    cc_annotations = get_cc_annotations()

    print("\n[2/2] 구현 파일 (.cc) 주석 추가:")
    for fname, annotations in cc_annotations.items():
        filepath = os.path.join(CACTI_DIR, fname)
        if os.path.exists(filepath):
            annotate_file(filepath, annotations)
        else:
            print(f"  WARNING: {fname} not found")

    print("\n=== 주석 추가 완료 ===")


def get_cc_annotations():
    """모든 .cc 파일의 주석"""
    return {
        'Ucache.cc': get_Ucache_cc_annotations(),
        'mat.cc': get_mat_cc_annotations(),
        'bank.cc': get_bank_cc_annotations(),
        'decoder.cc': get_decoder_cc_annotations(),
        'htree2.cc': get_htree2_cc_annotations(),
        'wire.cc': get_wire_cc_annotations(),
        'parameter.cc': {},  # technology params - mostly data tables
        'cacti_interface.cc': get_cacti_interface_cc_annotations(),
        'router.cc': get_router_cc_annotations(),
        'subarray.cc': get_subarray_cc_annotations(),
        'technology.cc': {},  # 기술 파라미터 데이터 테이블 - 주석 불필요
        'uca.cc': get_uca_cc_annotations(),
        'nuca.cc': {},  # NUCA 시뮬레이션 로직
        'crossbar.cc': get_crossbar_cc_annotations(),
        'highradix.cc': {},
        'arbiter.cc': get_arbiter_cc_annotations(),
        'io.cc': {},  # 출력 포맷팅 - 주석 불필요
        'main.cc': get_main_cc_annotations(),
    }


def get_Ucache_cc_annotations():
    return {
        55: '// nthreads: 멀티스레드 탐색에 사용할 스레드 수 (NTHREADS 상수)',
        58: '// update_min_values(): 다른 min_values_t 객체와 비교하여 최솟값 갱신',
        69: '// update_min_values(): UCA 결과에서 최솟값 갱신',
        78: '// update_min_values(): NUCA 결과에서 최솟값 갱신',
        87: '// update_min_values(): mem_array 결과에서 최솟값 갱신',
        98: '// calc_time_mt_wrapper(): pthread 래퍼 - 각 스레드가 배열 파티셔닝 공간의 일부를 탐색',
        117: '// Ndwl/Ndbl/Ndcm 반복 횟수 계산 (2의 거듭제곱으로 탐색)',
        141: '// Nspd: 병렬도를 Nspd_min부터 MAXDATASPD까지 2배씩 증가하며 탐색',
        143: '// wr: 배선 유형을 wt_min부터 wt_max까지 순회',
        145: '// iter: 스레드별로 Ndwl×Ndbl×Ndcm 조합을 분담 (stride=nthreads)',
        231: '// calculate_time(): 하나의 배열 파티셔닝 구성에 대해 UCA를 생성하고 결과를 기록',
        247: '// DynamicParameter 생성: 주어진 파라미터로 배열의 동적 특성 계산',
        254: '// UCA 객체 생성: 뱅크+H-tree 구조를 만들고 지연/전력/면적 즉시 계산',
        449: '// check_uca_org(): UCA 구성이 사용자 제약(deviation) 조건을 만족하는지 확인',
        473: '// check_mem_org(): mem_array 구성이 제약 조건을 만족하는지 확인',
        500: '// find_optimal_uca(): 유효한 UCA 구성들 중 가중 비용 함수가 최소인 것을 선택',
        585: '// filter_tag_arr(): 태그 배열 후보 중 제약을 만족하면서 비용이 최소인 것만 남김',
        642: '// filter_data_arr(): 데이터 배열 후보 중 지연과 동적 전력이 모두 50% 이상 나쁜 것을 제거',
        683: '// solve(): CACTI의 핵심 - 모든 가능한 배열 구성을 멀티스레드로 탐색하여 최적 캐시 구조를 찾음',
        706: '// calc_array: 스레드별 탐색 구조체 배열 생성',
        883: '// update(): 온도 변화 등에 의한 누설 전력 피드백 후 캐시 에너지 재계산',
    }


def get_mat_cc_annotations():
    return {
        38: '// Mat 생성자: 디코더, 프리디코더, 드라이버, 서브어레이 등 모든 하위 컴포넌트를 초기화하고 면적을 계산',
        66: '// 서브어레이 수 검증: Mat당 최대 4개, 행당 최대 2개',
        88: '// number_sa_subarray: 서브어레이당 센스앰프 수 = 열 수 / 비트라인 먹싱 차수',
        104: '// num_dec_signals: 디코딩 출력 수 = 서브어레이 행 수',
        154: '// row_dec: 행 디코더 생성 - 워드라인을 구동하는 최종 디코더',
        167: '// bit_mux_dec: 비트라인 먹스 디코더 생성',
        176: '// sa_mux_lev_1_dec: 센스앰프 먹스 레벨1 디코더 생성',
        185: '// sa_mux_lev_2_dec: 센스앰프 먹스 레벨2 디코더 생성',
        215: '// r_predec_blk1/2: 행 프리디코더 블록 1,2 생성',
        240: '// r_predec_blk_drv1/2: 행 프리디코더 블록 드라이버 1,2 생성',
        251: '// r_predec: 행 프리디코더 생성 (드라이버1 + 드라이버2 조합)',
        256: '// subarray_out_wire: 서브어레이 출력 배선 모델 생성',
        428: '// Mat 소멸자: 모든 동적 할당 객체 해제',
        478: '// compute_delays(): Mat 내 전체 지연 경로 계산 - CAM/FA/일반 캐시에 따라 다른 경로',
        604: '// compute_bit_mux_sa_precharge_sa_mux_wr_drv_wr_mux_h(): 비트먹스+SA+프리차지 등의 총 높이 계산',
        654: '// compute_cam_delay(): CAM 검색 경로의 전체 지연 계산 (5단계: 검색라인→매치라인→NAND→NOR→WL)',
        1318: '// compute_power_energy(): Mat 전체의 동적 에너지 및 누설 전력 계산',
    }


def get_bank_cc_annotations():
    return {
        38: '// Bank 생성자: DynamicParameter로 Bank를 초기화하고 내부 Mat 및 H-tree를 구성',
        43: '// 포트 수 설정: RWP(읽기쓰기), ERP(전용읽기), EWP(전용쓰기), SCHP(검색)',
        63: '// total_addrbits: 뱅크에 라우팅되는 총 주소 비트 수',
        64: '// datainbits: 뱅크 데이터 입력 비트 수',
        65: '// dataoutbits: 뱅크 데이터 출력 비트 수',
        84: '// htree_in_add: 주소 입력 H-tree 생성 (일반 캐시용)',
        114: '// num_addr_b_row_dec: 행 디코더 주소 비트 수 = log2(서브어레이 행 수)',
        121: '// Bank 소멸자: H-tree 객체 해제',
        135: '// compute_delays(): Bank 지연 = Mat 지연 (H-tree 지연은 UCA에서 처리)',
        142: '// compute_power_energy(): Bank 전체 전력 = Mat 전력 × 활성 Mat 수 + H-tree 전력',
        148: '// 일반 캐시: 활성 Mat 수만큼 동적 전력 곱하고, 전체 Mat 수만큼 누설 전력 곱함',
    }


def get_decoder_cc_annotations():
    return {
        44: '// Decoder 생성자: 디코딩 신호 수로부터 NAND 게이트 입력 수를 결정하고 logical effort로 사이징',
        74: '// num_addr_bits_dec: 디코더 입력 주소 비트 수 = log2(출력 신호 수)',
        106: '// 디코더 셀 높이 = h_dec(스케일 팩터) × cell.h',
        114: '// compute_widths(): logical effort 방법으로 각 단계의 최적 트랜지스터 폭 결정',
        117: '// p_to_n_sz_ratio: PMOS/NMOS 크기 비율 (전류 구동력 매칭용)',
        118: '// gnand2: NAND2 게이트의 logical effort (입력→출력 전류 비율)',
        119: '// gnand3: NAND3 게이트의 logical effort',
        136: '// F: 전체 electrical effort = 게이트 effort × (부하/입력) 커패시턴스 비',
        138: '// logical_effort(): 최적 게이트 수와 각 단계 폭을 계산하는 함수',
        154: '// compute_area(): 디코더 면적과 누설 전류 계산',
        190: '// compute_delays(): Horowitz 모델로 각 게이트 단계의 지연을 계산하고 합산',
        213: '// 첫 번째 게이트 (NAND2 또는 NAND3)의 RC 지연 계산',
        237: '// 최종 인버터: 워드라인을 구동하는 마지막 단계 (Vpp 전압 사용 가능)',
        285: '// PredecBlk 생성자: 주소 비트 수에 따라 NAND2/NAND3 게이트 구조를 결정',
        372: '// PredecBlk::compute_widths(): 1단계(L1) NAND와 2단계(L2) NAND의 사이징',
        559: '// PredecBlk::compute_area(): 프리디코더 블록의 면적, 누설 전류 계산',
        707: '// PredecBlk::compute_delays(): NAND2/NAND3 양쪽 경로의 지연을 각각 계산',
    }


def get_htree2_cc_annotations():
    return {
        39: '// Htree2 생성자: H-tree 유형에 따라 in_htree() 또는 out_htree()를 호출하여 구성',
        67: '// H-tree 유형별 분기: 주소, 데이터입력, 데이터출력, 검색입력, 검색출력',
        94: '// power_bit: 1비트당 전력. 전체 전력 = power_bit × 초기 버스 폭',
        104: '// input_nand(): H-tree 노드의 NAND 게이트 사이징 및 지연/전력 계산',
        135: '// output_buffer(): H-tree 노드의 tri-state 출력 버퍼 (NOT+NAND+NOR+전송 트랜지스터)',
        253: '// in_htree(): 입력 H-tree 계산 - 루트에서 리프까지 NAND 게이트로 신호를 제한하며 분배',
        263: '// h: 수평 H-tree 노드 수 = log2(ndwl/2), v: 수직 노드 수 = log2(ndbl/2)',
        454: '// out_htree(): 출력 H-tree 계산 - 리프에서 루트까지 tri-state 버퍼로 데이터를 수집',
    }


def get_wire_cc_annotations():
    return {
        35: '// Wire 주 생성자: 배선 유형과 길이로 지연/전력/면적 계산 후 단위를 um/초/줄로 변환',
        82: '// Wire 초기화 생성자: 정적 멤버(global, global_5 등)를 한 번만 초기화',
        124: '// calculate_wire_stats(): 배선 유형에 따라 적절한 모델의 지연/전력을 wire_length에 비례하여 계산',
        222: '// signal_fall_time(): 2단 인버터를 통한 신호의 하강 시간 계산 (Horowitz 모델 사용)',
        247: '// signal_rise_time(): 2단 인버터를 통한 신호의 상승 시간 계산',
        285: '// wire_cap(): 배선 커패시턴스 = 측벽 커플링 + 상하층 커플링 + 프린지 커패시턴스',
        363: '// wire_res(): 배선 저항 = 저항률 × 길이 / (폭 × 높이), alpha_scatter로 표면 산란 보정',
        400: '// low_swing_model(): 저전압 스윙 차동 배선 모델 (NAND+인버터+NMOS 드라이버+SA 수신기)',
        551: '// delay_optimal_wire(): 리피터 크기/간격을 최적화하여 지연을 최소화하는 모델',
        613: '// init_wire(): 다양한 리피터 설정으로 배선을 시뮬레이션하여 최적/준최적 설정을 찾음',
        656: '// update_fullswing(): 5%/10%/20%/30% 지연 오버헤드 내에서 전력이 최소인 리피터 설정 선택',
        713: '// wire_model(): 주어진 리피터 간격/크기로 배선의 지연/전력을 계산하는 내부 함수',
    }


def get_cacti_interface_cc_annotations():
    return {
        51: '// mem_array::lt(): 2개의 mem_array를 Nspd→Ndwl→Ndbl→deg_bl_muxing→Ndsam 순으로 비교하는 정렬 함수',
        69: '// uca_org_t::find_delay(): 캐시 접근 모드에 따라 총 접근 시간 결정',
        75: '// 순수 RAM/CAM/FA: 접근 시간 = 데이터 배열 접근 시간만',
        83: '// 빠른 접근 모드: 태그와 데이터 배열을 병렬 접근하고 max를 취함',
        90: '// 순차 접근 모드: 태그 접근 후 데이터 접근 (합산)',
        97: '// 일반 접근 모드: 태그와 데이터 병렬 접근, 단 데이터는 way-select 대기 후 전송',
        107: '// find_energy(): 전체 에너지 = 데이터 배열 + 태그 배열 (RAM/CAM은 데이터만)',
        117: '// find_area(): 캐시 면적 계산 (RAM은 데이터만, 캐시는 태그+데이터 나란히)',
        132: '// adjust_area(): McPAT용 면적 조정 - 면적 효율이 20% 미만이면 보정',
        148: '// find_cyc(): 사이클 시간 = max(태그 사이클, 데이터 사이클)',
        161: '// uca_org_t 기본 생성자: 포인터를 0으로 초기화',
        168: '// cleanup(): 동적 할당된 배열 객체 해제',
    }


def get_router_cc_annotations():
    return {
        36: '// MCPAT_Router 생성자: 크로스바 파라미터(전송 게이트 크기, 트랙 크기) 초기화',
        78: '// Cw3(): 3배 간격 배선의 커패시턴스 계산 (크로스바/아비터 제어선용)',
        86: '// gate_cap(): um→내부 단위 변환 후 게이트 커패시턴스 계산',
        92: '// diff_cap(): 확산 영역(드레인/소스) 커패시턴스 계산',
        101: '// transmission_buf_inpcap(): 전송 게이트 입력 커패시턴스 = NMOS + PMOS 확산 커패시턴스',
        117: '// crossbar_inpline(): 크로스바 입력선 커패시턴스 = 배선 + 전송게이트 + 입력 드라이버',
        136: '// tr_crossbar_power(): 전송 게이트 크로스바의 동적 전력',
        141: '// buffer_stats(): VC 버퍼를 SRAM Mat으로 모델링하여 전력/면적 계산',
        206: '// cb_stats(): 크로스바 전력/면적 계산 (Crossbar 클래스 사용)',
        228: '// get_router_power(): 라우터 전체 전력 = 버퍼 + 크로스바 + 아비터',
        258: '// get_router_delay(): 라우터 파이프라인 지연 계산 (FO4 기반)',
        271: '// get_router_area(): 라우터 면적 = 버퍼 높이 × I + 크로스바 폭',
    }


def get_subarray_cc_annotations():
    return {
        42: '// Subarray 생성자: 행/열 수, ECC 오버헤드, WL 스티칭 오버헤드를 고려하여 면적 계산',
        51: '// ECC 비트 추가: num_bits_per_ecc_b_ 비트마다 1개의 ECC 비트',
        52: '// ram_num_cells_wl_stitching: WL 스티칭 간격 (DRAM/SRAM에 따라 다름)',
        56: '// 서브어레이 높이 = 셀 높이 × 행 수',
        58: '// 서브어레이 폭 = 셀 폭 × 열 수 + WL 스티칭 오버헤드',
        78: '// FA/CAM: 높이는 CAM 셀 기준(+더미 행 1개), 폭은 CAM+RAM 합산',
        98: '// get_total_cell_area(): 순수 메모리 셀 면적 (면적 효율 계산에 사용)',
        121: '// compute_C(): 비트라인/워드라인의 커패시턴스 및 저항 계산',
        123: '// c_w_metal: 워드라인 방향 금속 배선 커패시턴스/셀',
        124: '// r_w_metal: 워드라인 방향 금속 배선 저항/셀',
        125: '// C_b_metal: 비트라인 방향 금속 배선 커패시턴스/셀',
        130: '// DRAM C_wl: 게이트 패스 트랜지스터 커패시턴스 + 금속 배선 커패시턴스',
        146: '// SRAM C_wl: 2개의 패스 트랜지스터(셀당 2 BL) + 금속 배선',
        148: '// C_bl: 행 수 × (드레인 커패시턴스 + 금속 커패시턴스)',
    }


def get_uca_cc_annotations():
    return {
        40: '// UCA 생성자: Bank + 뱅크 간 H-tree를 구성하고 즉시 지연/전력/면적을 계산',
        43: '// 뱅크 배치: 높이가 더 크면 수직 방향에 더 많은 뱅크 배치 (정사각형에 가깝게)',
        61: '// 뱅크당 주소/데이터/검색 비트 수 계산',
        75: '// 뱅크 간 H-tree 생성: 주소/데이터 입출력 각각에 대해 H-tree 생성',
        101: '// area_all_dataramcells: 전체 데이터 RAM 셀의 순수 면적 (면적 효율 계산용)',
        121: '// compute_delays(): UCA 전체 접근 시간 및 사이클 시간 계산',
        125: '// delay_array_to_mat: 배열 외부 H-tree + 뱅크 내부 H-tree의 주소 지연 합',
        136: '// delay_before_subarray_output_driver: 행 경로, 열 경로, SA먹스 경로 중 최대 지연',
        157: '// 일반 접근: 접근 시간 = 출력 드라이버 이전 지연 + 출력 드라이버~출력 지연',
        200: '// cycle_time: 파이프라인 없는 사이클 시간 = max(각 컴포넌트 지연)',
        224: '// compute_power_energy(): UCA 전체 전력 계산 (Bank 전력 + 뱅크 간 라우팅 전력)',
    }


def get_crossbar_cc_annotations():
    return {
        37: '// Crossbar 생성자: 입출력 포트 수, 플릿 크기, 디바이스 타입으로 초기화',
        44: '// min_w_pmos: n_to_p 비율 × 최소 NMOS 폭',
        51: '// output_buffer(): tri-state 출력 버퍼 사이징 - repeater 크기 기반으로 NAND+NOR+드라이버 결정',
        55: '// l_eff: 크로스바 입력선 유효 길이 = 입력 수 × 플릿 크기 × 배선 피치',
        91: '// compute_power(): 크로스바 동적/누설 전력 및 지연 계산 (종횡비 자동 조정)',
        115: '// 종횡비가 ASPECT_THRESHOLD 미만이면 CB_ADJ를 증가시켜 재귀적으로 재계산',
        127: '// 동적 전력 = 수평 배선 + 수직 배선 + tri-state 버퍼 스위칭',
        144: '// 지연 = Horowitz 모델로 크로스바 전체 RC 지연 계산',
    }


def get_arbiter_cc_annotations():
    return {
        34: '// MCPAT_Arbiter 생성자: NOR 기반 라운드로빈 아비터의 트랜지스터 사이징',
        45: '// NTn1/PTn1: 1단 NOR 게이트의 NMOS/PMOS 크기',
        49: '// NTi/PTi: 인버터의 NMOS/PMOS 크기',
        51: '// NTtr/PTtr: 전송 게이트의 NMOS/PMOS 길이',
        58: '// arb_req(): 요청 경로 커패시턴스 = (R-1)개 NOR1 + NOR2 + INV',
        66: '// arb_pri(): 우선순위 플립플롭의 스위칭 커패시턴스',
        74: '// arb_grant(): 허가 경로 커패시턴스 = NOR1 드레인 + 크로스바 제어선',
        80: '// arb_int(): 내부 경로 커패시턴스 = NOR1 드레인 + NOR2 게이트',
        87: '// compute_power(): 아비터 동적 전력 = 요청 + 우선순위 + 허가 + 내부',
        100: '// Cw3(): 3배 간격 배선 커패시턴스 (크로스바 제어선에 사용)',
        108: '// crossbar_ctrline(): 크로스바 제어선 커패시턴스 = 배선 + 인버터 드레인/게이트',
    }


def get_main_cc_annotations():
    return {
        38: '// main(): CACTI 독립 실행 진입점 - 명령줄 인자 또는 설정 파일로 캐시 모델링',
        41: '// result: 최적 캐시 구성 결과를 저장',
        42: '// argc 검사: 53개(McPAT 인터페이스) 또는 55개(Naveen 인터페이스) 인자',
        44: '// 설정 파일 모드: -infile 옵션으로 파일 지정',
        67: '// 설정 파일로 CACTI 실행',
        72: '// 53개 인자 모드: 모든 캐시 파라미터를 명령줄로 직접 지정',
        127: '// 55개 인자 모드: Naveen의 확장 인터페이스 (NUCA 파라미터 포함)',
        183: '// cleanup(): 결과 객체의 동적 메모리 해제',
    }


if __name__ == '__main__':
    main()
