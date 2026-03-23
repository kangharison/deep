/*
 * zip_test.cpp - Zipfian 분포 생성기 테스트 프로그램
 *
 * zip.h의 zipf_distribution 클래스가 올바르게 동작하는지 확인하는 단위 테스트.
 * 100개의 고유 키에서 Zipfian(s=0.5) 분포로 10000개의 샘플을 생성하여 출력한다.
 * 출력 결과를 히스토그램으로 확인하면 소수의 키에 접근이 집중되는 것을 관찰할 수 있다.
 */
/**
 * Zipf (Zeta) random distribution.
 *
 * Implementation taken from drobilla's May 24, 2017 answer to
 * https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
 *
 * That code is referenced with this:
 * "Rejection-inversion to generate variates from monotone discrete
 * distributions", Wolfgang Hörmann and Gerhard Derflinger
 * ACM TOMACS 6.3 (1996): 169-184
 *
 * Note that the Hörmann & Derflinger paper, and the stackoverflow
 * code base incorrectly names the paramater as `q`, when they mean `s`.
 * Thier `q` has nothing to do with the q-series. The names in the code
 * below conform to conventions.
 *
 * Example usage:
 *
 *    std::random_device rd;
 *    std::mt19937 gen(rd());
 *    zipf_distribution<> zipf(300);
 *
 *    for (int i = 0; i < 100; i++)
 *        printf("draw %d %d\n", i, zipf(gen));
 */


#include <iostream> 
#include <zip.h>


/*
 * main - Zipfian 분포 테스트 메인 함수
 *
 * 100개의 고유 키(N=100), 지수 s=0.5로 Zipfian 분포를 생성하고
 * 10,000번 샘플링하여 각 값을 출력한다.
 * s=0.5는 비교적 균등에 가까운 분포이며, s를 1.0 이상으로 올리면
 * 소수 키에 접근이 집중된다.
 *
 * @return: 항상 0 (암시적)
 */
int main(){

      std::random_device rd;                        /* 하드웨어 엔트로피 기반 시드 생성 */
      std::mt19937 gen(rd());                       /* Mersenne Twister PRNG 초기화 */
      zipf_distribution<uint64_t> zipf(100, 0.5);   /* N=100개 고유 키, 지수 s=0.5 */

      for (int i = 0; i < 10000; i++)               /* 10,000개 샘플 생성 */
          printf("i: %d val: %llu\n", i, zipf(gen));
}
