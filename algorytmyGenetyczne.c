#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define N 9
#define BOXSIZE 3

#define POP_SIZE      400
#define MAX_GEN       5000
#define TOURNAMENT_K  3
#define MUT_RATE      0.06f
#define ELITE_KEEP    4
#define SHOW_EVERY    10

typedef struct {
    int grid[N][N];
    int fitness;
} Individual;

static bool fixed[N][N];

/* Sprawdza, czy liczba num nie wystêpuje w danym kwadracie 3x3 */
static bool unUsedInBox(int g[N][N], int rowStart, int colStart, int num) {
    for (int i = 0; i < BOXSIZE; ++i)
        for (int j = 0; j < BOXSIZE; ++j)
            if (g[rowStart + i][colStart + j] == num) return false;
    return true;
}

/* Wype³nia losowymi, niepowtarzaj¹cymi siê liczbami kwadrat 3x3 w siatce Sudoku */
static void fillBox(int g[N][N], int row, int col) {
    for (int i = 0; i < BOXSIZE; ++i)
        for (int j = 0; j < BOXSIZE; ++j) {
            int num;
            do { num = rand() % N + 1; } while (!unUsedInBox(g, row, col, num));
            g[row + i][col + j] = num;
        }
}

/* Sprawdza, czy mo¿na bezpiecznie wstawiæ liczbê num do komórki (r, c) */
static bool checkIfSafe(int g[N][N], int r, int c, int num) {
    for (int k = 0; k < N; ++k)
        if (g[r][k] == num || g[k][c] == num) return false;
    return unUsedInBox(g, r - r % BOXSIZE, c - c % BOXSIZE, num);
}

/* Wype³nia przek¹tne kwadratami 3x3 w siatce Sudoku */
static void fillDiagonal(int g[N][N]) {
    for (int i = 0; i < N; i += BOXSIZE) fillBox(g, i, i);
}

/* Rekurencyjnie wype³nia pozosta³e komórki siatki Sudoku */
static bool fillRemaining(int g[N][N], int r, int c) {
    if (r == N) return true;
    if (c == N) return fillRemaining(g, r + 1, 0);
    if (g[r][c] != 0) return fillRemaining(g, r, c + 1);

    for (int num = 1; num <= N; ++num) {
        if (checkIfSafe(g, r, c, num)) {
            g[r][c] = num;
            if (fillRemaining(g, r, c + 1)) return true;
            g[r][c] = 0;
        }
    }
    return false;
}

/* Usuwa k losowych cyfr z siatki Sudoku */
static void removeKDigits(int g[N][N], int k) {
    while (k) {
        int r = rand() % N, c = rand() % N;
        if (g[r][c]) { g[r][c] = 0; --k; }
    }
}

/* Generuje now¹ ³amig³ówkê Sudoku z k pustymi polami */
static void generateSudoku(int g[N][N], int k) {
    memset(g, 0, sizeof(int) * N * N);
    fillDiagonal(g);
    fillRemaining(g, 0, 0);
    removeKDigits(g, k);
}

/* Wyœwietla siatkê Sudoku na konsoli */
static void printGrid(const int g[N][N]) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j)
            printf(j % 3 ? "%d " : " %d ", g[i][j]);
        puts(i % 3 == 2 ? "\n" : "");
    }
}

/* Zlicza liczbê duplikatów w tablicy d³ugoœci N */
static int duplicatesInArray(const int* arr) {
    bool seen[N + 1] = { false };
    int dup = 0;
    for (int i = 0; i < N; ++i)
        if (arr[i] && seen[arr[i]]) ++dup;
        else if (arr[i]) seen[arr[i]] = true;
    return dup;
}

/* Oblicza wartoœæ fitness osobnika (liczba b³êdów w siatce) */
static int evaluateFitness(const Individual* ind) {
    int bad = 0;
    int tmp[N];
    for (int i = 0; i < N; ++i) {
        memcpy(tmp, ind->grid[i], sizeof tmp);
        bad += duplicatesInArray(tmp);
        for (int j = 0; j < N; ++j) tmp[j] = ind->grid[j][i];
        bad += duplicatesInArray(tmp);
    }
    for (int br = 0; br < N; br += BOXSIZE)
        for (int bc = 0; bc < N; bc += BOXSIZE) {
            int k = 0;
            for (int r = 0; r < BOXSIZE; ++r)
                for (int c = 0; c < BOXSIZE; ++c)
                    tmp[k++] = ind->grid[br + r][bc + c];
            bad += duplicatesInArray(tmp);
        }
    return bad;
}

