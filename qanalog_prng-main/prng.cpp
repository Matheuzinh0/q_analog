#include <NTL/GF2X.h>
#include <NTL/GF2E.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <deque>
using namespace std;
using namespace NTL;
extern "C" {
#include "TestU01.h"
#include "swrite.h"
}
bool debug_mode = false;

GF2X parsePolynomial(const string& s) {
    GF2X result;
    istringstream ss(s);
    long coeff;
    while (ss >> coeff) {
        SetCoeff(result, coeff);
    }
    return result;
}

string GF2EToString(const GF2E& element) {
    GF2X poly = rep(element);
    stringstream ss;
    ss << poly;
    return ss.str();
}

struct PRNGState {
    GF2E x, a, c;
    long fieldDegree;
    deque<bool> bit_buffer;
};

static PRNGState prng_state;

unsigned int prng_generator(void) {
    PRNGState& s = prng_state;
    while (s.bit_buffer.size() < 32) {
        s.x = s.a * s.x + s.c;
        GF2X xPoly = rep(s.x);
        for (long j = 0; j < s.fieldDegree; ++j) {
            bool bit = IsOne(coeff(xPoly, j));
            s.bit_buffer.push_back(bit);
        }
    }

    unsigned int output = 0;
    for (int i = 0; i < 32; ++i) {
        output <<= 1;
        output |= s.bit_buffer.front() ? 1 : 0;
        s.bit_buffer.pop_front();
    }
    return output;
}

double prng_generator_out(void) {
    unsigned int v = prng_generator();
    return v / (double)UINT32_MAX;
}

void run_test_battery(const char *test_name) {
    swrite_Basic = FALSE;
    unif01_Gen* gen = unif01_CreateExternGenBits(const_cast<char*> ("PRNG"), prng_generator);

    if (strcmp(test_name, "Rabbit") == 0) {
        bbattery_Rabbit(gen, 1 << 20);
    } else if (strcmp(test_name, "Alphabit") == 0) {
        bbattery_Alphabit(gen, 1 << 20, 0, 32);
    } else if (strcmp(test_name, "SmallCrush") == 0) {
        unif01_Gen* gen01 = unif01_CreateExternGen01(const_cast<char*> ("PRNG"), prng_generator_out);
        bbattery_SmallCrush(gen01);
        unif01_DeleteExternGen01(gen01);
    } else {
        cerr << "Teste não suportado: " << test_name << endl;
        exit(1);
    }

    unif01_DeleteExternGenBits(gen);
}

int main(int argc, char* argv[]) {
    if (argc < 6 || argc > 7) {
        cerr << "Uso: " << argv[0] << " <h> <a> <c> <x0> <teste> [--debug]" << endl;
        return 1;
    }

    bool debug_mode = false;
    if (argc == 7 && string(argv[6]) == "--debug") {
        debug_mode = true;
    }

    GF2X h = parsePolynomial(argv[1]);
    GF2E::init(h);

    prng_state.fieldDegree = deg(h);
    prng_state.a = conv<GF2E>(parsePolynomial(argv[2]));
    prng_state.c = conv<GF2E>(parsePolynomial(argv[3]));
    prng_state.x = conv<GF2E>(parsePolynomial(argv[4]));
    const char* test_name = argv[5];

    if (debug_mode) {
        cout << "Polinômio do campo h(x): " << h << endl;
        cout << "Grau do campo: " << prng_state.fieldDegree << endl;
        cout << "a: " << GF2EToString(prng_state.a) << endl;
        cout << "c: " << GF2EToString(prng_state.c) << endl;
        cout << "x0: " << GF2EToString(prng_state.x) << endl;
        cout << "Teste selecionado: " << test_name << endl;
        cout << "-------------------------" << endl;
    }

    run_test_battery(test_name);
    return 0;
}
