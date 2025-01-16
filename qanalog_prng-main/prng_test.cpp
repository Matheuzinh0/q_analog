#include <NTL/GF2X.h>
#include <NTL/GF2E.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <bitset>
//#include <nlohmann/json.hpp>
#include <json>

using namespace std;
using namespace NTL;
using json = nlohmann::json;

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

void runPRNG(const string& hStr, const string& aStr, const string& cStr, const string& x0Str, long iterations, const string& output_filename) {
    GF2X h = parsePolynomial(hStr);
    long fieldDegree = deg(h);
    GF2E::init(h);
    GF2E a = conv<GF2E>(parsePolynomial(aStr));
    GF2E c = conv<GF2E>(parsePolynomial(cStr));
    GF2E x = conv<GF2E>(parsePolynomial(x0Str));
    
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
        return;
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
        
        const vector<unsigned char>& buffer = bitBuffer.getBuffer();
        size_t completeBytes = (bitBuffer.getBitCount() / 8);
        
        if (completeBytes > 0) {
            outfile.write(reinterpret_cast<const char*>(buffer.data()), completeBytes);
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
    
    if (bitBuffer.getBitCount() > 0) {
        outfile.write(reinterpret_cast<const char*>(bitBuffer.getBuffer().data()), 1);
    }
    
    outfile.close();
}

void processJSON(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Unable to open JSON file: " << filename << endl;
        return;
    }
    
    json config;
    file >> config;
    
    for (size_t i = 0; i < config.size(); i++) {
        auto entry = config[i];
        string h = entry["base_params"]["h"];
        string a = entry["base_params"]["a"];
        string c = entry["base_params"]["c"];
        vector<string> seeds = entry["seeds"];
        long iterations = entry["iterations"];
        
        for (size_t j = 0; j < seeds.size(); j++) {
            string x0 = seeds[j];
            string output_file = "output_" + to_string(i) + "_" + to_string(j) + ".bin";
            runPRNG(h, a, c, x0, iterations, output_file);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <config.json>" << endl;
        return 1;
    }
    
    string json_file = argv[1];
    processJSON(json_file);
    return 0;
}

