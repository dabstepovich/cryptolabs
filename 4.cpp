#include "math.h" // Для функции sqrt
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

const double PI = 3.14159265358979323846;
const int THREADS = 8;
const int TRIALS = 100000;
const int BATCH_SIZE = 1000;

// Кэш для проверки свободности от квадратов
unordered_map<long long, bool> square_free_cache;
mutex cache_mutex;

bool is_square_free(long long m) {
  // Проверка кэша
  {
    lock_guard<mutex> lock(cache_mutex);
    auto it = square_free_cache.find(m);
    if (it != square_free_cache.end()) {
      return it->second;
    }
  }

  // Проверка делимости на квадраты простых чисел
  if (m % 4 == 0)
    return false;

  long long max_test = sqrt(m) + 1;
  for (long long d = 3; d <= max_test; d += 2) {
    long long d_squared = d * d;
    if (d_squared > m)
      break;
    if (m % d_squared == 0) {
      lock_guard<mutex> lock(cache_mutex);
      square_free_cache[m] = false;
      return false;
    }
  }

  lock_guard<mutex> lock(cache_mutex);
  square_free_cache[m] = true;
  return true;
}

void process_batch(long long N, int batch_size, atomic<int> &count) {
  random_device rd;
  mt19937_64 gen(rd());
  uniform_int_distribution<long long> dis(1, N);

  int local_count = 0;
  for (int i = 0; i < batch_size; ++i) {
    long long m = dis(gen);
    if (is_square_free(m)) {
      local_count++;
    }
  }
  count += local_count;
}

double compute_probability(long long N) {
  atomic<int> count(0);
  vector<thread> threads;

  int full_batches = TRIALS / BATCH_SIZE;
  int remainder = TRIALS % BATCH_SIZE;

  for (int i = 0; i < THREADS; ++i) {
    int batches_per_thread = full_batches / THREADS;
    if (i < full_batches % THREADS)
      batches_per_thread++;

    threads.emplace_back([=, &count]() {
      for (int j = 0; j < batches_per_thread; ++j) {
        process_batch(N, BATCH_SIZE, count);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Обработка остатка
  if (remainder > 0) {
    process_batch(N, remainder, count);
  }

  return static_cast<double>(count) / TRIALS;
}

int main() {
  long long N_rand = pow(10, 15); // 10^15 для демонстрации
  vector<long long> N_range;

  // Генерируем диапазон значений N
  for (int i = 0; i <= 100; i += 10) {
    N_range.push_back(N_rand + i);
  }

  const double theoretical = 6.0 / (PI * PI);

  for (long long N : N_range) {
    auto start = chrono::high_resolution_clock::now();

    double empirical = compute_probability(N);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;

    cout << "N = " << N << ":\n";
    cout << "  Эмпирическая вероятность: " << empirical << "\n";
    cout << "  Теоретическая оценка: " << theoretical << "\n";
    cout << "  Разница: " << abs(empirical - theoretical) << "\n";
    cout << "  Время выполнения: " << elapsed.count() << " секунд\n\n";
  }

  return 0;
}
