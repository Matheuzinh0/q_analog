#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <flint/fmpz.h>
#include <flint/flint.h>
#include <flint/fq.h>
#include <flint/fmpz_mod.h>
#include <flint/fmpz_mod_poly.h>
#include "TestU01.h"
#include "swrite.h"
typedef struct {
    fq_ctx_t field_ctx;
    fmpz_mod_ctx_t mod_ctx;
    fq_t a, c, x;
    fmpz_mod_poly_t x_poly;
    
    // Buffers para diferentes tamanhos de polinômios
    union {
        struct {
            uint64_t bit_buffer;
            int bits_available;
        } small;
        
        struct {
            uint32_t *buffer;
            size_t buffer_size;
            size_t buffer_pos;
        } large;
    };
    
    long degree;
    char mode; // 'S' para pequenos (<128), 'L' para grandes
} PRNGState;

static PRNGState *current_state = NULL;

// ================== HELPER FUNCTIONS ==================
void parse_polynomial(fmpz_mod_poly_t poly, const char* s, const fmpz_mod_ctx_t ctx) {
    char* str = strdup(s);
    char* token;
    char* rest = str;

    fmpz_mod_poly_zero(poly, ctx);
    while ((token = strtok_r(rest, " ", &rest))) {
        long coeff = atol(token);
        fmpz_mod_poly_set_coeff_ui(poly, coeff, 1, ctx);
    }
    free(str);
}

void generate_large_buffer() {
    if (!current_state || current_state->mode != 'L') return;

    // Executar uma iteração completa do PRNG
    fq_t tmp;
    fq_init(tmp, current_state->field_ctx);
    fq_mul(tmp, current_state->a, current_state->x, current_state->field_ctx);
    fq_add(tmp, tmp, current_state->c, current_state->field_ctx);
    fq_swap(current_state->x, tmp, current_state->field_ctx);
    fq_clear(tmp, current_state->field_ctx);

    // Extrair todos os bits do estado atual
    fq_get_fmpz_mod_poly(current_state->x_poly, current_state->x, current_state->field_ctx);
    
    // Calcular tamanho necessário
    const size_t bits_needed = current_state->degree;
    const size_t words_needed = (bits_needed + 31) / 32;
    
    // Realocar buffer se necessário
    if (current_state->large.buffer_size < words_needed) {
        free(current_state->large.buffer);
        current_state->large.buffer = malloc(words_needed * sizeof(uint32_t));
        current_state->large.buffer_size = words_needed;
    }
    
    memset(current_state->large.buffer, 0, words_needed * sizeof(uint32_t));
    
    // Preencher buffer (MSB-first)
    for (long j = 0; j < current_state->degree; j++) {
        fmpz_t coeff;
        fmpz_init(coeff);
        fmpz_mod_poly_get_coeff_fmpz(coeff, current_state->x_poly, j, current_state->mod_ctx);
        
        if (fmpz_get_ui(coeff)) {
            const size_t bit_pos = j;
            current_state->large.buffer[bit_pos/32] |= (1U << (31 - (bit_pos % 32)));
        }
        fmpz_clear(coeff);
    }
    current_state->large.buffer_pos = 0;
}

// ================== CORE GENERATOR ==================
unsigned int prng_generator(void) {
    if (!current_state) return 0;

    if (current_state->mode == 'L') {
        if (current_state->large.buffer_pos >= current_state->large.buffer_size) {
            generate_large_buffer();
        }
        return current_state->large.buffer[current_state->large.buffer_pos++];
    }
    else {
        // Modo para polinômios pequenos
        while (current_state->small.bits_available < 32) {
            fq_t tmp;
            fq_init(tmp, current_state->field_ctx);
            fq_mul(tmp, current_state->a, current_state->x, current_state->field_ctx);
            fq_add(tmp, tmp, current_state->c, current_state->field_ctx);
            fq_swap(current_state->x, tmp, current_state->field_ctx);
            fq_clear(tmp, current_state->field_ctx);

            fq_get_fmpz_mod_poly(current_state->x_poly, current_state->x, current_state->field_ctx);
            
            for (long j = 0; j < current_state->degree; j++) {
                fmpz_t coeff;
                fmpz_init(coeff);
                fmpz_mod_poly_get_coeff_fmpz(coeff, current_state->x_poly, j, current_state->mod_ctx);
                
                current_state->small.bit_buffer = (current_state->small.bit_buffer << 1) | fmpz_get_ui(coeff);
                current_state->small.bits_available++;
                fmpz_clear(coeff);
            }
        }

        unsigned int output = (current_state->small.bit_buffer >> (current_state->small.bits_available - 32)) & 0xFFFFFFFF;
        current_state->small.bits_available -= 32;
        current_state->small.bit_buffer &= ((1ULL << current_state->small.bits_available) - 1);
        
        return output;
    }
}
double prng_generator_out(void) {
    unsigned int v = prng_generator();  // Obtém o próximo valor do gerador
    return (double)v / (double)UINT32_MAX;  // Normaliza para o intervalo [0, 1)
}

