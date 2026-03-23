/* bafs_ptr.h - BaM Array File System 스마트 포인터
 * NVMe SSD에 저장된 배열 데이터에 대해 일반 포인터처럼 사용할 수 있는 스마트 포인터 클래스이다.
 * 내부적으로 page_cache를 통해 필요한 데이터를 GPU 메모리로 가져오며,
 * 인덱스 연산([], *, ++, --)과 포인터 산술(+, -)을 지원한다.
 * GPU 커널 코드에서 SSD 데이터를 마치 배열처럼 투명하게 접근할 수 있게 해준다.
 */
#ifndef __BAFS_PTR_H__
#define __BAFS_PTR_H__

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif
#ifndef __forceinline__
#define __forceinline__ inline
#endif

#include "page_cache.h"
#include <cstdint>

/* bafs_ptr<T>: SSD 배열 데이터에 대한 스마트 포인터 클래스.
 * T는 배열 원소 타입이다. 내부적으로 array_d_t를 통해 page cache에서 데이터를 읽고 쓴다.
 */
template<typename T>
class bafs_ptr {
private:
    array_t<T>* h_pData;     // 호스트 측 배열 핸들 (통계 출력 등에 사용)
    array_d_t<T>* pData;     // 디바이스 측 배열 핸들 (실제 데이터 접근에 사용)
    uint64_t start_idx;      // 이 포인터가 가리키는 시작 인덱스 오프셋
public:
    /* print_stats: 호스트에서 page cache 접근 통계(hit/miss rate)를 출력하고 리셋한다 */
    __host__
    void print_stats() const {
        if (h_pData)
            h_pData->print_reset_stats();
    }

    /* 기본 생성자: 모든 멤버를 NULL/0으로 초기화한다 */
    __host__ __device__ bafs_ptr():
        h_pData(NULL), pData(NULL),start_idx(0){
    }

    /* 생성자: 디바이스 배열 포인터와 시작 오프셋으로 초기화한다 */
    __host__ __device__ bafs_ptr(array_d_t<T>* const pValue, const uint64_t start_off):
        h_pData(NULL), pData(pValue),start_idx(start_off){
    }

    /* 생성자: 호스트 배열 핸들로 초기화하고, 내부의 디바이스 포인터를 자동 설정한다 */
    __host__ __device__ bafs_ptr(array_t<T>* const pValue):
        h_pData(pValue), pData(pValue->d_array_ptr),start_idx(0){

    }

    /* 생성자: 호스트 배열 핸들과 시작 오프셋으로 초기화한다 */
    __host__ __device__ bafs_ptr(array_t<T>* const pValue, const uint64_t start_off):
        h_pData(pValue), pData(pValue->d_array_ptr),start_idx(start_off){
    }

    /* 소멸자: 특별한 정리 작업 없음 (page cache 참조 카운트는 bam_ptr가 관리) */
    __host__ __device__ ~bafs_ptr(){}

    /* 복사 생성자: 모든 멤버를 그대로 복사한다 */
    __host__ __device__ bafs_ptr(const bafs_ptr &var){
        h_pData = var.h_pData;
        pData = var.pData;
        start_idx = var.start_idx;
    }

    /* 역참조 연산자: 현재 start_idx 위치의 값을 SSD/캐시에서 읽어 반환한다 */
    __device__ T operator*(){
        return (*pData)[start_idx];
    }

    /* 대입 연산자: 자기 자신이 아닌 경우에만 모든 멤버를 복사한다 */
    __host__ __device__ bafs_ptr<T>& operator=(const bafs_ptr<T>& obj) {
        if(*this == obj)
            return *this;
        else{
            this->h_pData = obj.h_pData;
            this->pData = obj.pData;
            this->start_idx = obj.start_idx;
        }
        return *this;
    }

    /* 동등 비교 연산자를 friend로 선언한다 */
    template<typename T_>
    friend __host__ __device__ bool operator==(const bafs_ptr<T_>& lhs, const bafs_ptr<T_>& rhs);

    /* 함수 호출 연산자: 인덱스 i 위치에 val을 쓴다 (seq_write 호출) */
    __host__ __device__ void operator()(const uint64_t i, const T val) {
        (*pData)(i, val);
    }

    /* 인덱스 연산자 (읽기): start_idx + i 위치의 값을 SSD/캐시에서 읽어 반환한다 */
    __host__ __device__ T operator[](const uint64_t i) {
        return (*pData)[start_idx+i];
    }

    /* const 인덱스 연산자 (읽기 전용) */
    __host__ __device__ const T operator[](const uint64_t i) const {
        return (*pData)[start_idx+i];
    }

    /* 덧셈 연산자: 현재 위치에서 i만큼 앞으로 이동한 새 포인터를 반환한다 */
    __host__ __device__ bafs_ptr<T> operator+(const uint64_t i){
        uint64_t new_start_idx = this->start_idx+i;
        return bafs_ptr<T>(this->pData, new_start_idx);
    }

    /* 뺄셈 연산자: 현재 위치에서 i만큼 뒤로 이동한 새 포인터를 반환한다 */
    __host__ __device__ bafs_ptr<T> operator-(const uint64_t i){
        uint64_t new_start_idx = this->start_idx-i;
        return bafs_ptr<T>(this->pData, new_start_idx);
    }

    /* 후위 증가 연산자(i++): 현재 값을 복사해두고 start_idx를 1 증가시킨 후 복사본을 반환한다 */
    __host__ __device__ bafs_ptr<T> operator++(int){
        bafs_ptr<T> cpy = *this;
        this->start_idx += 1;
        return cpy;
    }

    /* 전위 증가 연산자(++i): start_idx를 1 증가시키고 자기 자신을 반환한다 */
    __host__ __device__ bafs_ptr<T>& operator++(){
        this->start_idx += 1;
        return *this;
    }

    /* 후위 감소 연산자(i--): 현재 값을 복사해두고 start_idx를 1 감소시킨 후 복사본을 반환한다 */
    __host__ __device__ bafs_ptr<T> operator--(int){
        bafs_ptr<T> cpy = *this;
        this->start_idx -= 1;
        return cpy;
    }

    /* 전위 감소 연산자(--i): start_idx를 1 감소시키고 자기 자신을 반환한다 */
    __host__ __device__ bafs_ptr<T>& operator--(){
        this->start_idx -= 1;
        return *this;
    }

    /* memcpy_to_array_aligned: 배열에서 정렬된 블록 단위로 dest에 데이터를 복사한다.
     * warp 단위 협력 복사를 내부적으로 사용한다.
     */
    __host__ __device__ void memcpy_to_array_aligned(const uint64_t src_idx, const uint64_t count, T* dest) const {
        pData->memcpy(src_idx, count, dest);
    }
};



/* 동등 비교 연산자 구현: pData, start_idx, h_pData가 모두 같으면 동일한 포인터로 판단한다 */
template<typename T_>
__host__ __device__
bool operator==(const bafs_ptr<T_>& lhs, const bafs_ptr<T_>& rhs){
   return (lhs.pData == rhs.pData && lhs.start_idx == rhs.start_idx && lhs.h_pData == rhs.h_pData);
}


//#ifndef __CUDACC__
//#undef __device__
//#undef __host__
//#undef __forceinline__
//#endif

#endif //__BAFS_PTR_H__
