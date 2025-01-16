import subprocess
import threading
import pandas as pd
import os
from pathlib import Path
import math
from concurrent.futures import ThreadPoolExecutor
import json
import tempfile
from typing import Dict, List
import re
import shutil
import time
import hashlib
import traceback


class NISTTestAutomation:
    def __init__(self, configs: List[Dict[str, List[str]]], output_dir: str):
        """
        Initialize the automation framework

        Args:
            configs: List of dictionaries, each containing 'base_params' and 'seeds'
            output_dir: Directory to store results
        """
        self.configs = configs
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def decimal_to_coeff_string(self, decimal_seed: int) -> str:
        """Convert decimal seed to coefficient string format"""
        # Convert to binary and get positions of 1s
        binary = bin(decimal_seed)[2:]  # Remove '0b' prefix
        positions = [i for i, bit in enumerate(reversed(binary)) if bit == "1"]
        return " ".join(map(str, sorted(positions, reverse=True)))

    def calculate_iterations(
        self, field_degree: int, target_bits: int = 100_000_000
    ) -> int:
        """Calculate required iterations to generate enough bits"""
        bits_per_iteration = field_degree
        return math.ceil(target_bits / bits_per_iteration)

    def run_prng(
        self, seed: str, iterations: int, base_params: Dict[str, str], output_file: str
    ) -> None:
        """Run the C++ PRNG with given parameters"""
        cmd = [
            "./prng",
            base_params["h"],
            base_params["a"],
            base_params["c"],
            seed,
            str(iterations),
            output_file,
        ]

        subprocess.run(cmd, check=True)

    def run_cropper(self, input_file: str, output_file: str) -> None:
        """Run the cropper on the generated file"""
        cmd = ["python3", "./cropper.py", input_file, output_file]
        subprocess.run(cmd, check=True)

    def parse_nist_results(self, result_file: str) -> pd.DataFrame:
        """Parse NIST test results into a DataFrame"""
        results = []
        with open(result_file, "r") as f:
            content = f.read()

        # Extract test results using regex
        pattern = r"(\w+)\s+(\d+\/\d+)\s+(\d+\.\d+)"
        matches = re.findall(pattern, content)

        for test_name, proportion, p_value in matches:
            passed, total = map(int, proportion.split("/"))
            results.append(
                {
                    "test_name": test_name,
                    "proportion_passed": passed / total,
                    "p_value": float(p_value),
                }
            )

        return pd.DataFrame(results)

    def process_seed(self, seed: str, base_params: Dict[str, str]) -> None:
        """Process a single seed through the entire pipeline"""
        # Convert space-separated coefficients to decimal for seed and polynomial a
        def coeff_to_decimal(coeff_str: str) -> int:
            coeffs = [int(x) for x in coeff_str.split()]
            return sum(1 << coeff for coeff in coeffs)
        
        decimal_seed = coeff_to_decimal(seed)
        decimal_a = coeff_to_decimal(base_params['a'])
        
        # Create unique identifier for this run
        temp_id = hashlib.md5(str(decimal_seed).encode()).hexdigest()[:8]

        # Create folder structure
        field_degree = int(base_params["h"].split()[0])
        pol_h = base_params["h"].replace(" ", "_")
        pol_c = base_params["c"].replace(" ", "_")

        # Create the result path using decimal for polynomial a
        result_dir = self.output_dir / str(field_degree) / pol_h / str(decimal_a) / pol_c
        result_dir.mkdir(parents=True, exist_ok=True)

        # Check if result already exists
        final_filename = f"seed_{decimal_seed}.txt"
        final_result_path = result_dir / final_filename
        if final_result_path.exists():
            print(
                f"[NIST {temp_id}] Result already exists at {final_result_path}, skipping..."
            )
            return

        # Create temporary directory for intermediate files
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_dir = Path(temp_dir)
            # Calculate iterations needed
            iterations = self.calculate_iterations(field_degree)

            # Generate initial output with temporary hash-based filename
            raw_output = temp_dir / f"raw_output_{temp_id}.bin"
            self.run_prng(seed, iterations, base_params, str(raw_output))

            # Crop to exact size
            cropped_output = temp_dir / f"cropped_output_{temp_id}.bin"
            self.run_cropper(str(raw_output), str(cropped_output))

            print(raw_output.absolute())
            print(cropped_output.absolute())

            # Set up NIST test
            nist_input = str(Path(cropped_output).absolute())
            nist_result_filename = (
                f"finalAnalysisReport_{temp_id}.txt"  # Using hash instead of seed
            )

            # Create a script file instead of using stdin
            script_file = temp_dir / "nist_script.txt"
            with open(script_file, "w") as f:
                f.write(
                    f"""0
{nist_input}
1
0
100
1
"""
                )

            original_dir = os.getcwd()
            try:
                # Change to NIST directory
                os.chdir("./NIST-Statistical-Test-Suite/sts")

                # Run NIST test suite with script file
                nist_cmd = (
                    f"cat {script_file} | ./assess 1000000 {nist_result_filename}"
                )
                print(f"[NIST {temp_id}] Running command: {nist_cmd}")

                process = subprocess.Popen(
                    nist_cmd,
                    shell=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                )

                # Read and print output in real-time
                while True:
                    output = process.stdout.readline()
                    if output:
                        print(f"[NIST {temp_id}] {output.strip()}")

                    if process.poll() is not None:
                        break

                # Move results to the hierarchical directory while still in NIST directory
                nist_result_path = (
                    Path("experiments/AlgorithmTesting") / nist_result_filename
                )
                final_result_path = (
                    Path(original_dir)
                    / self.output_dir
                    / str(field_degree)
                    / pol_h
                    / str(decimal_a)
                    / pol_c
                    / final_filename
                )

                print(
                    f"[NIST {temp_id}] Looking for result file at: {nist_result_path.absolute()}"
                )
                print(f"[NIST {temp_id}] Moving to: {final_result_path.absolute()}")

                if not nist_result_path.exists():
                    raise FileNotFoundError(
                        f"NIST result file not found at {nist_result_path.absolute()}"
                    )

                # Use shutil.move instead of Path.rename for cross-device moves
                shutil.move(str(nist_result_path.absolute()), str(final_result_path))

                if not final_result_path.exists():
                    raise FileNotFoundError(
                        f"Failed to move result file to {final_result_path.absolute()}"
                    )

                print(f"[NIST {temp_id}] Successfully moved result file")

            finally:
                os.chdir(original_dir)

    def run_all_tests(self) -> None:
        """Run tests for all configurations"""
        for config in self.configs:
            base_params = config["base_params"]
            seeds = config["seeds"]
            for seed in seeds:
                try:
                    self.process_seed(seed, base_params)
                    print(
                        f"Completed processing seed: {seed} with base_params: {base_params}"
                    )
                except Exception as e:
                    print(
                        f"Error processing seed {seed} with base_params {base_params}: {e}"
                    )
                    print(traceback.format_exc())


# Example usage:
if __name__ == "__main__":
    # Example configurations
    configs = json.load(open("config.json"))

    automation = NISTTestAutomation(configs=configs, output_dir="nist_results")

    automation.run_all_tests()
    print("Testing complete. Results saved to nist_results")
