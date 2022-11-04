#include <stdio.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using std::cout;
using std::endl;
using std::string;
using std::to_string;

struct Matrix
{
    /* data */
    int **matrix;
    int rows, cols;
};

void poppulate(Matrix *M, int rows, int columns, string option = "rand")
{

    M->matrix = new int *[rows];
    M->cols = columns;
    M->rows = rows;

    for (int row = 0; row < rows; row++)
    {

        M->matrix[row] = new int[columns];

        if (option == "rand")

            for (int col = 0; col < columns; col++)
            {

                M->matrix[row][col] = rand() % 100 / 10;
            }
    }
}

void print(Matrix *M)
{

    int rows = M->rows, cols = M->cols;

    cout << "\n[";
    for (int r = 0; r < rows; r++)
    {

        if (r > 0)
        {
            cout << " ";
        }
        cout << "[";
        for (int c = 0; c < cols; c++)
        {
            if (c == (rows - 1))
            {
                cout << M->matrix[r][c];
            }
            else
            {
                cout << M->matrix[r][c] << ",";
            }
        }

        if (r == (rows - 1))
            cout << "]";
        else
            cout << "]" << endl;
    }
    cout << "] \n"
         << endl;
};

Matrix matrixmultiplication_serial(Matrix matrix_struct_1, Matrix matrix_struct_2)
{

    double time_spent = 0.0;
    clock_t begin = clock();

    int inp_cols = matrix_struct_1.cols, share_dim;

    share_dim = matrix_struct_1.rows;

    Matrix result;
    result.rows = matrix_struct_1.rows;
    result.cols = matrix_struct_2.cols;

    poppulate(&result, matrix_struct_1.rows, matrix_struct_2.cols, "any");

    for (int i = 0; i < matrix_struct_1.rows; i++)
    {

        for (int j = 0; j < inp_cols; j++)
        {
            result.matrix[i][j] = 0;

            for (int k = 0; k < share_dim; k++)
            {
                result.matrix[i][j] += matrix_struct_1.matrix[i][k] * matrix_struct_2.matrix[k][j];
            }
        }
    }

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nsequential opetation time spend:  " << time_spent << endl;
    return result;
};

Matrix matrixmultiplication_parallel_rows(Matrix matrix_struct_1, Matrix matrix_struct_2)
{
    int inp_cols = matrix_struct_1.cols, share_dim;

    int out_cols = matrix_struct_2.cols;

    share_dim = matrix_struct_1.rows;

    double time_spent = 0.0;
    clock_t begin = clock();

    Matrix result;
    poppulate(&result, matrix_struct_1.rows, matrix_struct_2.cols, "any");

    int pid_list[matrix_struct_1.rows];
    int file_descriptor[matrix_struct_1.rows][2];

    for (int i = 0; i < matrix_struct_1.rows; i++)
    {

        if (pipe(file_descriptor[i]) < 0)
        {
            perror("pipe error");
        }

        pid_list[i] = fork();

        if (pid_list[i] == -1)
        {
            printf("error");
            exit(0);
        }
        else if (pid_list[i] == 0)
        {

            close(file_descriptor[i][0]);

            int *row_result = new int[matrix_struct_1.rows];

            for (int j = 0; j < out_cols; j++)
            {

                int element_value = 0;
                for (int k = 0; k < share_dim; k++)
                {
                    element_value += matrix_struct_1.matrix[i][k] * matrix_struct_2.matrix[k][j];
                }

                row_result[j] = element_value;
            }

            write(file_descriptor[i][1], row_result, sizeof(int) * matrix_struct_1.rows);
            close(file_descriptor[i][1]);

            // free memmory
            free(row_result);
            exit(0);
            break;
        }
    }

    while (wait(NULL) != -1 || errno != ECHILD)
        ;

    for (int i = 0; i < matrix_struct_1.rows; i++)
    {

        close(file_descriptor[i][1]);

        int result_rows[matrix_struct_1.rows];
        read(file_descriptor[i][0], result_rows, sizeof(int) * matrix_struct_1.rows);

        for (int j = 0; j < matrix_struct_2.cols; j++)
        {
            result.matrix[i][j] = result_rows[j];
        }
    }

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nparallel row level time spend:  " << time_spent << endl;
    cout << "number of child process = " << matrix_struct_1.rows + 1 << endl;

    return result;
}

int main()
{

    Matrix matrix_1;
    Matrix matrix_2;

    poppulate(&matrix_1, 500, 500);
    poppulate(&matrix_2, 500, 500);

    // print(&matrix_1);
    // print(&matrix_2);

    Matrix mt3 = matrixmultiplication_serial(matrix_1, matrix_2);
    // print(&mt3);

    Matrix mt4 = matrixmultiplication_parallel_rows(matrix_1, matrix_2);
    // print(&mt4);

    return 0;
}