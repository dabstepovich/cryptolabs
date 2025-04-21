import random
import math
import matplotlib.pyplot as plt


def compute_probability(N, trials):
    count = 0
    for _ in range(trials):
        m = random.randint(1, N)
        r = N % m
        if r < m / 2:
            count += 1
    return count / trials


def theoretical_estimate():
    return 2 - 2 * math.log(2)  # ≈ 0.6137


def compare_probabilities(N_range, trials=10**4):
    empirical_probs = []
    theoretical_prob = theoretical_estimate()

    for N in N_range:
        emp_prob = compute_probability(N, trials)
        empirical_probs.append(emp_prob)

        print(f"N = {N}:")
        print(f"  Эмпирическая вероятность: {emp_prob:.6f}")
        print(f"  Теоретическая оценка: {theoretical_prob:.6f}")
        print(f"  Разница: {abs(emp_prob - theoretical_prob):.6f}")
        print()

    plt.figure(figsize=(10, 6))
    plt.plot(N_range, empirical_probs, "bo-", label="Эмпирическая вероятность")
    plt.axhline(
        y=theoretical_prob,
        color="r",
        linestyle="--",
        label="Теоретическая оценка (2 - 2 ln 2)",
    )
    plt.xlabel("N")
    plt.ylabel("Вероятность (r < m/2)")
    plt.title("Вероятность остатка r < m/2 при делении N на m")
    plt.legend()
    plt.grid(True)
    plt.show()


# Генерация диапазона N ≈ 10^100
N_rand = 10**100
N_range = range(N_rand, N_rand + 101)  # Шаг 10 для экономии времени

compare_probabilities(N_range)
