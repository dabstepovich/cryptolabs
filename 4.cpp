#include <atomic>
#include <chrono>
#include <cmath>
#include <gmp.h>
#include <gmpxx.h>
#include <iostream>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// Специальный класс для использования mpz_class в качестве ключа в
// unordered_map
struct mpz_class_hash {
  std::size_t operator()(const mpz_class &z) const {
    return mpz_get_ui(z.get_mpz_t()) ^ mpz_sizeinbase(z.get_mpz_t(), 2);
  }
};

struct mpz_class_equal {
  bool operator()(const mpz_class &lhs, const mpz_class &rhs) const {
    return mpz_cmp(lhs.get_mpz_t(), rhs.get_mpz_t()) == 0;
  }
};

// Кэш для результатов проверки бесквадратности
std::unordered_map<mpz_class, bool, mpz_class_hash, mpz_class_equal>
    squarefree_cache;
std::shared_mutex cache_mutex; // Для многопоточного доступа к кэшу

// Кэш для проверки простоты чисел
std::unordered_map<mpz_class, bool, mpz_class_hash, mpz_class_equal>
    prime_cache;
std::shared_mutex prime_cache_mutex;

// Кэш для множителей
std::unordered_map<mpz_class, mpz_class, mpz_class_hash, mpz_class_equal>
    factor_cache;
std::shared_mutex factor_cache_mutex;

// Мьютекс для синхронизации вывода
std::mutex cout_mutex;

// Атомарный счетчик бесквадратных чисел
std::atomic<int> squarefree_count(0);

// Счетчик кэш-хитов
std::atomic<int> cache_hits(0);

