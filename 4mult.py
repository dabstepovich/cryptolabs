import random
import math
from sympy.ntheory import factorint
import matplotlib.pyplot as plt
import numpy as np
from functools import lru_cache
from multiprocessing import Pool, cpu_count, freeze_support
import time


# Кэшируем результаты проверки на свободность от квадратов
@lru_cache(maxsize=100000)
def is_square_free(m):
    print(m)
    """Оптимизированная проверка числа на свободность от квадратов с кэшированием"""
    factors = factorint(m)
    return all(exp == 1 for exp in factors.values())


def compute_batch(args):
    """Обработка пакета чисел в одном процессе"""
    N, batch_size = args
    count = 0
    for _ in range(batch_size):
        m = random.randint(1, N)
        if is_square_free(m):
            count += 1
    return count


def compute_square_free_probability(N, trials, num_workers=None, batch_size=100):
    """Вычисление вероятности с использованием многопроцессорности"""
    if num_workers is None:
        num_workers = cpu_count()

    total_count = 0
    batches = [(N, batch_size)] * (trials // batch_size)

    with Pool(processes=num_workers) as pool:
        results = pool.map(compute_batch, batches)
        total_count = sum(results)

    # Обработка остатка
    remainder = trials % batch_size
    if remainder > 0:
        total_count += compute_batch((N, remainder))

    return total_count / trials


def theoretical_estimate():
    return 6 / (math.pi**2)


def compare_square_free(trials=10**4, num_workers=None):
    """Сравнение эмпирической и теоретической вероятностей"""
    N = 10**50
    if num_workers is None:
        num_workers = cpu_count()

    empirical_probs = []
    theoretical_prob = theoretical_estimate()

    start = time.time()

    emp_prob = compute_square_free_probability(N, trials, num_workers)
    empirical_probs.append(emp_prob)
    print(f"\nN = {N}:")
    print(f"  Эмпирическая вероятность: {emp_prob:.6f}")
    print(f"  Теоретическая оценка: {theoretical_prob:.6f}")
    print(f"  Разница: {abs(emp_prob - theoretical_prob):.6f}")

    end = time.time()

    res = end - start
    print(res)


if __name__ == "__main__":
    freeze_support()  # Необходимо для Windows, на других ОС можно опустить

    # Параметры выполнения
    trials = 10**3  # Количество испытаний
    num_workers = None  # Использовать все доступные ядра

    compare_square_free(trials, num_workers)
