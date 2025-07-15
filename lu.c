#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

double *vec_new(int n) {
    return malloc(sizeof(double) * n);
}

void vec_del(double *v) {
    free(v);
}

double vec_l2(double *Ax, double *b, int n) {
    double s = 0.0;
    for (int i = 0; i < n; i++)
        s += (Ax[i] - b[i]) * (Ax[i] - b[i]);
    return sqrt(s);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    if (argc != 4) {
        fprintf(stderr,
            "Erro: parâmetros insuficientes.\n"
            "Uso correto: %s <matriz.bin> <saida_x.bin> <log.txt>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const char *entrada = argv[1];
    const char *saida_x = argv[2];
    const char *log_txt = argv[3];

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n;
    float *Af = NULL, *bf = NULL;
    double *local_A, *local_b;
    int rows;

    if (rank == 0) {
        FILE *fp = fopen(entrada, "rb");
        if (!fp) {
            fprintf(stderr, "Erro ao abrir matriz de entrada: %s\n", entrada);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fread(&n, sizeof(int), 1, fp);
        Af = malloc(sizeof(float) * n * n);
        bf = malloc(sizeof(float) * n);
        fread(Af, sizeof(float), n * n, fp);
        fread(bf, sizeof(float), n, fp);
        fclose(fp);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    rows = n / size;

    local_A = malloc(sizeof(double) * rows * n);
    local_b = malloc(sizeof(double) * rows);
    double *x = vec_new(n);

    if (rank == 0) {
        for (int r = 1; r < size; r++) {
            MPI_Send(&Af[r * rows * n], rows * n, MPI_FLOAT, r, 0, MPI_COMM_WORLD);
            MPI_Send(&bf[r * rows], rows, MPI_FLOAT, r, 0, MPI_COMM_WORLD);
        }
        for (int i = 0; i < rows * n; i++)
            local_A[i] = (double)Af[i];
        for (int i = 0; i < rows; i++)
            local_b[i] = (double)bf[i];
        free(Af); free(bf);
    } else {
        float *tmpA = malloc(sizeof(float) * rows * n);
        float *tmpb = malloc(sizeof(float) * rows);
        MPI_Recv(tmpA, rows * n, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(tmpb, rows, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < rows * n; i++)
            local_A[i] = (double)tmpA[i];
        for (int i = 0; i < rows; i++)
            local_b[i] = (double)tmpb[i];
        free(tmpA); free(tmpb);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    // 🧠 Fatoração LU e substituição (versão simplificada para contexto)
    // Aqui entra sua lógica paralela de LU distribuída
    // Após cálculo, todos os ranks possuem vetor x completo

    MPI_Barrier(MPI_COMM_WORLD);
    double t2 = MPI_Wtime();

    if (rank == 0) {
        FILE *fx = fopen(saida_x, "wb");
        if (fx) {
            fwrite(x, sizeof(double), n, fx);
            fclose(fx);
        } else {
            fprintf(stderr, "Erro ao salvar solução: %s\n", saida_x);
        }

        // Validação ||Ax - b||
        double *Ax = vec_new(n);
        for (int i = 0; i < n; i++) Ax[i] = 0.0;
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < n; j++)
                Ax[i] += local_A[i * n + j] * x[j];

        double local_error = vec_l2(Ax, local_b, rows);
        double global_error = 0.0;
        MPI_Reduce(&local_error, &global_error, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        FILE *log = fopen(log_txt, "w");
        if (log) {
            time_t now = time(NULL);
            char *ts = ctime(&now);
            ts[strlen(ts) - 1] = '\0';
            fprintf(log, "Arquivo de entrada: %s\n", entrada);
            fprintf(log, "Arquivo de saída  : %s\n", saida_x);
            fprintf(log, "Ordem da matriz   : %d\n", n);
            fprintf(log, "Tempo total       : %.6f segundos\n", t2 - t1);
            fprintf(log, "Norma L2 do erro  : %.6e\n", global_error);
            fprintf(log, "Timestamp         : %s\n", ts);
            fclose(log);
        }

        vec_del(Ax);
    }

    vec_del(x);
    free(local_A); free(local_b);

    MPI_Finalize();
    return 0;
}
