import json
import subprocess
import os

# Caminho para o executável PRNG
EXECUTABLE = "./prng"

# Iterações fixas para o PRNG
ITERATIONS = "33554432"

# Diretório para os arquivos de saída
OUTPUT_DIR = "outputs"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Carregar o arquivo JSON
with open("config.json", "r") as json_file:
    configs = json.load(json_file)

# Iterar pelas configurações e sementes
for config_idx, config in enumerate(configs):
    base_params = config["base_params"]
    seeds = config["seeds"]

    for seed_idx, seed in enumerate(seeds):
        # Montar os argumentos
        h = base_params["h"]
        a = base_params["a"]
        c = base_params["c"]
        seed_arg = seed

        # Nome do arquivo de saída
        output_file = os.path.join(OUTPUT_DIR, f"out_config{config_idx}_seed{seed_idx}.bin")

        # Construir o comando
        command = [
            EXECUTABLE,
            h,          # Polinômio H
            a,          # Polinômio A
            c,          # Polinômio C
            seed_arg,   # Semente
            ITERATIONS, # Número de iterações
            output_file # Arquivo de saída
        ]

        # Executar o comando
        try:
            print(f"Executando: {' '.join(command)}")
            subprocess.run(command, check=True)
            print(f"Arquivo gerado: {output_file}")
        except subprocess.CalledProcessError as e:
            print(f"Erro ao executar o comando: {' '.join(command)}")
            print(e)

