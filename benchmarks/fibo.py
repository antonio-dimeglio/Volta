#!/usr/bin/env python3
import subprocess
import time
import matplotlib.pyplot as plt
import numpy as np

def fibonacci_python(n):
    """Naive recursive Fibonacci implementation in Python"""
    if n == 1 or n == 2:
        return 1
    else:
        return fibonacci_python(n - 1) + fibonacci_python(n - 2)

def benchmark_volta(runs=100):
    """Benchmark the compiled Volta fibonacci executable"""
    times = []
    for i in range(runs):
        start = time.perf_counter()
        subprocess.run(['./fibonacci'], capture_output=True, check=False)
        end = time.perf_counter()
        times.append(end - start)
        print(f"Volta run {i+1}/{runs}: {times[-1]:.4f}s")
    return times

def benchmark_python(n=40, runs=100):
    """Benchmark the Python fibonacci implementation"""
    times = []
    for i in range(runs):
        start = time.perf_counter()
        result = fibonacci_python(n)
        end = time.perf_counter()
        times.append(end - start)
        print(f"Python run {i+1}/{runs}: {times[-1]:.4f}s")
    return times

def plot_results(volta_times, python_times):
    """Create a comparison graph of the results"""
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    # Box plot comparison
    axes[0, 0].boxplot([volta_times, python_times], labels=['Volta', 'Python'])
    axes[0, 0].set_ylabel('Time (seconds)')
    axes[0, 0].set_title('Execution Time Distribution')
    axes[0, 0].grid(True, alpha=0.3)

    # Time series
    axes[0, 1].plot(volta_times, label='Volta', alpha=0.7, linewidth=1)
    axes[0, 1].plot(python_times, label='Python', alpha=0.7, linewidth=1)
    axes[0, 1].set_xlabel('Run Number')
    axes[0, 1].set_ylabel('Time (seconds)')
    axes[0, 1].set_title('Execution Time per Run')
    axes[0, 1].legend()
    axes[0, 1].grid(True, alpha=0.3)

    # Histograms
    axes[1, 0].hist(volta_times, bins=20, alpha=0.7, label='Volta', color='blue')
    axes[1, 0].set_xlabel('Time (seconds)')
    axes[1, 0].set_ylabel('Frequency')
    axes[1, 0].set_title('Volta Time Distribution')
    axes[1, 0].legend()
    axes[1, 0].grid(True, alpha=0.3)

    axes[1, 1].hist(python_times, bins=20, alpha=0.7, label='Python', color='orange')
    axes[1, 1].set_xlabel('Time (seconds)')
    axes[1, 1].set_ylabel('Frequency')
    axes[1, 1].set_title('Python Time Distribution')
    axes[1, 1].legend()
    axes[1, 1].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('fibonacci_benchmark.png', dpi=300, bbox_inches='tight')
    print("\nGraph saved as 'fibonacci_benchmark.png'")
    plt.show()

def print_statistics(volta_times, python_times):
    """Print statistical comparison"""
    print("\n" + "="*60)
    print("BENCHMARK RESULTS")
    print("="*60)
    print(f"\nVolta (Compiled with -O3):")
    print(f"  Mean:   {np.mean(volta_times):.4f}s")
    print(f"  Median: {np.median(volta_times):.4f}s")
    print(f"  Std:    {np.std(volta_times):.4f}s")
    print(f"  Min:    {np.min(volta_times):.4f}s")
    print(f"  Max:    {np.max(volta_times):.4f}s")

    print(f"\nPython (Naive Implementation):")
    print(f"  Mean:   {np.mean(python_times):.4f}s")
    print(f"  Median: {np.median(python_times):.4f}s")
    print(f"  Std:    {np.std(python_times):.4f}s")
    print(f"  Min:    {np.min(python_times):.4f}s")
    print(f"  Max:    {np.max(python_times):.4f}s")

    speedup = np.mean(python_times) / np.mean(volta_times)
    print(f"\n{'='*60}")
    print(f"Volta is {speedup:.2f}x faster than Python!")
    print(f"{'='*60}\n")

if __name__ == "__main__":
    print("Starting Fibonacci Benchmark (100 runs each)")
    print("="*60)

    print("\n[1/2] Running Volta benchmarks...")
    volta_times = benchmark_volta(runs=100)

    print("\n[2/2] Running Python benchmarks...")
    python_times = benchmark_python(n=40, runs=100)

    print_statistics(volta_times, python_times)
    plot_results(volta_times, python_times)