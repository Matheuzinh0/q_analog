#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unif01.h>
#include <bbattery.h>
#include <ufile.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_VALUE 31
#define FILENAME_SIZE 2048
#define OUTPUT_TXT_SIZE 256
#define OUTPUT_PDF_SIZE 256
#define COMMAND_SIZE 512
/*
 * Nome do Autor: Matheus Sousa
 * E-mail de Contato: matheus.henriquesousa@ufpe.br
 * Ano: 2024
 * Versão: v1.0
 * Descrição: O programa faz a leitura dos arquivos contidos em outputs( arquivos binários gerados por um PRNG ) e reatilza o teste selecionado
   pelo usuário. Os resultados são gerados em PDF e armazenados no diretório com o nome do teste.
   Nesta versão inicial o libre-office está sendo usado para gerar o documento em PDF.
   Comando: gcc teste.c -o teste -Iinclude -Ilib -ltestu01 -lmylib -lprobdist -lm && ./teste
 */
int elev;
double nb;
static int r = 0;
static int s = 32;
int num;
void autor(){
printf("\t ______________________________________\t");
    printf("\n\t|Autor: Matheus Sousa                 |\t \n");
    printf("\t|E-mail: matheus.henriquesousa@ufpe.br|\t \n");
    printf("\t|Ano: 2024                            |\t \n");
    printf("\t|Versão: v1.0                         |\n");
    printf("\t---------------------------------------\n");
    printf("\t---------------\t\n");
    printf("\t|TESTU01 1.2.3|\t \n");
    printf("\t---------------\t\n");
}
void remove_file_if_exists(const char *filename) {
    if (remove(filename) != 0) {
        perror("Erro ao excluir o arquivo");
    } else {
        printf("Arquivo removido: %s\n", filename);
    }
}

void create_directory(const char *dir_name) {
    struct stat st = {0};
    if (stat(dir_name, &st) == -1) {
        if (mkdir(dir_name, 0700) != 0) {
            perror("Erro ao criar o diretório");
            exit(1);  // Se falhar em criar o diretório, o programa termina
        }
        printf("Diretório criado: %s\n", dir_name);
    }
}