// ================== LIFECYCLE MANAGEMENT ==================
void cleanup_prng() {
    if (current_state) {
        fq_clear(current_state->a, current_state->field_ctx);
        fq_clear(current_state->c, current_state->field_ctx);
        fq_clear(current_state->x, current_state->field_ctx);
        fmpz_mod_poly_clear(current_state->x_poly, current_state->mod_ctx);
        fq_ctx_clear(current_state->field_ctx);
        fmpz_mod_ctx_clear(current_state->mod_ctx);
        if (current_state->mode == 'L') free(current_state->large.buffer);
        free(current_state);
        current_state = NULL;
    }
}

void initialize_prng(const char *h_str, const char *a_str,
                   const char *c_str, const char *seed_str) {
    cleanup_prng();
    
    current_state = malloc(sizeof(PRNGState));
    memset(current_state, 0, sizeof(PRNGState));

    // Inicializar GF(2)
    fmpz_t p;
    fmpz_init(p);
    fmpz_set_ui(p, 2);
    fmpz_mod_ctx_init(current_state->mod_ctx, p);
    fmpz_clear(p);

    // Carregar polinômios
    fmpz_mod_poly_t h, a, c, x0;
    fmpz_mod_poly_init(h, current_state->mod_ctx);
    fmpz_mod_poly_init(a, current_state->mod_ctx);
    fmpz_mod_poly_init(c, current_state->mod_ctx);
    fmpz_mod_poly_init(x0, current_state->mod_ctx);
    
    parse_polynomial(h, h_str, current_state->mod_ctx);
    parse_polynomial(a, a_str, current_state->mod_ctx);
    parse_polynomial(c, c_str, current_state->mod_ctx);
    parse_polynomial(x0, seed_str, current_state->mod_ctx);

    // Criar corpo GF(2^m)
    fq_ctx_init_modulus(current_state->field_ctx, h, current_state->mod_ctx, "x");
    current_state->degree = fq_ctx_degree(current_state->field_ctx);

    // Configurar modo de operação
    current_state->mode = (current_state->degree > 64) ? 'L' : 'S';

    // Inicializar elementos do corpo
    fq_init(current_state->a, current_state->field_ctx);
    fq_init(current_state->c, current_state->field_ctx);
    fq_init(current_state->x, current_state->field_ctx);
    
    fq_set_fmpz_mod_poly(current_state->a, a, current_state->field_ctx);
    fq_set_fmpz_mod_poly(current_state->c, c, current_state->field_ctx);
    fq_set_fmpz_mod_poly(current_state->x, x0, current_state->field_ctx);

    fmpz_mod_poly_init(current_state->x_poly, current_state->mod_ctx);

    // Pré-geração inicial
    if (current_state->mode == 'L') {
        generate_large_buffer();
    }
}

// ================== TEST INTERFACE ==================
void run_test_battery(const char *test_name, double nb) {
    swrite_Basic = FALSE;
    unif01_Gen* gen = unif01_CreateExternGenBits("PRNG", prng_generator);
    
    if (strcmp(test_name, "Rabbit") == 0) {
        bbattery_Rabbit(gen, nb);
    } else if (strcmp(test_name, "Alphabit") == 0) {
        bbattery_Alphabit(gen, nb, 0, 32);
    }
    else if (strcmp(test_name, "SmallCrush") == 0) {
        unif01_Gen* gen = unif01_CreateExternGen01("PRNG", prng_generator_out);
        bbattery_SmallCrush(gen);
        unif01_DeleteExternGen01(gen);
    } else {
        fprintf(stderr, "Teste não suportado: %s\n", test_name);
    }
    
    unif01_DeleteExternGenBits(gen);
}

int main(int argc, char** argv) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <h> <a> <c> <seed> <test_name>\n", argv[0]);
        return 1;
    }

    initialize_prng(argv[1], argv[2], argv[3], argv[4]);
    double nb = 1 << 20;
    
    run_test_battery(argv[5], nb);
    cleanup_prng();
    
    return 0;
}
