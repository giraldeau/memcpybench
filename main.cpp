#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <typeinfo>
#include <functional>
#include <cassert>
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
class MemcpySIMDsi : public Memcpy<T> {
public:
    MemcpySIMDsi() : Memcpy<T>("simd_si") {}
    void copy(vector<T> &dst, vector<T> &src) {
        int chunk = (16 / sizeof(T));
        int end = src.size() / chunk;
        for (int i = 0; i < end; i++) {
            __m128i A = _mm_loadu_si128(((const __m128i *) src.data()) + i);
            _mm_storeu_si128(((__m128i *) dst.data()) + i, A);
        }
    }
};

template<typename T>
class MemcpySIMDps : public Memcpy<T> {
public:
    MemcpySIMDps() : Memcpy<T>("simd_ps") {}
    void copy(vector<T> &dst, vector<T> &src) {
        float *src_data = (float *)(src.data());
        float *dst_data = (float *)(dst.data());
        uint n = src.size() / 4;
        for (uint i = 0; i < n; i += 4) {
            __m128 A = _mm_loadu_ps(src_data + i);
            _mm_storeu_ps(dst_data + i, A);
        }
    }
};

float benchmark_lambda(int &repeat_min,
                      chrono::microseconds &delay_min,
                      std::function<void ()> func) {
    chrono::microseconds elapsed = chrono::microseconds::zero();
    int repeat = 0;
    while (repeat < repeat_min && elapsed < delay_min) {
        auto t1 = chrono::steady_clock::now();
        func();
        auto t2 = chrono::steady_clock::now();
        elapsed += chrono::duration_cast<chrono::microseconds>(t2 - t1);
        repeat++;
    }
    return elapsed.count() / repeat;
}

void print_results(map<string, map<int, float>> &results) {
    // header
    for (const auto &item: results) {
        cout << item.first << ",";
    }
    cout << endl;

    // data
    if (results.size() > 0) {
        map<int, float> keys = results.begin()->second;
        for (const auto &key: keys) {
            int power = key.first;
            cout << power << ",";
            for (auto &item: results) {
                //item.first; // name
                //item.second; // map
                cout << item.second[power] << ",";
            }
            cout << endl;
        }
    }
}

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;

    vector<Memcpy<char>* > flavors = {
        new MemcpyScalar<char>(),
        new MemcpyStd<char>(),
        new MemcpyErms<char>(),
        new MemcpySIMDsi<char>(),
        new MemcpySIMDps<char>(),
    };

    int repeat_min = 10E5;
    auto delay_min = chrono::microseconds(100000); // 100ms min
    int power_max = 20;
    map<string, map<int, float>> results; // (power, avg_us)

    for (int power = 5; power <= power_max; power++) {
        int n = (1 << power);
        vector<char> src(n), dst(n);
        for (auto flavor: flavors) {
            cout << flavor->m_name << " " << power << endl;
            fill(src.begin(), src.end(), 42);
            fill(dst.begin(), dst.end(), 0);
            float elapsed = benchmark_lambda(repeat_min, delay_min,
                [&]() {
                    flavor->copy(dst, src);
                }
            );
            results[flavor->m_name][power] = elapsed;
            assert(dst[n - 1] == 42);
        }
    }

    print_results(results);

    return 0;
}
