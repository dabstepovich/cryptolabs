import random
import math
from sympy.ntheory import factorint
import matplotlib.pyplot as plt
from concurrent.futures import ThreadPoolExecutor, as_completed
import numpy as np
from functools import lru_cache


# Кэшируем результаты проверки на свободность от квадратов
@lru_cache(maxsize=100000)
def is_square_free(m):
    """Оптимизированная проверка числа на свободность от квадратов с кэшированием"""
    factors = factorint(m)
    return all(exp == 1 for exp in factors.values())


def compute_batch(args):
    """Обработка пакета чисел в одном потоке"""
    N, batch_size = args
    count = 0
    for _ in range(batch_size):
        m = random.randint(1, N)
        print(m)
        if is_square_free(m):
            count += 1
    return count


def compute_square_free_probability(N, trials, num_workers=8, batch_size=100):
    """Вычисление вероятности с использованием многопоточности"""
    total_count = 0
    batches = [(N, batch_size)] * (trials // batch_size)
    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        futures = [executor.submit(compute_batch, batch) for batch in batches]
        for future in futures:
            total_count += future.result()
    # Обработка остатка
    remainder = trials % batch_size
    if remainder > 0:
        total_count += compute_batch((N, remainder))
    return total_count / trials


def theoretical_estimate():
    return 6 / (math.pi**2)


def compare_square_free(N_range, trials=10**4, num_workers=8):
    empirical_probs = []
    theoretical_prob = theoretical_estimate()
    for N in N_range:
        emp_prob = compute_square_free_probability(N, trials, num_workers)
        empirical_probs.append(emp_prob)
        print(f"\nN = {N}:")
        print(f"  Эмпирическая вероятность: {emp_prob:.6f}")
        print(f"  Теоретическая оценка: {theoretical_prob:.6f}")
        print(f"  Разница: {abs(emp_prob - theoretical_prob):.6f}")


# Параметры выполнения
N_rand = 10**50  # Уменьшил до более реалистичного значения
N_range = list(range(N_rand, N_rand + 101, 10))  # Проверяем 10 значений
trials = 10**3  # Количество испытаний
num_workers = 12  # Количество потоков

compare_square_free(N_range, trials, num_workers)
