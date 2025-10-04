#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <limits.h>

/*
 * gerador.c
 *
 * Geração de matrizes e vetores para testes de sistemas lineares Ax = b.
 * Este programa cria um arquivo binário contendo:
 *   - [int]     n         → ordem da matriz
 *   - [float]   A[n][n]   → matriz dos coeficientes
 *   - [float]   b[n]      → vetor dos termos independentes
 *
 * A matriz A é gerada com valores aleatórios ou com estrutura de diagonal dominante,
 * dependendo do parâmetro "tipo". A geração é paralelizada com OpenMP para acelerar
 * a escrita das linhas da matriz. O vetor b é gerado sequencialmente com valores suaves.
 *
 * Parâmetros:
 *   - ordem         → número de linhas/colunas da matriz (até INT_MAX)
 *   - arquivo_saida → nome do arquivo binário a ser gerado
 *   - tipo          → opcional: 0 (aleatória - padrão), 1 (diagonal dominante)
 *
 * Uso:
 *   ./gerador <ordem> <arquivo_saida> [tipo]
 *
 * Exemplo:
 *   ./gerador 1000 matriz.bin 1
 *
 * Observações:
 *   - O gerador assume ambiente controlado e não realiza validação numérica da matriz.
 *   - O arquivo gerado é compatível com os programas resolve_matriz_sequencial.c e resolve_matriz.c.
 */


float gerar_valor(int i, int j, int tipo, int n) {
    if (tipo == 1) {
        if (i == j) return n * 10.0f;  // força diagonal dominante
        return (float)(rand_r(&i) % 10 - 5);
    } else {
        return ((float)rand_r(&i) / RAND_MAX) * 20.0f - 10.0f;
    }
}

float gerar_b(int i) {
    return ((float)(i % 1000) / 1000.0f);  // valor suave e reprodutível
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <ordem> <arquivo_saida> [tipo]\n", argv[0]);
        fprintf(stderr, "  tipo = 0 (aleatória - padrão), 1 (diagonal dominante)\n");
        return 1;
    }

    long long n = atoll(argv[1]);
    if (n <= 0 || n > INT_MAX) {
        fprintf(stderr, "Ordem inválida: %lld. Máximo suportado: %d\n", n, INT_MAX);
        return 1;
    }

    char *saida = argv[2];
    int tipo = (argc >= 4) ? atoi(argv[3]) : 0;

    // Escreve o cabeçalho com ordem da matriz
    FILE *fp = fopen(saida, "wb");
    if (!fp) {
        perror("Erro ao criar arquivo de saída");
        return 1;
    }
    int n32 = (int)n;
    fwrite(&n32, sizeof(int), 1, fp);
    fclose(fp);

    // Geração paralela da matriz A
    #pragma omp parallel
    {
        FILE *fp_local = fopen(saida, "r+b");
        if (!fp_local) {
            perror("Erro ao abrir arquivo para escrita paralela");
            exit(1);
        }

        #pragma omp for schedule(static)
        for (long long i = 0; i < n; i++) {
            float *linha = malloc(n * sizeof(float));
            if (!linha) {
                fprintf(stderr, "Erro de alocação na linha %lld\n", i);
                exit(1);
            }

            for (long long j = 0; j < n; j++) {
                linha[j] = gerar_valor((int)i, (int)j, tipo, (int)n);
            }

            long long offset = sizeof(int) + i * n * sizeof(float);
            fseek(fp_local, offset, SEEK_SET);
            fwrite(linha, sizeof(float), n, fp_local);

            free(linha);
        }

        fclose(fp_local);
    }

    // Escrita sequencial do vetor b
    fp = fopen(saida, "ab");
    if (!fp) {
        perror("Erro ao reabrir arquivo para escrever vetor b");
        return 1;
    }
    for (long long i = 0; i < n; i++) {
        float bi = gerar_b((int)i);
        fwrite(&bi, sizeof(float), 1, fp);
    }
    fclose(fp);

    printf("Arquivo %s gerado com ordem %lld (%s)\n",
           saida, n, tipo ? "diagonal dominante" : "aleatória");

    return 0;
}
