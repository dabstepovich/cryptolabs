import random
import math
import matplotlib.pyplot as plt


def gcd_with_division_count(a, b):
    count = 0
    while b != 0:
        a, b = b, a % b
        count += 1
    return count


def theoretical_estimate(N):
    return (12 * math.log(2)) / (math.pi**2) * math.log(N)


def simulate_average_divisions(N, trials):
    total_divisions = 0
    for _ in range(trials):
        a = random.randint(1, N)
        b = random.randint(1, N)
        total_divisions += gcd_with_division_count(a, b)
    return total_divisions / trials


def compare_divisions(N_range, trials=10**4):
    empirical_avg = []
    theoretical_avg = []

    for N in N_range:
        emp_avg = simulate_average_divisions(N, trials)
        theo_avg = theoretical_estimate(N)

        empirical_avg.append(emp_avg)
        theoretical_avg.append(theo_avg)

        print(f"N = {N}:")
        print(f"  Эмпирическое среднее: {emp_avg:.4f}")
        print(f"  Теоретическая оценка: {theo_avg:.4f}")
        print(f"  Разница: {abs(emp_avg - theo_avg):.4f}")
        print()

    plt.figure(figsize=(10, 6))
    plt.plot(N_range, empirical_avg, "bo-", label="Эмпирическое среднее")
    plt.plot(N_range, theoretical_avg, "r--", label="Теоретическая оценка")
    plt.xlabel("N")
    plt.ylabel("Среднее число делений")
    plt.title("Сравнение среднего числа делений в алгоритме Евклида")
    plt.legend()
    plt.grid(True)
    plt.show()


# Генерация диапазона N ≈ 10^100
N_rand = 10**100
N_range = range(N_rand, N_rand + 101)  # Шаг 10 для экономии времени

compare_divisions(N_range)
