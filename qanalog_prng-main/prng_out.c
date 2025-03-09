#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <flint/fmpz.h>
#include <flint/flint.h>
#include <flint/fq.h>
#include <flint/fmpz_mod.h>
#include <flint/fmpz_mod_poly.h>

typedef struct {
    unsigned char* buffer;
    size_t buffer_size;
    size_t current_bit;
} BitBuffer;

void bit_buffer_init(BitBuffer* bb) {
    bb->buffer = (unsigned char*)malloc(1);
    bb->buffer[0] = 0;
    bb->buffer_size = 1;
    bb->current_bit = 0;
}

void bit_buffer_add_bit(BitBuffer* bb, int bit) {
    size_t byte_pos = bb->current_bit / 8;
    size_t bit_pos = 7 - (bb->current_bit % 8);

    if (byte_pos >= bb->buffer_size) {
        bb->buffer_size++;
        bb->buffer = (unsigned char*)realloc(bb->buffer, bb->buffer_size);
        bb->buffer[byte_pos] = 0;
    }

    if (bit) bb->buffer[byte_pos] |= (1 << bit_pos);
    bb->current_bit++;
}

size_t bit_buffer_get_bit_count(const BitBuffer* bb) {
    return bb->current_bit;
}

void bit_buffer_clear(BitBuffer* bb) {
    free(bb->buffer);
    bb->buffer = NULL;
    bb->buffer_size = 0;
    bb->current_bit = 0;
}

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

int main(int argc, char** argv) {
    if (argc < 6 || argc > 8) {
        fprintf(stderr, "Uso: %s <h> <a> <c> <x0> <iter> [saida] [--debug]\n", argv[0]);
        return 1;
    }

    // Configuração inicial
    int debug = 0;
    const char* out_file = "prng_out.bin";
    for (int i = 6; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) debug = 1;
        else out_file = argv[i];
    }

    // Inicializa GF(2)
    fmpz_t p;
    fmpz_init(p);
    fmpz_set_ui(p, 2);
    fmpz_mod_ctx_t mod_ctx;
    fmpz_mod_ctx_init(mod_ctx, p);

    // Carrega polinômio h(x)
    fmpz_mod_poly_t h;
    fmpz_mod_poly_init(h, mod_ctx);
    parse_polynomial(h, argv[1], mod_ctx);

    // Cria corpo GF(2^m)
    fq_ctx_t field_ctx;
    fq_ctx_init_modulus(field_ctx, h, mod_ctx, "x");
    long degree = fq_ctx_degree(field_ctx);

    // Parse a, c, x0
    fmpz_mod_poly_t a_poly, c_poly, x0_poly;
    fmpz_mod_poly_init(a_poly, mod_ctx);
    fmpz_mod_poly_init(c_poly, mod_ctx);
    fmpz_mod_poly_init(x0_poly, mod_ctx);
    parse_polynomial(a_poly, argv[2], mod_ctx);
    parse_polynomial(c_poly, argv[3], mod_ctx);
    parse_polynomial(x0_poly, argv[4], mod_ctx);

    // Converte para elementos do corpo
    fq_t a, c, x;
    fq_init(a, field_ctx);
    fq_init(c, field_ctx);
    fq_init(x, field_ctx);
    fq_set_fmpz_mod_poly(a, a_poly, field_ctx);
    fq_set_fmpz_mod_poly(c, c_poly, field_ctx);
    fq_set_fmpz_mod_poly(x, x0_poly, field_ctx);

    long iterations = atol(argv[5]);

    // Debug
    if (debug) {
        printf("h(x) = "); fmpz_mod_poly_print_pretty(h, "x", mod_ctx); printf("\n");
        printf("Grau: %ld\n", degree);
        printf("a = "); fq_print_pretty(a, field_ctx); printf("\n");
        printf("c = "); fq_print_pretty(c, field_ctx); printf("\n");
        printf("x0 = "); fq_print_pretty(x, field_ctx); printf("\n");
        printf("Iterações: %ld\n", iterations);
        printf("Saída: %s\n", out_file);
        printf("----------------------------\n");
    }

    // Arquivo de saída
    FILE* fout = fopen(out_file, "wb");
    if (!fout) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    BitBuffer bb;
    bit_buffer_init(&bb);
    fq_t tmp;
    fq_init(tmp, field_ctx);
    fmpz_mod_poly_t x_poly;
    fmpz_mod_poly_init(x_poly, mod_ctx);

    for (long i = 0; i < iterations; i++) {
        // x = a*x + c
        fq_mul(tmp, a, x, field_ctx);
        fq_add(tmp, tmp, c, field_ctx);
        fq_swap(x, tmp, field_ctx);

        // Debug (primeiras 10 iterações)
        if (debug && i < 10) {
            printf("x_%ld = ", i+1);
            fq_print_pretty(x, field_ctx);
            printf("\n");
        }

        // Extrai bits
        fq_get_fmpz_mod_poly(x_poly, x, field_ctx);
        for (long j = 0; j < degree; j++) {
            fmpz_t coeff;
            fmpz_init(coeff);
            fmpz_mod_poly_get_coeff_fmpz(coeff, x_poly, j, mod_ctx);
            int bit = fmpz_get_ui(coeff);
            bit_buffer_add_bit(&bb, bit);
            fmpz_clear(coeff);
        }

        // Escreve bytes completos
        size_t bytes = bb.current_bit / 8;
        if (bytes > 0) {
            fwrite(bb.buffer, 1, bytes, fout);
            size_t remaining = bb.current_bit % 8;
            BitBuffer new_bb;
            bit_buffer_init(&new_bb);
            if (remaining > 0) {
                unsigned char last = bb.buffer[bytes];
                for (size_t j = 0; j < remaining; j++)
                    bit_buffer_add_bit(&new_bb, (last >> (7 - j)) & 1);
            }
            bit_buffer_clear(&bb);
            bb = new_bb;
        }
    }

    // Escreve bits restantes
    if (bb.current_bit > 0) fwrite(bb.buffer, 1, 1, fout);
    fclose(fout);
    bit_buffer_clear(&bb);

    // Limpeza
    fmpz_mod_poly_clear(x_poly, mod_ctx);
    fq_clear(tmp, field_ctx);
    fq_clear(a, field_ctx);
    fq_clear(c, field_ctx);
    fq_clear(x, field_ctx);
    fmpz_mod_poly_clear(h, mod_ctx);
    fmpz_mod_poly_clear(a_poly, mod_ctx);
    fmpz_mod_poly_clear(c_poly, mod_ctx);
    fmpz_mod_poly_clear(x0_poly, mod_ctx);
    fq_ctx_clear(field_ctx);
    fmpz_mod_ctx_clear(mod_ctx);
    fmpz_clear(p);

    if (debug) printf("Concluído. %ld bits escritos.\n", iterations * degree);
    return 0;
}
