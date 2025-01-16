#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unif01.h>
#include <bbattery.h>
#include <ufile.h>
#include <stdlib.h>

#define MAX_VALUE 150
static double nb = pow(2, 24);
static int r = 0;
static int s = 32;

int main() {
    for (int i = 0; i < 30; ++i) {
        for (int j = 0; j < 4; ++j) {
            char filename[MAX_VALUE];        // Buffer para o nome do arquivo binário
            char output_txt[100];      // Buffer para o nome do arquivo de saída TXT
            char output_pdf[100];      // Buffer para o nome do arquivo de saída PDF
            char command[200];         // Buffer para o comando de conversão para PDF
          
            // Formatar os nomes dos arquivos
            snprintf(filename, sizeof(filename), "outputs/out_config%d_seed%d.bin", i, j);
            /*
            // Verificar se o arquivo existe (por exemplo, usando o nome do arquivo "NULL" como critério)
            if (strcmp(filename, "NULL") == 0) {
                j++;  // Avança o índice de j
                continue; // Pula para o próximo loop sem realizar o processamento
            }
*/
            snprintf(output_txt, sizeof(output_txt), "results_config%d_seed%d.txt", i, j);
            snprintf(output_pdf, sizeof(output_pdf), "results_config%d_seed%d.pdf", i, j);

            // Criação do gerador de números a partir do arquivo
            unif01_Gen *gen;
            gen = ufile_CreateReadBin(filename, 128);

            if (gen == NULL) {
                printf("Erro ao abrir o arquivo: %s\n", filename);
                continue;  // Pula para o próximo caso se o arquivo não for encontrado
            }

            // Inicialização (caso necessário)
            ufile_InitReadBin();

            // Redirecionar a saída para um arquivo TXT
            FILE *output = freopen(output_txt, "w", stdout);
            if (output == NULL) {
                printf("Erro ao criar o arquivo de saída: %s\n", output_txt);
                ufile_DeleteReadBin(gen);
                continue;
            }

            // Testes (descomente os testes que deseja realizar)
            bbattery_Rabbit(gen, nb);
            bbattery_Alphabit(gen, nb, r, s);
            // bbattery_FIPS_140_2(gen);

            // Finaliza o uso do gerador e do arquivo
            fclose(output);  // Fecha o arquivo TXT
            ufile_DeleteReadBin(gen);

            // Converte o arquivo TXT gerado para PDF
            snprintf(command, sizeof(command), "libreoffice --convert-to pdf %s --outdir .", output_txt);
            int conversion_result = system(command);

            if (conversion_result != 0) {
                printf("Erro ao converter %s para PDF\n", output_txt);
            } else {
                printf("Arquivo PDF gerado: %s\n", output_pdf);
            }
        }
    }
    return 0;
}

