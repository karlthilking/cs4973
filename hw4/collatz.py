def collatz(n):
    return f(n,1)

def f(n, x):
    if n==1:
        return x
    elif n%2==0:
        return f(n//2, x+1)
    else:
        return f(3*n+1, x+1)

for i in range(1, 100):
    print(f'collatz({i}): {collatz(i)}')

