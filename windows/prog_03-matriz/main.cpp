#include <iostream>
#include <vector>

int main() {
    const int N = 1000; 
    //const int N = 10000; 
    //const int N = 100000; 
    std::vector<std::vector<double>> A(N, std::vector<double>(N, 1.0));
    std::vector<std::vector<double>> B(N, std::vector<double>(N, 2.0));
    std::vector<std::vector<double>> C(N, std::vector<double>(N, 0.0));

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];

    std::cout << "Processamento concluído!\n";
    return 0;
}