/* Uzupe³nia brakuj¹ce liczby w wierszu losowo */
static void fillMissingNumbersRow(int row[N]) {
    bool present[N + 1] = { false };
    int missing[N], m = 0;
    for (int c = 0; c < N; ++c)
        if (row[c]) present[row[c]] = true;
    for (int v = 1; v <= N; ++v)
        if (!present[v]) missing[m++] = v;
    for (int i = m - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = missing[i]; missing[i] = missing[j]; missing[j] = tmp;
    }
    int k = 0;
    for (int c = 0; c < N; ++c)
        if (!row[c]) row[c] = missing[k++];
}

/* Inicjalizuje osobnika na podstawie bazowej siatki Sudoku */
static void initIndividual(Individual* ind, const int base[N][N]) {
    memcpy(ind->grid, base, sizeof(int) * N * N);
    for (int r = 0; r < N; ++r) fillMissingNumbersRow(ind->grid[r]);
    ind->fitness = evaluateFitness(ind);
}

/* Inicjalizuje ca³¹ populacjê osobników */
static void initPopulation(Individual pop[POP_SIZE], const int base[N][N]) {
    for (int i = 0; i < POP_SIZE; ++i) initIndividual(&pop[i], base);
}

/* Wybiera najlepszego osobnika z losowo wybranej grupy (turniej) */
static Individual* tournament(Individual pop[POP_SIZE]) {
    Individual* best = NULL;
    for (int i = 0; i < TOURNAMENT_K; ++i) {
        Individual* p = &pop[rand() % POP_SIZE];
        if (!best || p->fitness < best->fitness) best = p;
    }
    return best;
}

/* Tworzy nowego osobnika przez krzy¿owanie dwóch rodziców */
static void crossover(const Individual* p1, const Individual* p2,
    Individual* child, const bool fixed[N][N]) {
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            child->grid[r][c] = (rand() % 2) ? p1->grid[r][c] : p2->grid[r][c];
    for (int r = 0; r < N; ++r) {
        bool present[N + 1] = { false };
        for (int c = 0; c < N; ++c) present[child->grid[r][c]] = true;
        for (int v = 1; v <= N; ++v)
            if (!present[v]) {
                int col;
                do { col = rand() % N; } while (fixed[r][col]);
                child->grid[r][col] = v;
            }
    }
    child->fitness = evaluateFitness(child);
}

/* Mutuje osobnika zamieniaj¹c dwie losowe, nie-sta³e komórki w wierszu */
static void mutate(Individual* ind, const bool fixed[N][N]) {
    for (int r = 0; r < N; ++r)
        if ((float)rand() / RAND_MAX < MUT_RATE) {
            int c1, c2;
            do { c1 = rand() % N; } while (fixed[r][c1]);
            do { c2 = rand() % N; } while (fixed[r][c2] || c2 == c1);
            int tmp = ind->grid[r][c1]; ind->grid[r][c1] = ind->grid[r][c2]; ind->grid[r][c2] = tmp;
        }
    ind->fitness = evaluateFitness(ind);
}

/* Wykonuje algorytm genetyczny w celu rozwi¹zania Sudoku */
static Individual runGA(const int base[N][N]) {
    Individual population[POP_SIZE];
    Individual newPop[POP_SIZE];
    initPopulation(population, base);
    for (int gen = 0; gen < MAX_GEN; ++gen) {
        for (int i = 0; i < POP_SIZE - 1; ++i)
            for (int j = i + 1; j < POP_SIZE; ++j)
                if (population[j].fitness < population[i].fitness) {
                    Individual tmp = population[i];
                    population[i] = population[j];
                    population[j] = tmp;
                }
        if (population[0].fitness == 0) {
            return population[0];
        }
        if (gen % SHOW_EVERY == 0) {
            printf("Gen %d, najlepsza = %d\n", gen, population[0].fitness);
        }
        for (int i = 0; i < ELITE_KEEP; ++i) newPop[i] = population[i];
        for (int i = ELITE_KEEP; i < POP_SIZE; ++i) {
            Individual* p1 = tournament(population);
            Individual* p2 = tournament(population);
            crossover(p1, p2, &newPop[i], fixed);
            mutate(&newPop[i], fixed);
        }
        memcpy(population, newPop, sizeof population);
    }
    return population[0];
}

/* Punkt wejœcia programu: generuje Sudoku, uruchamia algorytm genetyczny i wyœwietla wynik */
int main(void) {
    srand((unsigned)time(NULL));
    int puzzle[N][N];
	generateSudoku(puzzle, 50);  //ilosc pustych komorek
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            fixed[r][c] = puzzle[r][c] != 0;
    puts("===== WYGENEROWANA ZAGADKA =====\n");
    printGrid(puzzle);
    puts("===== URUCHAMIAM ALGORYTM GENETYCZNY =====\n");
    Individual solution = runGA(puzzle);
    puts("\n===== WYNIK KOÑCOWY =====\n");
    printGrid(solution.grid);
    printf("Fitness = %d\n", solution.fitness);
    if (solution.fitness == 0)
        puts("\nSudoku rozwi¹zane!");

    return 0;
}