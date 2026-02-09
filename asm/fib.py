def fibrec(n):
    if n == 0:
        return 0
    elif n <= 2:
        return 1
    else:
        return fibrec(n-1)+fibrec(n-2)

def fibiter(n):
    a, b = 0, 1
    for i in range(n):
        tmp = b
        b += a
        a = tmp
    return a

for i in range(0, 11):
    print(f'fibrecursive({i}): {fibrec(i)}')
    print(f'fibiterative({i}): {fibiter(i)}\n')
