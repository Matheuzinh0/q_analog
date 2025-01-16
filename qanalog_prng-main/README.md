## Setup

0. Make sure to have python and g++ installed.
1. Setup the NIST test suite by following the instructions in the README.md file in the NIST_test_suite folder.
2. Compile the PRNG by running `g++ -o prng prng.cpp -lntl -lgmp -lm` in the root directory.
3. (Optional) Change the PRNG configuration in the `config.json` file.

## Running the NIST tests

Run the tests by running `python automation.py`.

The results will be saved in the `nist_results` folder with an hierarchy of the form `nist_results/<field_size>/<polynomial_h>/<polynomial_a>/<polynomial_c>/<seed_id>.txt`. where `<seed_id>` is the decimal representation of the seed when seen as a binary string.

## Running the PRNG only

Run the PRNG by running `./prng <h> <a> <c> <x_0> <iterations> [output_file] [--debug]`. Polynomials should be given in the form of a space separated string of degrees with coefficient one.


For example:
```bash
./prng "31 13 8 3 0" "4 0" "0" "31 29 28 25 23 22 21 19 17 14 13 11 10 9 7 5 0" 3225807 out.bin
```

you can crop the output to 100.000.000 bits by running the script `python cropper.py <input_file> [output_file]`.