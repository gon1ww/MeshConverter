#include <iostream>
#include <string>
#include "VTKConverter.h"
#include "MeshReader.h"
#include "MeshTypes.h"
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

/**
 * @brief Test program to verify that MeshReader correctly reads CellData from VTU files
 */
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        std::cerr << "Usage: MeshReaderCellDataTest <input VTU file path>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];

    std::cout << "=== MeshReader CellData Read Test ===" << std::endl;
    std::cout << "Input VTU file: " << inputPath << std::endl;

    MeshErrorCode errorCode;
    std::string errorMsg;
    
    // Test 1: Read directly using MeshReader
    std::cout << "\n--- Test 1: Direct MeshReader.readAutoToVTK ---" << std::endl;
    vtkSmartPointer<vtkUnstructuredGrid> grid = MeshReader::readAutoToVTK(inputPath, errorCode, errorMsg);
    
    if (!grid) {
        std::cerr << "MeshReader read failed: " << errorMsg << std::endl;
        return 1;
    }
    
    std::cout << "✓ MeshReader read successful" << std::endl;
    std::cout << "- Number of points: " << grid->GetNumberOfPoints() << std::endl;
    std::cout << "- Number of cells: " << grid->GetNumberOfCells() << std::endl;
    
    // Check CellData
    std::cout << "\n--- CellData Analysis ---" << std::endl;
    int numCellDataArrays = grid->GetCellData()->GetNumberOfArrays();
    std::cout << "- Number of CellData arrays: " << numCellDataArrays << std::endl;
    
    for (int i = 0; i < numCellDataArrays; ++i) {
        vtkDataArray* array = grid->GetCellData()->GetArray(i);
        std::cout << "  [" << i << "] " << array->GetName() 
                  << " (" << array->GetNumberOfComponents() << " components, " 
                  << array->GetNumberOfTuples() << " tuples)" << std::endl;
    }
    
    // Check PointData
    std::cout << "\n--- PointData Analysis ---" << std::endl;
    int numPointDataArrays = grid->GetPointData()->GetNumberOfArrays();
    std::cout << "- Number of PointData arrays: " << numPointDataArrays << std::endl;
    
    for (int i = 0; i < numPointDataArrays; ++i) {
        vtkDataArray* array = grid->GetPointData()->GetArray(i);
        std::cout << "  [" << i << "] " << array->GetName() 
                  << " (" << array->GetNumberOfComponents() << " components, " 
                  << array->GetNumberOfTuples() << " tuples)" << std::endl;
    }
    
    // Print actual values
    std::cout << "\n--- Sample CellData Values ---" << std::endl;
    if (numCellDataArrays > 0) {
        for (int i = 0; i < numCellDataArrays; ++i) {
            vtkDataArray* array = grid->GetCellData()->GetArray(i);
            std::cout << array->GetName() << ": ";
            for (int j = 0; j < array->GetNumberOfTuples() && j < 8; ++j) {
                double val = array->GetTuple1(j);
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    return 0;
}