int main() {
autor();
    printf("\t Qual teste você desejar fazer?\t\n");
    printf("\t 0 - Rabbit \n \t 1 - Alphabit \n \t 2 - BlockAlphabit \n \t 3 - FIPS140_2 \n \t 4 - PseudoDiehard \n \t 5 - Sair \n");
    scanf("%d", &num);
    num!=5 ? printf("Number bits (2^{n})\n") : exit(0);
    scanf("%d", &elev);
    nb = 1 << elev;
    // Nome do teste baseado na escolha do usuário
    char *test_name;
    switch (num) {
        case 0:
            test_name = "Rabbit";
            break;
        case 1:
            test_name = "Alphabit";
            break;
        case 2:
            test_name = "BlockAlphabit";
            break;
        case 3:
            test_name = "FIPS140_2";
            break;
        case 4:
            test_name = "PseudoDiehard";
            break;
        case 5:
            return 0;
        default:
            printf("Opção inválida.\n");
            return 1;
    }

    // Criar o diretório para o teste escolhido
    create_directory(test_name);

    // Loop para as configurações e seeds
    for (int i = 0; i < MAX_VALUE; ++i) {
        for (int j = 0; j < 5; ++j) {
            // Alocação dinâmica para os buffers
            char *filename = (char *)malloc(FILENAME_SIZE * sizeof(char));
            char *output_txt = (char *)malloc(OUTPUT_TXT_SIZE * sizeof(char));
            char *output_pdf = (char *)malloc(OUTPUT_PDF_SIZE * sizeof(char));
            char *command = (char *)malloc(COMMAND_SIZE * sizeof(char));

            if (!filename || !output_txt || !output_pdf || !command) {
                printf("Erro ao alocar memória para buffers. Encerrando o programa.\n");
                return 1;  // Falha na alocação de memória
            }

            // Formatar os nomes dos arquivos, incluindo o diretório do teste
            snprintf(filename, FILENAME_SIZE, "/home/matheus/Área de Trabalho/qanalog_prng-main/qanalog_prng-main/outputs/out_config%d_seed%d.bin", i, j);
            snprintf(output_txt, OUTPUT_TXT_SIZE, "%s/results_config%d_seed%d_%s.txt", test_name, i, j, test_name);
            snprintf(output_pdf, OUTPUT_PDF_SIZE, "%s/results_config%d_seed%d_%s.pdf", test_name, i, j, test_name);

            // Verificar se o arquivo existe antes de tentar abrir
            FILE *test_file = fopen(filename, "rb");
            if (!test_file) {
                printf("Erro: O arquivo %s não foi encontrado ou não pode ser aberto.\n", filename);
                free(filename);
                free(output_txt);
                free(output_pdf);
                free(command);
                continue;  // Avançar para a próxima iteração se o arquivo não for encontrado
            }
            fclose(test_file);  // Arquivo pode ser fechado após a verificação

            // Criar o gerador de números a partir do arquivo
            unif01_Gen *gen = ufile_CreateReadBin(filename, 65);
            if (gen == NULL) {
                printf("Erro ao abrir o arquivo: %s. Verifique se o arquivo está no formato correto.\n", filename);
                free(filename);
                free(output_txt);
                free(output_pdf);
                free(command);
                continue;  // Avançar para a próxima iteração
            }

            // Inicialização necessária do gerador
            ufile_InitReadBin();

            // Redirecionar a saída para um arquivo TXT
            FILE *output = freopen(output_txt, "w", stdout);
            if (output == NULL) {
                printf("Erro ao criar o arquivo de saída: %s. Verifique permissões de escrita.\n", output_txt);
                perror("Detalhes do erro");
                ufile_DeleteReadBin(gen);
                free(filename);
                free(output_txt);
                free(output_pdf);
                free(command);
                continue;  // Avançar para a próxima iteração
            }

            // Imprimir mensagem de depuração para verificar a iteração
            printf("Processando config %d, seed %d, teste: %s\n", i, j, test_name);

            // Chamar a função do teste selecionado
            switch(num) {
                case 0:
                    bbattery_Rabbit(gen, nb);  // Teste Rabbit
                    break;
                case 1:
                    bbattery_Alphabit(gen, nb, r, s);  // Teste Alphabit
                    break;
                case 2:
                    bbattery_BlockAlphabit(gen, nb, r, s);  // Teste BlockAlphabit
                    break;
                case 3:
                    bbattery_FIPS_140_2(gen);  // Teste FIPS 140-2
                    break;
                case 4:
                    bbattery_pseudoDIEHARD(gen);
                    break;
                default:
                    break;
            }

            // Finaliza o uso do gerador e do arquivo
            fclose(output);  // Fecha o arquivo TXT
            ufile_DeleteReadBin(gen);

            // Verifica a presença do LibreOffice e tenta converter o arquivo para PDF
            snprintf(command, COMMAND_SIZE, "command -v libreoffice > /dev/null 2>&1");
            int check_libreoffice = system(command);
            if (check_libreoffice != 0) {
                printf("Erro: LibreOffice não está instalado ou não é encontrado no PATH.\n");
                free(filename);
                free(output_txt);
                free(output_pdf);
                free(command);
                continue;  // Avançar para a próxima iteração
            }

            // Comando de conversão para PDF
            snprintf(command, COMMAND_SIZE, "libreoffice --headless --convert-to pdf %s --outdir %s", output_txt, test_name);
            printf("Comando de conversão: %s\n", command);
            int conversion_result = system(command);
            if (conversion_result != 0) {
                printf("Erro ao converter %s para PDF. Verifique se o LibreOffice foi instalado corretamente.\n", output_txt);
            } else {
                printf("Arquivo PDF gerado com sucesso: %s\n", output_pdf);
                // Após a conversão para PDF, excluir o arquivo .txt
                remove_file_if_exists(output_txt);
            }

            // Liberação de memória
            free(filename);
            free(output_txt);
            free(output_pdf);
            free(command);
        }
    }

    return 0;
}

