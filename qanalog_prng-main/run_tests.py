import json
import subprocess
import os

# Configurações
TEST_BATTERIES = ["Rabbit","Alphabit"]  # Adicione outros testes conforme necessário
C_PROGRAM = "./gf2"
OUTPUT_DIR = "test_results"

def parse_h_base(h_str):
    return h_str.split()[0]

def main():
    with open("config.json", "r") as f:
        configs = json.load(f)
    
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    for config in configs:
        base_params = config["base_params"]
        seeds = config["seeds"]
        
        h_base = parse_h_base(base_params["h"])
        a_len = len(base_params["a"].split())
        
        for seed_idx, seed in enumerate(seeds):
            for test in TEST_BATTERIES:
                # Construir nome do arquivo
                output_file = f"e{h_base}_w{a_len}_seed{seed_idx}_{test}.txt"
                output_path = os.path.join(OUTPUT_DIR, output_file)
                
                # Construir comando
                cmd = [
                    C_PROGRAM,
                    base_params["h"],
                    base_params["a"],
                    base_params["c"],
                    seed,
                    test
                ]
                
                # Executar e capturar saída
                with open(output_path, "w") as f_out:
                    subprocess.run(cmd, stdout=f_out, stderr=subprocess.STDOUT)

if __name__ == "__main__":
    main()
