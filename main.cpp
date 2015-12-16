#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <typeinfo>
#include <x86intrin.h>

using namespace std;

extern "C" {
    void memcpy_erms(void *dst, void *src, size_t size);
}

template<typename T>
class Memcpy {
public:
    Memcpy(const string &name) : m_name(name) {}
    virtual void copy(vector<T> &dst, vector<T> &src) = 0;
    bool check(vector<T> &dst, vector<T> &src) {
        for (uint i = 0; i < dst.size(); i++) {
            if (dst[i] != src[i]) {
                return false;
            }
        }
        return true;
    }
    string m_name;
};

template<typename T>
class MemcpyScalar : public Memcpy<T> {
public:
    MemcpyScalar() : Memcpy<T>("scalar") {}
    void copy(vector<T> &dst, vector<T> &src) {
        for (uint i = 0; i < dst.size(); i++) {
            dst[i] = src[i];
        }
    }
};

template<typename T>
class MemcpyStd : public Memcpy<T> {
public:
    MemcpyStd() : Memcpy<T>("std") {}
    void copy(vector<T> &dst, vector<T> &src) {
        std::copy(src.begin(), src.end(), dst.begin());
    }
};

template<typename T>
class MemcpyErms : public Memcpy<T> {
public:
    MemcpyErms() : Memcpy<T>("erms") {}
    void copy(vector<T> &dst, vector<T> &src) {
        memcpy_erms(dst.data(), src.data(), sizeof(T) * src.size());
    }
};

template<typename T>
class MemcpySIMD : public Memcpy<T> {
public:
    MemcpySIMD() : Memcpy<T>("simd") {}
    void copy(vector<T> &dst, vector<T> &src) {
        int chunk = (16 / sizeof(T));
        int end = src.size() / chunk;
        for (int i = 0; i < end; i++) {
            __m128i A = _mm_loadu_si128(((const __m128i *) src.data()) + i);
            _mm_storeu_si128(((__m128i *) dst.data()) + i, A);
        }
    }
};

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;

    int n = 1024 * 16;
    vector<char> src(n), dst(n);

    vector<Memcpy<char>* > flavor = {
        new MemcpyScalar<char>(),
        new MemcpyStd<char>(),
        new MemcpyErms<char>(),
        new MemcpySIMD<char>(),
    };

    for (auto fl: flavor) {
        // reset
        fill(src.begin(), src.end(), 42);
        fill(dst.begin(), dst.end(), 0);

        chrono::microseconds elapsed = chrono::microseconds::zero();
        auto min_delay = chrono::microseconds(1000000);
        int repeat = 0;
        while (repeat < 1000 && elapsed < min_delay) {
            auto t1 = chrono::steady_clock::now();
            fl->copy(dst, src);
            auto t2 = chrono::steady_clock::now();
            elapsed += chrono::duration_cast<chrono::microseconds>(t2 - t1);
            repeat++;
        }
        std::cout << fl->m_name << ": " << elapsed.count() << std::endl;
    }

    return 0;
}
