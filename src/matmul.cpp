#include <bits/stdc++.h>
#include <filesystem> // TODO: añadir a la documentación de que se requiere C++17
using namespace std;
namespace fs = filesystem;

// TODO: modificar Makefile para que este archivo también se compile pero no se linkee al programa principal 

void printRow(vector<int>& row, ostream& ostream) {
    for (int& val : row) ostream << val << " ";
}

void printRowLn(vector<int>& row, ostream& ostream) {
    for (int& val : row) ostream << val << " ";
    ostream << endl;
}

bool parseMatrixFromFile(const fs::path& path, vector<vector<int>>& matrix, const char separator) {
    matrix.clear();

    // open file to read
    ifstream file(path);
    if (!file) {
        cerr << "el archivo " << path << " no pudo leerse" << endl;
        return false;
    }

    // parse file
    string line;
    int rowNumber = 0;
    while (getline(file, line)) {
        stringstream ss(line);
        // ignore only whitespace lines
        // TODO: que approach es mejor?
        if (all_of(line.begin(), line.end(), [](const char& c) {return std::isspace(c);})) {
            continue;
        }
        if ((ss >> std::ws).eof()) continue; 
        
        vector<int> row;
        string tempStr;
        // TODO: que ocurre si hay espacio en blanco después o antes de un valor de la matriz?
        while (getline(ss, tempStr, separator)) {
            try {
                row.push_back(stoi(tempStr));
            }
            catch (const invalid_argument& e) {
                cerr << "un valor de la matriz no pudo ser interpretado como numero:" << endl;
                // print rowNumber 1-indexed
                cerr << rowNumber+1 << "  .";
                printRow(row, std::cerr);
                cerr << tempStr << endl;

                return false;
            } 
            catch (const out_of_range& e) {
                cerr << "un valor de la matriz se escapa de rango" << endl;
                // print rowNumber 1-indexed
                cerr << rowNumber+1 << "  .";
                printRow(row, std::cerr);
                cerr << tempStr << endl;

                return false;
            }
        }

        matrix.push_back(row);

        // check rows are the same size
        if (rowNumber > 0 && matrix[rowNumber-1].size() != matrix[rowNumber].size()) {
            cerr << "matriz en " << path << " tiene filas de distinto largo" << endl;

            // print rowNumber 1-indexed
            cerr << rowNumber   << ".  ";
            printRowLn(matrix[rowNumber-1], std::cerr);
            cerr << rowNumber+1 << ".  ";
            printRowLn(matrix[rowNumber], std::cerr);
            
            return false;
        }

        rowNumber++;
    }

    if (matrix.size() == 0) {
        cerr << "no se encontro matriz en " << path << endl;
        return false;
    }

    return true;
}

// WARNING: falla si los valores de la matriz resultante no caben en un int
vector<vector<int>> matMulNaive(const vector<vector<int>>& a, const vector<vector<int>>& b) {
	assert (a.size() > 0 && a[0].size() > 0 && b.size() > 0 && b[0].size() > 0 && a[0].size() == b.size());
	size_t N = b[0].size();
	size_t M = a.size();

	vector<vector<int>> c(N, vector<int>(M, 0));

	for (size_t i = 0; i < N; i++) {
		for (size_t j = 0; j < M; j++) {
			for (size_t k = 0; k < a[0].size(); k++) {
                c[j][i] += a[j][k] * b[k][i];
			}	
		}
	}

    return c;
}

// TODO: optional faster method 
//int matMulKaratsuba(const vector<vector<int>>& a, const vector<vector<int>>& b) {
//    return 0;
//}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "arcg = " << argc << endl; 
        cerr << "Uso: " << argv[0] << " <ruta absoluta a primer archivo txt> <ruta absoluta a segundo archivo txt> <separador>" << endl;
        return 1;
    }

    filesystem::path A_path(argv[1]); 
    if (!filesystem::exists(A_path) || !A_path.is_absolute()) {
        cerr << "archivo " << argv[1] << " no existe o no es una ruta absoluta" << endl;
        return 2;
    }

    filesystem::path B_path(argv[2]); 
    if (!filesystem::exists(B_path) || !B_path.is_absolute()) {
        cerr << "archivo " << argv[2] << " no existe o no es una ruta absoluta" << endl;
        return 2;
    }

    if (strlen(argv[3]) != 1) {
        cerr << "el separador debe ser un solo caracter" << endl;
        return 3; 
    }
    char separator = argv[3][0];

    
    vector<vector<int>> A;
    if (!parseMatrixFromFile(A_path, A, separator)) return 4;
    
    vector<vector<int>> B;
    if (!parseMatrixFromFile(B_path, B, separator)) return 4;
    
    
    if (A[0].size() != B.size()) {
        cerr << "las matrices tienen dimensiones incompatibles. ";
        cerr << "( " << A.size() << "x" << A[0].size() << " y " << B.size() << "x" << B[0].size() << " )" << endl; 
        return 5;
    }
    
    vector<vector<int>> C = matMulNaive(A, B);

    for (vector<int>& row : C) printRowLn(row, std::cout);
    cout << endl;

    return 0;
}