// Функция для НОД
mpz_class gcd(mpz_class a, mpz_class b) {
  mpz_class result;
  mpz_gcd(result.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
  return result;
}

// Функция быстрого возведения в степень по модулю
mpz_class powmod(mpz_class base, mpz_class exp, const mpz_class &mod) {
  mpz_class result;
  mpz_powm(result.get_mpz_t(), base.get_mpz_t(), exp.get_mpz_t(),
           mod.get_mpz_t());
  return result;
}

// Тест Миллера-Рабина на простоту с кэшированием
bool is_prime(const mpz_class &n, int iterations = 20) {
  if (n <= 1)
    return false;
  if (n <= 3)
    return true;
  if (n % 2 == 0)
    return false;

  // Проверяем в кэше
  {
    std::shared_lock<std::shared_mutex> lock(prime_cache_mutex);
    auto it = prime_cache.find(n);
    if (it != prime_cache.end()) {
      cache_hits++;
      return it->second;
    }
  }

  // Представляем n - 1 как d * 2^r
  mpz_class d = n - 1;
  int r = 0;
  while (d % 2 == 0) {
    d /= 2;
    r++;
  }

  // Используем встроенную функцию GMP для теста Миллера-Рабина
  int is_prime_result = mpz_probab_prime_p(n.get_mpz_t(), iterations);
  bool result = is_prime_result != 0;

  // Кэшируем результат
  {
    std::unique_lock<std::shared_mutex> lock(prime_cache_mutex);
    prime_cache[n] = result;
  }

  return result;
}

// Ро-алгоритм Полларда для факторизации с кэшированием
mpz_class pollard_rho(const mpz_class &n) {
  if (n % 2 == 0)
    return 2;

  if (is_prime(n))
    return n;

  // Проверяем кэш множителей
  {
    std::shared_lock<std::shared_mutex> lock(factor_cache_mutex);
    auto it = factor_cache.find(n);
    if (it != factor_cache.end()) {
      cache_hits++;
      return it->second;
    }
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(1, 100);

  mpz_class x = dis(gen);
  mpz_class y = x;
  mpz_class c = dis(gen);
  mpz_class d = 1;

  // f(x) = x^2 + c mod n
  auto f = [&](const mpz_class &x) {
    mpz_class result = (x * x + c) % n;
    return result;
  };

  int max_iterations = 10000; // Ограничение для очень сложных факторизаций
  int iterations = 0;

  while (d == 1 && iterations < max_iterations) {
    x = f(x);
    y = f(f(y));
    d = gcd(abs(x - y), n);
    iterations++;

    if (d == n) {
      // Если не удалось найти нетривиальный множитель, пробуем с другими
      // начальными значениями
      x = dis(gen);
      y = x;
      c = dis(gen);
      d = 1;
    }
  }

  // Если не нашли множитель за max_iterations, возвращаем само число
  if (iterations >= max_iterations && d == 1) {
    d = n;
  }

  // Кэшируем результат
  {
    std::unique_lock<std::shared_mutex> lock(factor_cache_mutex);
    factor_cache[n] = d;
  }

  return d;
}

// Функция для проверки, является ли число бесквадратным
bool is_squarefree(const mpz_class &n) {
  if (n == 1)
    return true;
  if (n == 0)
    return false;

  // Проверяем кэш бесквадратности
  {
    std::shared_lock<std::shared_mutex> lock(cache_mutex);
    auto it = squarefree_cache.find(n);
    if (it != squarefree_cache.end()) {
      cache_hits++;
      return it->second;
    }
  }

  std::vector<mpz_class> factors;
  mpz_class temp = n;

  // Факторизация числа с использованием ро-алгоритма Полларда
  while (temp > 1) {
    mpz_class factor = pollard_rho(temp);
    factors.push_back(factor);
    temp /= factor;
  }

  // Сортируем множители
  std::sort(factors.begin(), factors.end());

  // Проверяем, есть ли повторяющиеся множители
  bool result = true;
  for (size_t i = 1; i < factors.size(); i++) {
    if (factors[i] == factors[i - 1]) {
      result = false; // Число не бесквадратное
      break;
    }
  }

  // Кэшируем результат
  {
    std::unique_lock<std::shared_mutex> lock(cache_mutex);
    squarefree_cache[n] = result;
  }

  return result;
}

// Функция для генерации случайного большого числа в заданном диапазоне
mpz_class random_mpz(const mpz_class &min, const mpz_class &max) {
  mpz_class range = max - min + 1;
  gmp_randstate_t state;
  gmp_randinit_default(state);
  gmp_randseed_ui(state, std::random_device{}());

  mpz_class result;
  mpz_urandomm(result.get_mpz_t(), state, range.get_mpz_t());
  result += min;

  gmp_randclear(state);
  return result;
}

// Функция для работы каждого потока
void worker_thread(const mpz_class &N, int trials_per_thread, int thread_id) {
  int local_squarefree_count = 0;
  std::random_device rd;
  std::mt19937 gen(rd() + thread_id); // Разные семена для разных потоков

  for (int i = 0; i < trials_per_thread; i++) {
    // Генерируем случайное число от 1 до N
    mpz_class m = random_mpz(1, N);
    std::cout << "Processing m = " << m << std::endl;

    // Проверяем, является ли оно бесквадратным
    if (is_squarefree(m)) {
      local_squarefree_count++;
    }

    // Периодически выводим прогресс
    if ((i + 1) % (trials_per_thread / 10) == 0 || trials_per_thread < 10) {
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << "Thread " << thread_id << ": "
                << (i + 1) * 100 / trials_per_thread << "% complete"
                << std::endl;
    }
  }

  // Обновляем глобальный счетчик
  squarefree_count += local_squarefree_count;
}

int main() {
  // Константы из условия задачи
  int M = 1000; // Количество испытаний
  mpz_class Nrand(
      "1000000000000000000000000000000000000000000000000000"); // 10^50

  // Диапазон для N
  mpz_class N_start = Nrand;
  mpz_class N_end = Nrand + 100;

  // Определение количества потоков
  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0)
    num_threads = 4; // Если не удалось определить, используем 4 потока

  std::cout << "Using " << num_threads << " threads" << std::endl;

  // Для каждого N в диапазоне
  for (mpz_class N = N_start; N <= N_end; N += 10) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "Processing N = " << N << std::endl;

    // Сбрасываем счетчики
    squarefree_count = 0;
    cache_hits = 0;

    // Распределяем работу между потоками
    int trials_per_thread = M / num_threads;
    int remaining_trials = M % num_threads;

    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < num_threads; i++) {
      int thread_trials =
          trials_per_thread + (i < (unsigned int)remaining_trials ? 1 : 0);
      threads.push_back(std::thread(worker_thread, N, thread_trials, i));
    }

    // Ждем завершения всех потоков
    for (auto &t : threads) {
      if (t.joinable()) {
        t.join();
      }
    }

    // Вычисляем экспериментальную вероятность
    double experimental_prob = static_cast<double>(squarefree_count) / M;

    // Вычисляем теоретическую вероятность
    double pi = 3.14159265358979323846;
    double theoretical_prob = 6.0 / (pi * pi);
    double correction = 1.0 / sqrt(N.get_d());
    double theoretical_with_correction = theoretical_prob + correction;

    // Выводим результаты
    std::cout << "Results for N = " << N << ":" << std::endl;
    std::cout << "  Squarefree numbers found: " << squarefree_count
              << " out of " << M << std::endl;
    std::cout << "  Experimental probability: " << experimental_prob
              << std::endl;
    std::cout << "  Theoretical probability (6/pi^2): " << theoretical_prob
              << std::endl;
    std::cout << "  Theoretical with O(1/sqrt(N)) correction: "
              << theoretical_with_correction << std::endl;
    std::cout << "  Difference: "
              << std::abs(experimental_prob - theoretical_prob) << std::endl;
    std::cout << "  Cache hits: " << cache_hits << std::endl;
    std::cout << "  Current cache sizes:" << std::endl;
    std::cout << "    Squarefree cache: " << squarefree_cache.size()
              << " entries" << std::endl;
    std::cout << "    Prime cache: " << prime_cache.size() << " entries"
              << std::endl;
    std::cout << "    Factor cache: " << factor_cache.size() << " entries"
              << std::endl;

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time)
            .count();
    std::cout << "  Computation time: " << duration << " seconds" << std::endl;
    std::cout << std::endl;
  }

  return 0;
}
