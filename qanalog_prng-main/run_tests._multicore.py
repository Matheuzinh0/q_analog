import json
import subprocess
import os
from concurrent.futures import ThreadPoolExecutor, as_completed

# Configurações
TEST_BATTERIES = ["Crush"]
C_PROGRAM = "./prng"
OUTPUT_DIR = "test_results"

def process_task(config, seed_idx, seed, test):
    base_params = config["base_params"]
    
    h_base = base_params["h"].split()[0]
    a_len = len(base_params["a"].split())
    
    # Nome do arquivo usando o índice da semente
    output_file = f"e{h_base}_w{a_len}_seed{seed_idx}_{test}.txt"
    output_path = os.path.join(OUTPUT_DIR, output_file)
    
    cmd = [
        C_PROGRAM,
        base_params["h"],
        base_params["a"],
        base_params["c"],
        seed,  # Semente completa é usada no comando
        test
    ]
    
    with open(output_path, "w") as f_out:
        subprocess.run(cmd, stdout=f_out, stderr=subprocess.STDOUT)

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    with open("config_for_crush.json", "r") as f:
        configs = json.load(f)
    
    tasks = []
    for config in configs:
        for seed_idx, seed in enumerate(config["seeds"]):
            for test in TEST_BATTERIES:
                tasks.append((config, seed_idx, seed, test))
    
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = [executor.submit(process_task, *task) for task in tasks]
        
        for future in as_completed(futures):
            try:
                future.result()
            except Exception as e:
                print(f"Erro: {e}")

if __name__ == "__main__":
    main()
