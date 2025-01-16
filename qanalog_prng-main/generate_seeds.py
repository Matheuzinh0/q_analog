import random
from typing import List


def generate_seed(field_degree: int) -> str:
    """Generate a random polynomial of given degree as a string of indices"""
    # Generate random bits for coefficients
    coefficients = [random.randint(0, 1) for _ in range(field_degree + 1)]

    # Convert to string of indices where coefficient is 1
    indices = reversed([i for i, coef in enumerate(coefficients) if coef == 1])
    return " ".join(map(str, indices))


def generate_seeds(field_degree: int, num_seeds: int = 10) -> List[str]:
    """Generate multiple unique random seeds"""
    seeds = set()  # Use set to avoid duplicates
    while len(seeds) < num_seeds:
        seed = generate_seed(field_degree)
        seeds.add(seed)
    return list(seeds)


if __name__ == "__main__":
    field_degree = 256
    num_seeds = 5

    seeds = generate_seeds(field_degree, num_seeds)

    print("\nGenerated Seeds:")
    for i, seed in enumerate(seeds, 1):
        print(f"{i}. {seed}")

    # Save to file
    with open("generated_seeds.txt", "w") as f:
        f.write("\n".join(seeds))
    print("\nSeeds saved to generated_seeds.txt")
