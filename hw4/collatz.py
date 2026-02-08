def collatz(n):
    return f(n, 1)

def f(n, x):
    while True:
        if n == 1:
            break
        elif n % 2 == 0:
            n //= 2; x += 1
        else:
            n = 3 * n + 1; x += 1
    return x

for i in range(1, 100):
    if collatz(i) < 100:
        continue
    print(f'collatz({i}): {collatz(i)} > 100')

