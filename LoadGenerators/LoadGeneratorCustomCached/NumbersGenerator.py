import numpy as np

random_numbers = np.random.randint(-99999, 99999, size=100000, dtype=np.int32)

with open("../build/input.txt", "w") as f:
    for number in random_numbers:
        f.write(f"{number}\n")