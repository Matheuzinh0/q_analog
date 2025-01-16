#include <NTL/GF2X.h>
#include <NTL/GF2E.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <bitset>
using namespace std;
using namespace NTL;

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
    ostringstream oss;
    oss << element;
    return oss.str();
}

class BitBuffer {
private:
    vector<unsigned char> buffer;
    size_t currentBit;
    
public:
    BitBuffer() : currentBit(0) {
        buffer.push_back(0);
    }
    
    void addBit(bool bit) {
        size_t bytePos = currentBit / 8;
        size_t bitPos = 7 - (currentBit % 8);
        
        if (bytePos >= buffer.size()) {
            buffer.push_back(0);
        }
        
        if (bit) {
            buffer[bytePos] |= (1 << bitPos);
        }
        
        currentBit++;
    }
    
    const vector<unsigned char>& getBuffer() const {
        return buffer;
    }
    
    size_t getBitCount() const {
        return currentBit;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 6 || argc > 8) {
        cerr << "Usage: " << argv[0] << " <h> <a> <c> <x_0> <iterations> [output_file] [--debug]" << endl;
        return 1;
    }
    
    bool debug_mode = false;
    string output_filename = "prng_output.bin";  // default filename
    
    // Parse optional arguments
    for (int i = 6; i < argc; i++) {
        string arg(argv[i]);
        if (arg == "--debug") {
            debug_mode = true;
        } else {
            output_filename = arg;
        }
    }
    
    GF2X h = parsePolynomial(argv[1]);
    long fieldDegree = deg(h);  // Get the degree of the field polynomial
    GF2E::init(h);
    GF2E a = conv<GF2E>(parsePolynomial(argv[2]));
    GF2E c = conv<GF2E>(parsePolynomial(argv[3]));
    GF2E x = conv<GF2E>(parsePolynomial(argv[4]));
    long iterations = atol(argv[5]);
    
    if (debug_mode) {
        cout << "Field polynomial h(x): " << h << endl;
        cout << "Field degree: " << fieldDegree << endl;
        cout << "a: " << GF2EToString(a) << endl;
        cout << "c: " << GF2EToString(c) << endl;
        cout << "x_0: " << GF2EToString(x) << endl;
        cout << "Iterations: " << iterations << endl;
        cout << "Output file: " << output_filename << endl;
        cout << "-------------------------" << endl;
    }
    
    ofstream outfile(output_filename, ios::out | ios::binary);
    if (!outfile) {
        cerr << "Unable to open output file: " << output_filename << endl;
        return 1;
    }
    
    BitBuffer bitBuffer;
    
    for (long i = 0; i < iterations; i++) {
        x = a * x + c;
        
        if (debug_mode && i < 10) {
            cout << "Iteration " << i + 1 << ": x = " << GF2EToString(x) << endl;
        }
        
        GF2X xPoly = rep(x);
        
        // Always output fieldDegree bits
        for (long j = 0; j < fieldDegree; j++) {
            bitBuffer.addBit(IsOne(coeff(xPoly, j)));
        }
        
        // Write complete bytes to file when we have accumulated enough bits
        const vector<unsigned char>& buffer = bitBuffer.getBuffer();
        size_t completeBytes = (bitBuffer.getBitCount() / 8);
        
        if (completeBytes > 0) {
            outfile.write(reinterpret_cast<const char*>(buffer.data()), completeBytes);
            
            // Keep remaining bits and reset buffer
            size_t remainingBits = bitBuffer.getBitCount() % 8;
            BitBuffer newBuffer;
            if (remainingBits > 0) {
                unsigned char lastByte = buffer[completeBytes];
                for (size_t j = 0; j < remainingBits; j++) {
                    bool bit = (lastByte >> (7 - j)) & 1;
                    newBuffer.addBit(bit);
                }
            }
            bitBuffer = newBuffer;
        }
    }
    
    // Write any remaining bits in the final byte
    if (bitBuffer.getBitCount() > 0) {
        outfile.write(reinterpret_cast<const char*>(bitBuffer.getBuffer().data()), 1);
    }
    
    outfile.close();
    
    if (debug_mode) {
        cout << "-------------------------" << endl;
        cout << "PRNG sequence generated and written to " << output_filename << endl;
        cout << "Total bits written: " << iterations * fieldDegree << endl;
    }
    
    return 0;
}
