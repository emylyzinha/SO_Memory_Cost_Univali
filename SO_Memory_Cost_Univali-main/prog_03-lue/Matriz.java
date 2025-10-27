public class Matriz {
    public static void main(String[] args) {
        int N = 1000; 
        //int N = 10000; 
        //int N = 100000; 
        double[][] A = new double[N][N];
        double[][] B = new double[N][N];
        double[][] C = new double[N][N];

        // Inicializa as matrizes
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = 1.0;
                B[i][j] = 2.0;
                C[i][j] = 0.0;
            }
        }

        // Multiplicação de matrizes
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }

        System.out.println("Processamento concluído!");
    }
}