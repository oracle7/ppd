#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

double **mat_new(int n) {
    double **x = malloc(sizeof(double*) * n);
    x[0] = malloc(sizeof(double) * n * n);
    for (int i = 0; i < n; i++)
        x[i] = x[0] + i * n;
    return x;
}

void mat_del(double **x) {
    free(x[0]);
    free(x);
}

double *vec_new(int n) {
    return malloc(sizeof(double) * n);
}

void vec_del(double *v) {
    free(v);
}

void mat_pivot(double **A, double **P, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            P[i][j] = (i == j);

    for (int i = 0; i < n; i++) {
        int max_j = i;
        for (int j = i + 1; j < n; j++)
            if (fabs(A[j][i]) > fabs(A[max_j][i]))
                max_j = j;
        if (max_j != i) {
            for (int k = 0; k < n; k++) {
                double tmp = P[i][k];
                P[i][k] = P[max_j][k];
                P[max_j][k] = tmp;
            }
        }
    }
}

double **mat_mul(double **A, double **B, int n) {
    double **C = mat_new(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            C[i][j] = 0.0;
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
        }
    return C;
}

double *vec_mul(double **A, double *v, int n) {
    double *r = vec_new(n);
    for (int i = 0; i < n; i++) {
        r[i] = 0.0;
        for (int j = 0; j < n; j++)
            r[i] += A[i][j] * v[j];
    }
    return r;
}

void LU_decompor(double **A, double **L, double **U, double **P, int n) {
    mat_pivot(A, P, n);
    double **Aprime = mat_mul(P, A, n);

    for (int i = 0; i < n; i++)
        L[i][i] = 1.0;

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double s = 0.0;
            if (j <= i) {
                for (int k = 0; k < j; k++)
                    s += L[j][k] * U[k][i];
                U[j][i] = Aprime[j][i] - s;
            }
            if (j >= i) {
                for (int k = 0; k < i; k++)
                    s += L[j][k] * U[k][i];
                L[j][i] = (Aprime[j][i] - s) / U[i][i];
            }
        }

    mat_del(Aprime);
}

double *resolver_Ly(double **L, double *b, int n) {
    double *y = vec_new(n);
    for (int i = 0; i < n; i++) {
        double s = 0.0;
        for (int j = 0; j < i; j++)
            s += L[i][j] * y[j];
        y[i] = b[i] - s;
    }
    return y;
}

double *resolver_Ux(double **U, double *y, int n) {
    double *x = vec_new(n);
    for (int i = n - 1; i >= 0; i--) {
        double s = 0.0;
        for (int j = i + 1; j < n; j++)
            s += U[i][j] * x[j];
        x[i] = (y[i] - s) / U[i][i];
    }
    return x;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <entrada.bin> <saida_x.bin> <log.txt>\n", argv[0]);
        return 1;
    }

    const char *entrada = argv[1];
    const char *saida   = argv[2];
    const char *logfile = argv[3];

    FILE *fp = fopen(entrada, "rb");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir arquivo de entrada: %s\n", entrada);
        return 1;
    }

    int n;
    fread(&n, sizeof(int), 1, fp);

    float *Af = malloc(sizeof(float) * n * n);
    float *bf = malloc(sizeof(float) * n);
    fread(Af, sizeof(float), n * n, fp);
    fread(bf, sizeof(float), n, fp);
    fclose(fp);

    double **A = mat_new(n);
    double *b  = vec_new(n);
    for (int i = 0; i < n * n; i++)
        A[0][i] = (double)Af[i];
    for (int i = 0; i < n; i++)
        b[i] = (double)bf[i];
    free(Af);
    free(bf);

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double **L = mat_new(n), **U = mat_new(n), **P = mat_new(n);
    LU_decompor(A, L, U, P, n);

    double *Pb = vec_mul(P, b, n);
    double *y  = resolver_Ly(L, Pb, n);
    double *x  = resolver_Ux(U, y, n);

    clock_gettime(CLOCK_MONOTONIC, &t2);
    double tempo = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;

    FILE *fx = fopen(saida, "wb");
    if (fx) {
        fwrite(x, sizeof(double), n, fx);
        fclose(fx);
    }

    FILE *log = fopen(logfile, "w");
    if (log) {
        time_t agora = time(NULL);
        char *timestamp = ctime(&agora);
        timestamp[strlen(timestamp) - 1] = '\0';  // remove \n
        fprintf(log, "Arquivo de entrada: %s\n", entrada);
        fprintf(log, "Arquivo de saída  : %s\n", saida);
        fprintf(log, "Ordem da matriz   : %d\n", n);
        fprintf(log, "Tempo de execução : %.6f segundos\n", tempo);
        fprintf(log, "Timestamp         : %s\n", timestamp);
        fclose(log);
    }

    mat_del(A); mat_del(L); mat_del(U); mat_del(P);
    vec_del(b); vec_del(Pb); vec_del(y); vec_del(x);

    return 0;
}
