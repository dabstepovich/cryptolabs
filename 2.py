import random
import math
import matplotlib.pyplot as plt


def is_coprime(a, b):
    return math.gcd(a, b) == 1


def simulate_probability(N, trials):
    coprime_count = 0
    for _ in range(trials):
        a = random.randint(1, N)
        b = random.randint(1, N)
        if is_coprime(a, b):
            coprime_count += 1
    return coprime_count / trials


def theoretical_probability(N):
    return 6 / (math.pi**2)  # Игнорируем O(ln N / N) для больших N


def compare(N_range, trials=10**4):
    empirical_probs = []
    theoretical_probs = []

    for N in N_range:
        emp_prob = simulate_probability(N, trials)
        theo_prob = theoretical_probability(N)

        empirical_probs.append(emp_prob)
        theoretical_probs.append(theo_prob)

        print(f"N = {N}:")
        print(f"  Эмпирическая вероятность: {emp_prob:.6f}")
        print(f"  Теоретическая оценка: {theo_prob:.6f}")
        print(f"  Разница: {abs(emp_prob - theo_prob):.6f}")
        print()

    plt.figure(figsize=(10, 6))
    plt.plot(N_range, empirical_probs, "bo-", label="Эмпирическая вероятность")
    plt.plot(N_range, theoretical_probs, "r--", label="Теоретическая оценка (6/π²)")
    plt.xlabel("N")
    plt.ylabel("Вероятность взаимной простоты")
    plt.title("Сравнение эмпирической и теоретической вероятности")
    plt.legend()
    plt.grid(True)
    plt.show()


# Генерация диапазона N ≈ 10^100
N_rand = 10**100
N_range = range(N_rand, N_rand + 101)  # Берем каждое 10-е число для экономии времени

compare(N_range)
