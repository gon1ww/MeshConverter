// Include standard library headers first
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MeshReader.h"
#include <fstream>
#include <filesystem>
#include <vtkTetra.h>
#include <vtkHexahedron.h>
#include <vtkWedge.h>
#include <vtkPyramid.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkLine.h>
#include <vtkVertex.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkStructuredGridReader.h>
#include <vtkRectilinearGridReader.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLPolyDataReader.h>
#ifdef HAVE_CGNS
#include <vtkCGNSReader.h>
#include <vtkMultiBlockDataSet.h>
#endif
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkFloatArray.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkPolyData.h>
#include <vtkPLYReader.h>

#ifdef _WIN32
#include <windows.h>

/**
 * @brief Convert UTF-8 string to wide string
 * @param utf8 UTF-8 string
 * @return Wide string
 */
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring wide(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wide[0], size_needed);
    return wide;
}

/**
 * @brief Set VTK reader file name with UTF-8 support
 * @param reader VTK reader
 * @param filePath UTF-8 encoded file path
 */
template<typename ReaderType>
void setVTKReaderFileName(vtkSmartPointer<ReaderType> reader, const std::string& filePath) {
#ifdef _WIN32
    // Use SetFileName with UTF-8 encoded path
    // VTK's SetFileName should handle UTF-8 correctly
    reader->SetFileName(filePath.c_str());
#else
    reader->SetFileName(filePath.c_str());
#endif
}

/**
 * @brief Set VTK reader file name with UTF-8 support (raw pointer version)
 * @param reader VTK reader
 * @param filePath UTF-8 encoded file path
 */
template<typename ReaderType>
void setVTKReaderFileName(ReaderType* reader, const std::string& filePath) {
#ifdef _WIN32
    // Use SetFileName with UTF-8 encoded path
    // VTK's SetFileName should handle UTF-8 correctly
    reader->SetFileName(filePath.c_str());
#else
    reader->SetFileName(filePath.c_str());
#endif
}

#endif

// Try to include Gmsh API header
#ifdef HAVE_GMSH
#ifdef __cplusplus
extern "C" {
#endif
#include "gmshc.h"
#ifdef __cplusplus
}
#endif
#endif

/**
 * @brief Check if file exists
 * @param filePath File path
 * @return Whether exists
 */
bool MeshReader::fileExists(const std::string& filePath) {
    return std::filesystem::exists(filePath);
}

/**
 * @brief Detect format from file header
 * @param filePath File path
 * @return Detected format
 */
MeshFormat MeshReader::detectFormatFromHeader(const std::string& filePath) {
    // Check if file exists
    if (!fileExists(filePath)) {
        return MeshFormat::UNKNOWN;
    }

    // First check file extension
    std::string lowerPath = filePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    
    if (lowerPath.substr(lowerPath.size() - 4) == ".vtk") {
        return MeshFormat::VTK_LEGACY;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".vtu" || 
               lowerPath.substr(lowerPath.size() - 4) == ".vtp" || 
               lowerPath.substr(lowerPath.size() - 4) == ".vti" || 
               lowerPath.substr(lowerPath.size() - 4) == ".vts") {
        return MeshFormat::VTK_XML;
    } else if (lowerPath.substr(lowerPath.size() - 5) == ".cgns") {
        return MeshFormat::CGNS;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".msh") {
        // Will check version later
        return MeshFormat::GMSH_V2;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".stl") {
        // Check if it's ASCII or Binary STL
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            char header[80] = {0};
            file.read(header, 80);
            std::string headerStr(header);
            file.close();
            
            // Check if header starts with "solid" (case-insensitive)
            std::string trimmedHeader = headerStr.substr(0, 5);
            std::transform(trimmedHeader.begin(), trimmedHeader.end(), trimmedHeader.begin(), ::tolower);
            if (trimmedHeader == "solid") {
                return MeshFormat::STL_ASCII;
            } else {
                return MeshFormat::STL_BINARY;
            }
        }
        return MeshFormat::STL_BINARY;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".obj") {
        return MeshFormat::OBJ;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".ply") {
        // Will check ASCII/Binary later
        // Continue to header detection to determine actual format
        // return MeshFormat::PLY_ASCII;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".off") {
        return MeshFormat::OFF;
    } else if (lowerPath.substr(lowerPath.size() - 4) == ".su2") {
        return MeshFormat::SU2;
    } else if (std::filesystem::is_directory(filePath) && std::filesystem::exists(filePath + "/polyMesh")) {
        return MeshFormat::OPENFOAM;
    }

    // If extension check fails, try header detection
    // Open file
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return MeshFormat::UNKNOWN;
    }

    // Read file header
    char header[128] = {0};
    file.read(header, sizeof(header));
    std::string headerStr(header);

    // Detect format
    if (headerStr.find("# vtk") != std::string::npos) {
        return MeshFormat::VTK_LEGACY;
    } else if (headerStr.find("<?xml") != std::string::npos && headerStr.find("VTKFile") != std::string::npos) {
        return MeshFormat::VTK_XML;
    } else if (headerStr.find("CGNS") != std::string::npos || 
               (header[0] == 'C' && header[1] == 'G' && header[2] == 'N' && header[3] == 'S')) {
        return MeshFormat::CGNS;
    } else if (headerStr.find("$MeshFormat") != std::string::npos) {
        // Check Gmsh version
        if (headerStr.find("4.") != std::string::npos) {
            return MeshFormat::GMSH_V4;
        } else {
            return MeshFormat::GMSH_V2;
        }
    } else if (headerStr.find("solid") != std::string::npos || headerStr.find("SOLID") != std::string::npos) {
        return MeshFormat::STL_ASCII;
    } else if (headerStr.substr(0, 8) == "\x00\x00\x00\x00") {
        // STL Binary feature
        return MeshFormat::STL_BINARY;
    } else if (headerStr.find("v ") != std::string::npos && headerStr.find("f ") != std::string::npos) {
        return MeshFormat::OBJ;
    } else if (headerStr.find("ply") != std::string::npos) {
        if (headerStr.find("ascii") != std::string::npos) {
            return MeshFormat::PLY_ASCII;
        } else {
            return MeshFormat::PLY_BINARY;
        }
    } else if (headerStr.find("OFF") != std::string::npos) {
        return MeshFormat::OFF;
    } else if (headerStr.find("SU2_MESH") != std::string::npos) {
        return MeshFormat::SU2;
    }

    return MeshFormat::UNKNOWN;
}

/**
 * @brief Auto-detect file format and read mesh data
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful (true=success, false=failure)
 */
bool MeshReader::readAuto(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg) {
    // Check if file exists
    if (!fileExists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File does not exist: " + filePath;
        return false;
    }

    // Detect format
    MeshFormat format = detectFormatFromHeader(filePath);
    if (format == MeshFormat::UNKNOWN) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot detect file format: " + filePath;
        return false;
    }

    // Call corresponding read method based on format
    switch (format) {
        case MeshFormat::VTK_LEGACY:
        case MeshFormat::VTK_XML:
            return readVTK(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::CGNS:
            return readCGNS(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::GMSH_V2:
        case MeshFormat::GMSH_V4:
            return readGmsh(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::STL_ASCII:
        case MeshFormat::STL_BINARY:
            return readSTL(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::OBJ:
            return readOBJ(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::PLY_ASCII:
        case MeshFormat::PLY_BINARY:
            return readPLY(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::OFF:
            return readOFF(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::SU2:
            return readSU2(filePath, meshData, errorCode, errorMsg);
        case MeshFormat::OPENFOAM:
            return readOpenFOAM(filePath, meshData, errorCode, errorMsg);
        default:
            errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
            errorMsg = "Format not supported: " + filePath;
            return false;
    }
}

/**
 * @brief Read VTK format file (support Legacy/XML auto-detection)
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readVTK(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Clear existing data
    meshData.clear();
    
    // Detect VTK format type
    MeshFormat format = detectFormatFromHeader(filePath);
    
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid;
    
    try {
        if (format == MeshFormat::VTK_LEGACY) {
            // Read Legacy VTK format - handle different dataset types
            vtkSmartPointer<vtkDataReader> reader = nullptr;
            vtkSmartPointer<vtkDataSet> dataset = nullptr;
            
            // First determine the dataset type from the file
            std::ifstream file(filePath);
            std::string line;
            std::string datasetType;
            
            // Skip header lines until we find DATASET line
            while (std::getline(file, line)) {
                if (line.substr(0, 8) == "DATASET ") {
                    datasetType = line.substr(8);
                    break;
                }
            }
            
            // Use appropriate reader based on dataset type
            if (datasetType == "UNSTRUCTURED_GRID") {
                // Use unstructured grid reader
                vtkSmartPointer<vtkUnstructuredGridReader> ugReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
                setVTKReaderFileName(ugReader, filePath);
                ugReader->Update();
                
                if (ugReader->GetOutput() && ugReader->GetOutput()->GetNumberOfPoints() > 0) {
                    unstructuredGrid = ugReader->GetOutput();
                } else {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Failed to read unstructured grid";
                    return false;
                }
            } else if (datasetType == "STRUCTURED_GRID") {
                // Use structured grid reader
                vtkSmartPointer<vtkStructuredGridReader> sgReader = vtkSmartPointer<vtkStructuredGridReader>::New();
                setVTKReaderFileName(sgReader, filePath);
                sgReader->Update();
                
                if (sgReader->GetOutput() && sgReader->GetOutput()->GetNumberOfPoints() > 0) {
                    vtkSmartPointer<vtkStructuredGrid> structuredGrid = sgReader->GetOutput();
                    // Convert structured grid to unstructured grid
                    unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                    unstructuredGrid->SetPoints(structuredGrid->GetPoints());
                    
                    // Convert structured cells to unstructured cells
                    int dims[3];
                    structuredGrid->GetDimensions(dims);
                    int numCells;
                    
                    // Handle 2D case (z dimension = 1)
                    if (dims[2] == 1) {
                        // 2D grid - create quad cells
                        numCells = (dims[0]-1) * (dims[1]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[4];
                            // Calculate cell indices for 2D structured grid
                            int j = i / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = j * dims[0] + I;
                            cellIds[1] = j * dims[0] + I + 1;
                            cellIds[2] = (j + 1) * dims[0] + I + 1;
                            cellIds[3] = (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_QUAD, 4, cellIds);
                        }
                    } else {
                        // 3D grid - create hexahedron cells
                        numCells = (dims[0]-1) * (dims[1]-1) * (dims[2]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[8];
                            // Calculate cell indices for 3D structured grid
                            int k = i / ((dims[0]-1) * (dims[1]-1));
                            int j = (i % ((dims[0]-1) * (dims[1]-1))) / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = k * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[1] = k * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[2] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[3] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            cellIds[4] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[5] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[6] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[7] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, cellIds);
                        }
                    }
                    
                    // Copy cell data
                    unstructuredGrid->GetCellData()->ShallowCopy(structuredGrid->GetCellData());
                    // Copy point data
                    unstructuredGrid->GetPointData()->ShallowCopy(structuredGrid->GetPointData());
                } else {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Failed to read structured grid";
                    return false;
                }
            } else if (datasetType == "RECTILINEAR_GRID") {
                // Use rectilinear grid reader
                vtkSmartPointer<vtkRectilinearGridReader> rgReader = vtkSmartPointer<vtkRectilinearGridReader>::New();
                setVTKReaderFileName(rgReader, filePath);
                rgReader->Update();
                
                if (rgReader->GetOutput() && rgReader->GetOutput()->GetNumberOfPoints() > 0) {
                    vtkSmartPointer<vtkRectilinearGrid> rectilinearGrid = rgReader->GetOutput();
                    // Convert rectilinear grid to unstructured grid
                    unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                    
                    // Create points from rectilinear grid
                    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                    int dims[3];
                    rectilinearGrid->GetDimensions(dims);
                    
                    for (int k = 0; k < dims[2]; k++) {
                        for (int j = 0; j < dims[1]; j++) {
                            for (int I = 0; I < dims[0]; I++) {
                                double x = rectilinearGrid->GetXCoordinates()->GetComponent(I, 0);
                                double y = rectilinearGrid->GetYCoordinates()->GetComponent(j, 0);
                                double z = rectilinearGrid->GetZCoordinates()->GetComponent(k, 0);
                                points->InsertNextPoint(x, y, z);
                            }
                        }
                    }
                    
                    unstructuredGrid->SetPoints(points);
                    
                    // Convert rectilinear cells to unstructured cells
                    int numCells;
                    
                    // Handle 2D case (z dimension = 1)
                    if (dims[2] == 1) {
                        // 2D grid - create quad cells
                        numCells = (dims[0]-1) * (dims[1]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[4];
                            // Calculate cell indices for 2D rectilinear grid
                            int j = i / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = j * dims[0] + I;
                            cellIds[1] = j * dims[0] + I + 1;
                            cellIds[2] = (j + 1) * dims[0] + I + 1;
                            cellIds[3] = (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_QUAD, 4, cellIds);
                        }
                    } else {
                        // 3D grid - create hexahedron cells
                        numCells = (dims[0]-1) * (dims[1]-1) * (dims[2]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[8];
                            // Calculate cell indices for 3D rectilinear grid
                            int k = i / ((dims[0]-1) * (dims[1]-1));
                            int j = (i % ((dims[0]-1) * (dims[1]-1))) / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = k * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[1] = k * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[2] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[3] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            cellIds[4] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[5] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[6] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[7] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, cellIds);
                        }
                    }
                    
                    // Copy cell data
                    unstructuredGrid->GetCellData()->ShallowCopy(rectilinearGrid->GetCellData());
                    // Copy point data
                    unstructuredGrid->GetPointData()->ShallowCopy(rectilinearGrid->GetPointData());
                } else {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Failed to read rectilinear grid";
                    return false;
                }
            } else if (datasetType == "POLYDATA") {
                // Use polydata reader
                vtkSmartPointer<vtkPolyDataReader> pdReader = vtkSmartPointer<vtkPolyDataReader>::New();
                setVTKReaderFileName(pdReader, filePath);
                pdReader->Update();
                
                if (pdReader->GetOutput() && pdReader->GetOutput()->GetNumberOfPoints() > 0) {
                    // Convert polydata to unstructured grid
                    vtkSmartPointer<vtkPolyData> polyData = pdReader->GetOutput();
                    unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                    unstructuredGrid->SetPoints(polyData->GetPoints());
                    
                    // Convert cells
                    for (vtkIdType i = 0; i < polyData->GetNumberOfCells(); i++) {
                        vtkCell* cell = polyData->GetCell(i);
                        if (cell) {
                            unstructuredGrid->InsertNextCell(cell->GetCellType(), cell->GetNumberOfPoints(), cell->GetPointIds()->GetPointer(0));
                        }
                    }
                    
                    // Copy cell data
                    unstructuredGrid->GetCellData()->ShallowCopy(polyData->GetCellData());
                    // Copy point data
                    unstructuredGrid->GetPointData()->ShallowCopy(polyData->GetPointData());
                } else {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Failed to read polydata";
                    return false;
                }
            } else {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Failed to read VTK file: unsupported dataset type: " + datasetType;
                return false;
            }
        } else if (format == MeshFormat::VTK_XML) {
            // Read XML VTK format - handle different dataset types
            vtkSmartPointer<vtkXMLDataReader> reader = nullptr;
            vtkSmartPointer<vtkDataSet> dataset = nullptr;
            
            // First try as unstructured grid
            vtkSmartPointer<vtkXMLUnstructuredGridReader> ugReader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
            setVTKReaderFileName(ugReader, filePath);
            ugReader->Update();
            
            if (ugReader->GetOutput() && ugReader->GetOutput()->GetNumberOfPoints() > 0) {
                unstructuredGrid = ugReader->GetOutput();
            } else {
                // Try as structured grid
                vtkSmartPointer<vtkXMLStructuredGridReader> sgReader = vtkSmartPointer<vtkXMLStructuredGridReader>::New();
                setVTKReaderFileName(sgReader, filePath);
                sgReader->Update();
                
                if (sgReader->GetOutput() && sgReader->GetOutput()->GetNumberOfPoints() > 0) {
                    vtkSmartPointer<vtkStructuredGrid> structuredGrid = sgReader->GetOutput();
                    // Convert structured grid to unstructured grid
                    unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                    unstructuredGrid->SetPoints(structuredGrid->GetPoints());
                    
                    // Convert structured cells to unstructured cells
                    int dims[3];
                    structuredGrid->GetDimensions(dims);
                    int numCells;
                    
                    // Handle 2D case (z dimension = 1)
                    if (dims[2] == 1) {
                        // 2D grid - create quad cells
                        numCells = (dims[0]-1) * (dims[1]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[4];
                            // Calculate cell indices for 2D structured grid
                            int j = i / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = j * dims[0] + I;
                            cellIds[1] = j * dims[0] + I + 1;
                            cellIds[2] = (j + 1) * dims[0] + I + 1;
                            cellIds[3] = (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_QUAD, 4, cellIds);
                        }
                    } else {
                        // 3D grid - create hexahedron cells
                        numCells = (dims[0]-1) * (dims[1]-1) * (dims[2]-1);
                        
                        for (int i = 0; i < numCells; i++) {
                            vtkIdType cellIds[8];
                            // Calculate cell indices for 3D structured grid
                            int k = i / ((dims[0]-1) * (dims[1]-1));
                            int j = (i % ((dims[0]-1) * (dims[1]-1))) / (dims[0]-1);
                            int I = i % (dims[0]-1);
                            
                            cellIds[0] = k * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[1] = k * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[2] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[3] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            cellIds[4] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I;
                            cellIds[5] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I + 1;
                            cellIds[6] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                            cellIds[7] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                            
                            unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, cellIds);
                        }
                    }
                    
                    // Copy cell data
                    unstructuredGrid->GetCellData()->ShallowCopy(structuredGrid->GetCellData());
                    // Copy point data
                    unstructuredGrid->GetPointData()->ShallowCopy(structuredGrid->GetPointData());
                } else {
                    // Try as rectilinear grid
                    vtkSmartPointer<vtkXMLRectilinearGridReader> rgReader = vtkSmartPointer<vtkXMLRectilinearGridReader>::New();
                    setVTKReaderFileName(rgReader, filePath);
                    rgReader->Update();
                    
                    if (rgReader->GetOutput() && rgReader->GetOutput()->GetNumberOfPoints() > 0) {
                        vtkSmartPointer<vtkRectilinearGrid> rectilinearGrid = rgReader->GetOutput();
                        // Convert rectilinear grid to unstructured grid
                        unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                        
                        // Create points from rectilinear grid
                        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                        int dims[3];
                        rectilinearGrid->GetDimensions(dims);
                        
                        for (int k = 0; k < dims[2]; k++) {
                            for (int j = 0; j < dims[1]; j++) {
                                for (int I = 0; I < dims[0]; I++) {
                                    double x = rectilinearGrid->GetXCoordinates()->GetComponent(I, 0);
                                    double y = rectilinearGrid->GetYCoordinates()->GetComponent(j, 0);
                                    double z = rectilinearGrid->GetZCoordinates()->GetComponent(k, 0);
                                    points->InsertNextPoint(x, y, z);
                                }
                            }
                        }
                        
                        unstructuredGrid->SetPoints(points);
                        
                        // Convert rectilinear cells to unstructured cells
                        int numCells;
                        
                        // Handle 2D case (z dimension = 1)
                        if (dims[2] == 1) {
                            // 2D grid - create quad cells
                            numCells = (dims[0]-1) * (dims[1]-1);
                            
                            for (int i = 0; i < numCells; i++) {
                                vtkIdType cellIds[4];
                                // Calculate cell indices for 2D rectilinear grid
                                int j = i / (dims[0]-1);
                                int I = i % (dims[0]-1);
                                
                                cellIds[0] = j * dims[0] + I;
                                cellIds[1] = j * dims[0] + I + 1;
                                cellIds[2] = (j + 1) * dims[0] + I + 1;
                                cellIds[3] = (j + 1) * dims[0] + I;
                                
                                unstructuredGrid->InsertNextCell(VTK_QUAD, 4, cellIds);
                            }
                        } else {
                            // 3D grid - create hexahedron cells
                            numCells = (dims[0]-1) * (dims[1]-1) * (dims[2]-1);
                            
                            for (int i = 0; i < numCells; i++) {
                                vtkIdType cellIds[8];
                                // Calculate cell indices for 3D rectilinear grid
                                int k = i / ((dims[0]-1) * (dims[1]-1));
                                int j = (i % ((dims[0] - 1) * (dims[1]-1))) / (dims[0]-1);
                                int I = i % (dims[0]-1);
                                
                                cellIds[0] = k * dims[0] * dims[1] + j * dims[0] + I;
                                cellIds[1] = k * dims[0] * dims[1] + j * dims[0] + I + 1;
                                cellIds[2] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                                cellIds[3] = k * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                                cellIds[4] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I;
                                cellIds[5] = (k + 1) * dims[0] * dims[1] + j * dims[0] + I + 1;
                                cellIds[6] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I + 1;
                                cellIds[7] = (k + 1) * dims[0] * dims[1] + (j + 1) * dims[0] + I;
                                
                                unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, 8, cellIds);
                            }
                        }
                        
                        // Copy cell data
                        unstructuredGrid->GetCellData()->ShallowCopy(rectilinearGrid->GetCellData());
                        // Copy point data
                        unstructuredGrid->GetPointData()->ShallowCopy(rectilinearGrid->GetPointData());
                    } else {
                        errorCode = MeshErrorCode::READ_FAILED;
                        errorMsg = "Failed to read VTK file: unsupported dataset type";
                        return false;
                    }
                }
            }
        } else {
            errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
            errorMsg = "Not a valid VTK file format";
            return false;
        }
        
        // Check if reading was successful
        if (!unstructuredGrid || unstructuredGrid->GetNumberOfPoints() == 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to read VTK file or file is empty";
            return false;
        }
        
        // Convert vtkUnstructuredGrid to MeshData
        // Points
        vtkIdType numPoints = unstructuredGrid->GetNumberOfPoints();
        meshData.points.reserve(numPoints * 3);
        
        for (vtkIdType i = 0; i < numPoints; ++i) {
            double point[3];
            unstructuredGrid->GetPoint(i, point);
            meshData.points.push_back(static_cast<float>(point[0]));
            meshData.points.push_back(static_cast<float>(point[1]));
            meshData.points.push_back(static_cast<float>(point[2]));
        }
        
        // Cells
        vtkIdType numCells = unstructuredGrid->GetNumberOfCells();
        meshData.cells.reserve(numCells);
        
        for (vtkIdType i = 0; i < numCells; ++i) {
            vtkCell* cell = unstructuredGrid->GetCell(i);
            if (!cell) continue;
            
            MeshData::Cell meshCell;
            
            // Convert VTK cell type to VtkCellType
            int vtkCellType = cell->GetCellType();
            switch (vtkCellType) {
                case VTK_VERTEX:
                    meshCell.type = VtkCellType::VERTEX;
                    break;
                case VTK_LINE:
                    meshCell.type = VtkCellType::LINE;
                    break;
                case VTK_POLY_LINE:
                    // 处理POLY_LINE类型，使用LINE类型存储
                    meshCell.type = VtkCellType::LINE;
                    break;
                case VTK_TRIANGLE:
                    meshCell.type = VtkCellType::TRIANGLE;
                    break;
                case VTK_TRIANGLE_STRIP:
                    meshCell.type = VtkCellType::TRIANGLE_STRIP;
                    break;
                case VTK_POLYGON:
                    meshCell.type = VtkCellType::POLYGON;
                    break;
                case VTK_PIXEL:
                    // 处理PIXEL类型，使用QUAD类型存储
                    meshCell.type = VtkCellType::QUAD;
                    break;
                case VTK_QUAD:
                    meshCell.type = VtkCellType::QUAD;
                    break;
                case VTK_TETRA:
                    meshCell.type = VtkCellType::TETRA;
                    break;
                case VTK_VOXEL:
                    // 处理VOXEL类型，使用HEXAHEDRON类型存储
                    meshCell.type = VtkCellType::HEXAHEDRON;
                    break;
                case VTK_HEXAHEDRON:
                    meshCell.type = VtkCellType::HEXAHEDRON;
                    break;
                case VTK_WEDGE:
                    meshCell.type = VtkCellType::WEDGE;
                    break;
                case VTK_PYRAMID:
                    meshCell.type = VtkCellType::PYRAMID;
                    break;
                default:
                    // 添加调试信息，查看未处理的单元类型
                    std::cout << "Warning: Skipping unsupported cell type: " << vtkCellType << std::endl;
                    continue; // Skip unsupported cell types
            }
            
            // Get point indices
            vtkIdType numCellPoints = cell->GetNumberOfPoints();
            meshCell.pointIndices.reserve(numCellPoints);
            
            for (vtkIdType j = 0; j < numCellPoints; ++j) {
                meshCell.pointIndices.push_back(static_cast<uint32_t>(cell->GetPointId(j)));
            }
            
            meshData.cells.push_back(meshCell);
        }
        
        // Cell Data
        vtkCellData* cellData = unstructuredGrid->GetCellData();
        if (cellData) {
            int numCellArrays = cellData->GetNumberOfArrays();
            for (int i = 0; i < numCellArrays; ++i) {
                vtkDataArray* array = cellData->GetArray(i);
                if (array) {
                    std::string arrayName = array->GetName();
                    if (arrayName.empty()) {
                        arrayName = "CellArray_" + std::to_string(i);
                    }
                    
                    vtkIdType numTuples = array->GetNumberOfTuples();
                    int numComponents = array->GetNumberOfComponents();
                    
                    std::vector<float> data;
                    data.reserve(numTuples * numComponents);
                    
                    for (vtkIdType j = 0; j < numTuples; ++j) {
                        for (int k = 0; k < numComponents; ++k) {
                            double value = array->GetComponent(j, k);
                            data.push_back(static_cast<float>(value));
                        }
                    }
                    
                    meshData.cellData[arrayName] = data;
                }
            }
        }
        
        // Point Data
        vtkPointData* pointData = unstructuredGrid->GetPointData();
        if (pointData) {
            int numPointArrays = pointData->GetNumberOfArrays();
            for (int i = 0; i < numPointArrays; ++i) {
                vtkDataArray* array = pointData->GetArray(i);
                if (array) {
                    std::string arrayName = array->GetName();
                    if (arrayName.empty()) {
                        arrayName = "PointArray_" + std::to_string(i);
                    }
                    
                    vtkIdType numTuples = array->GetNumberOfTuples();
                    int numComponents = array->GetNumberOfComponents();
                    
                    std::vector<float> data;
                    data.reserve(numTuples * numComponents);
                    
                    for (vtkIdType j = 0; j < numTuples; ++j) {
                        for (int k = 0; k < numComponents; ++k) {
                            double value = array->GetComponent(j, k);
                            data.push_back(static_cast<float>(value));
                        }
                    }
                    
                    meshData.pointData[arrayName] = data;
                }
            }
        }
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        return true;
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading VTK file: ") + e.what();
        return false;
    }
}

/**
 * @brief Read CGNS format file
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @param baseIndex CGNS Base index (default 0, first Base)
 * @param zoneIndex CGNS Zone index (default 0, first Zone)
 * @return Whether reading is successful
 */
bool MeshReader::readCGNS(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg,
                         int baseIndex,
                         int zoneIndex) {
    // Clear existing data
    meshData.clear();
    
#ifdef HAVE_CGNS
    try {
        // Use VTK's CGNS reader to read the file
        vtkSmartPointer<vtkCGNSReader> reader = vtkSmartPointer<vtkCGNSReader>::New();
        setVTKReaderFileName(reader, filePath);
        reader->Update();
        
        vtkSmartPointer<vtkMultiBlockDataSet> output = reader->GetOutput();
        
        // Check if reading was successful
        if (!output || output->GetNumberOfBlocks() == 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to read CGNS file or file is empty";
            return false;
        }
        
        // Find the first unstructured grid in the multi-block dataset
        vtkSmartPointer<vtkUnstructuredGrid> grid;
        
        // Non-recursive approach to find unstructured grid in multi-block dataset
        std::vector<vtkDataObject*> objectsToCheck;
        objectsToCheck.push_back(output);
        
        while (!objectsToCheck.empty() && !grid) {
            vtkDataObject* current = objectsToCheck.back();
            objectsToCheck.pop_back();
            
            if (vtkUnstructuredGrid::SafeDownCast(current)) {
                grid = vtkUnstructuredGrid::SafeDownCast(current);
            } else if (vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(current)) {
                for (unsigned int i = 0; i < mb->GetNumberOfBlocks(); ++i) {
                    vtkDataObject* child = mb->GetBlock(i);
                    if (child) {
                        objectsToCheck.push_back(child);
                    }
                }
            }
        }
        
        if (!grid || grid->GetNumberOfPoints() == 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "No unstructured grid found in CGNS file or grid is empty";
            return false;
        }
        
        // Convert vtkUnstructuredGrid to MeshData
        // Points
        vtkIdType numPoints = grid->GetNumberOfPoints();
        meshData.points.reserve(numPoints * 3);
        
        for (vtkIdType i = 0; i < numPoints; ++i) {
            double point[3];
            grid->GetPoint(i, point);
            meshData.points.push_back(static_cast<float>(point[0]));
            meshData.points.push_back(static_cast<float>(point[1]));
            meshData.points.push_back(static_cast<float>(point[2]));
        }
        
        // Cells
        vtkIdType numCells = grid->GetNumberOfCells();
        meshData.cells.reserve(numCells);
        
        for (vtkIdType i = 0; i < numCells; ++i) {
            vtkCell* cell = grid->GetCell(i);
            if (!cell) continue;
            
            MeshData::Cell meshCell;
            
            // Convert VTK cell type to VtkCellType
            int vtkCellType = cell->GetCellType();
            switch (vtkCellType) {
                case VTK_VERTEX:
                    meshCell.type = VtkCellType::VERTEX;
                    break;
                case VTK_LINE:
                    meshCell.type = VtkCellType::LINE;
                    break;
                case VTK_TRIANGLE:
                    meshCell.type = VtkCellType::TRIANGLE;
                    break;
                case VTK_QUAD:
                    meshCell.type = VtkCellType::QUAD;
                    break;
                case VTK_TETRA:
                    meshCell.type = VtkCellType::TETRA;
                    break;
                case VTK_HEXAHEDRON:
                    meshCell.type = VtkCellType::HEXAHEDRON;
                    break;
                case VTK_WEDGE:
                    meshCell.type = VtkCellType::WEDGE;
                    break;
                case VTK_PYRAMID:
                    meshCell.type = VtkCellType::PYRAMID;
                    break;
                default:
                    continue; // Skip unsupported cell types
            }
            
            // Get point indices
            vtkIdType numCellPoints = cell->GetNumberOfPoints();
            meshCell.pointIndices.reserve(numCellPoints);
            
            for (vtkIdType j = 0; j < numCellPoints; ++j) {
                meshCell.pointIndices.push_back(static_cast<uint32_t>(cell->GetPointId(j)));
            }
            
            meshData.cells.push_back(meshCell);
        }
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        return true;
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading CGNS file: ") + e.what();
        return false;
    }
#else
    errorCode = MeshErrorCode::DEPENDENCY_MISSING;
    errorMsg = "CGNS support is not available (HAVE_CGNS not defined)";
    return false;
#endif
}

/**
 * @brief Read Gmsh format file (support v2/v4 auto-detection)
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readGmsh(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg) {
    // Clear existing data
    meshData.clear();
    
    #ifdef HAVE_GMSH
    // Gmsh C API implementation
    int ierr = 0;
    
    // Initialize Gmsh API
    gmshInitialize(0, nullptr, 0, 0, &ierr);
    if (ierr != 0) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Error initializing Gmsh API";
        return false;
    }
    
    try {
        // Open the Gmsh file
#ifdef _WIN32
        // Gmsh API expects const char* even on Windows
        gmshOpen(reinterpret_cast<const char*>(filePath.c_str()), &ierr);
#else
        gmshOpen(filePath.c_str(), &ierr);
#endif
        if (ierr != 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Error opening Gmsh file: " + filePath;
            gmshFinalize(&ierr);
            return false;
        }
        
        // Get all nodes in the mesh
        size_t *nodeTags = nullptr;
        size_t nodeTags_n = 0;
        double *coord = nullptr;
        size_t coord_n = 0;
        double *parametricCoord = nullptr;
        size_t parametricCoord_n = 0;
        
        gmshModelMeshGetNodes(&nodeTags, &nodeTags_n, &coord, &coord_n, &parametricCoord, &parametricCoord_n, -1, -1, 0, 0, &ierr);
        if (ierr != 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Error getting nodes from Gmsh file";
            gmshFree(nodeTags);
            gmshFree(coord);
            gmshFree(parametricCoord);
            gmshFinalize(&ierr);
            return false;
        }
        
        // Convert nodes to meshData.points
        // Note: coord is [x1, y1, z1, x2, y2, z2, ...]
        meshData.points.reserve(coord_n);
        for (size_t i = 0; i < coord_n; ++i) {
            meshData.points.push_back(static_cast<float>(coord[i]));
        }
        
        // Get all elements in the mesh
        int *elementTypes = nullptr;
        size_t elementTypes_n = 0;
        size_t **elementTags = nullptr;
        size_t *elementTags_n = nullptr;
        size_t elementTags_nn = 0;
        size_t **nodeTagsPerElement = nullptr;
        size_t *nodeTagsPerElement_n = nullptr;
        size_t nodeTagsPerElement_nn = 0;
        
        gmshModelMeshGetElements(&elementTypes, &elementTypes_n, &elementTags, &elementTags_n, &elementTags_nn, &nodeTagsPerElement, &nodeTagsPerElement_n, &nodeTagsPerElement_nn, -1, -1, &ierr);
        if (ierr != 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Error getting elements from Gmsh file";
            gmshFree(nodeTags);
            gmshFree(coord);
            gmshFree(parametricCoord);
            gmshFinalize(&ierr);
            return false;
        }
        
        // Process each element type
        for (size_t i = 0; i < elementTypes_n; ++i) {
            int elementType = elementTypes[i];
            const size_t *elements = elementTags[i];
            size_t elements_n = elementTags_n[i];
            const size_t *nodes = nodeTagsPerElement[i];
            size_t nodes_n = nodeTagsPerElement_n[i];
            
            // Get element properties to determine number of nodes per element
            char *elementName = nullptr;
            int dim = 0;
            int order = 0;
            int numNodes = 0;
            double *localNodeCoord = nullptr;
            size_t localNodeCoord_n = 0;
            int numPrimaryNodes = 0;
            
            gmshModelMeshGetElementProperties(elementType, &elementName, &dim, &order, &numNodes, &localNodeCoord, &localNodeCoord_n, &numPrimaryNodes, &ierr);
            if (ierr != 0) {
                gmshFree(elementName);
                gmshFree(localNodeCoord);
                continue;
            }
            
            // Map Gmsh element type to VtkCellType
            VtkCellType cellType = VtkCellType::VERTEX;
            bool supportedType = true;
            
            switch (elementType) {
                case 15: // Gmsh node
                    cellType = VtkCellType::VERTEX;
                    break;
                case 1: // Gmsh line
                    cellType = VtkCellType::LINE;
                    break;
                case 2: // Gmsh triangle
                    cellType = VtkCellType::TRIANGLE;
                    break;
                case 3: // Gmsh quad
                    cellType = VtkCellType::QUAD;
                    break;
                case 4: // Gmsh tetrahedron
                    cellType = VtkCellType::TETRA;
                    break;
                case 5: // Gmsh hexahedron
                    cellType = VtkCellType::HEXAHEDRON;
                    break;
                case 6: // Gmsh prism
                    cellType = VtkCellType::WEDGE;
                    break;
                case 7: // Gmsh pyramid
                    cellType = VtkCellType::PYRAMID;
                    break;
                default:
                    // Skip unsupported cell types
                    supportedType = false;
                    break;
            }
            
            // Free element properties memory
            gmshFree(elementName);
            gmshFree(localNodeCoord);
            
            if (!supportedType) {
                continue;
            }
            
            // Process each element of this type
            // Note: nodes vector is [e1n1, e1n2, ..., e1nN, e2n1, ...]
            size_t nodeIndex = 0;
            for (size_t j = 0; j < elements_n; ++j) {
                MeshData::Cell meshCell;
                meshCell.type = cellType;
                meshCell.pointIndices.reserve(numNodes);
                
                // Read node indices for this element
                for (int k = 0; k < numNodes; ++k) {
                    if (nodeIndex >= nodes_n) break;
                    size_t nodeTag = nodes[nodeIndex++];
                    // Find the index of this node in nodeTags
                    // Note: This is necessary because nodeTags may not be contiguous
                    size_t nodeIdx = 0;
                    for (; nodeIdx < nodeTags_n; ++nodeIdx) {
                        if (nodeTags[nodeIdx] == nodeTag) {
                            break;
                        }
                    }
                    if (nodeIdx < nodeTags_n) {
                        meshCell.pointIndices.push_back(static_cast<uint32_t>(nodeIdx));
                    }
                }
                
                meshData.cells.push_back(meshCell);
            }
        }
        
        // Free memory
        gmshFree(nodeTags);
        gmshFree(coord);
        gmshFree(parametricCoord);
        gmshFree(elementTypes);
        for (size_t i = 0; i < elementTypes_n; ++i) {
            gmshFree(elementTags[i]);
            gmshFree(nodeTagsPerElement[i]);
        }
        gmshFree(elementTags);
        gmshFree(elementTags_n);
        gmshFree(nodeTagsPerElement);
        gmshFree(nodeTagsPerElement_n);
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        // Finalize Gmsh API
        gmshFinalize(&ierr);
        
        return true;
        
    } catch (const std::exception& e) {
        // Finalize Gmsh API before returning
        gmshFinalize(&ierr);
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading Gmsh file: ") + e.what();
        return false;
    } catch (...) {
        // Finalize Gmsh API before returning
        gmshFinalize(&ierr);
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Unknown error reading Gmsh file";
        return false;
    }
    #else
    // Original file I/O implementation as fallback
    // This ensures backward compatibility when Gmsh API is not available
    errorCode = MeshErrorCode::READ_FAILED;
    errorMsg = "Gmsh API is not available. Please install Gmsh and rebuild the project.";
    return false;
    #endif
}

/**
 * @brief Read STL format file (ASCII/Binary)
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readSTL(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Clear existing data
    meshData.clear();
    
    // Check if file exists
    if (!fileExists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File does not exist: " + filePath;
        return false;
    }
    
    try {
        // Open file
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to open file: " + filePath;
            return false;
        }
        
        // Detect STL format
        MeshFormat format = detectFormatFromHeader(filePath);
        if (format != MeshFormat::STL_ASCII && format != MeshFormat::STL_BINARY) {
            errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
            errorMsg = "Not a valid STL file format";
            return false;
        }
        
        // Reset file position
        file.clear();
        file.seekg(0, std::ios::beg);
        
        if (format == MeshFormat::STL_ASCII) {
            // Read ASCII STL format
            return readSTLASCII(file, meshData, errorCode, errorMsg);
        } else {
            // Read Binary STL format
            return readSTLBinary(file, meshData, errorCode, errorMsg);
        }
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading STL file: ") + e.what();
        return false;
    }
}

/**
 * @brief Read ASCII STL format
 * @param file Input file stream
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readSTLASCII(std::ifstream& file,
                             MeshData& meshData,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg) {
    std::string line;
    std::vector<float> currentTriangle;
    
    // Skip solid line
    if (!std::getline(file, line)) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Invalid ASCII STL file: empty file";
        return false;
    }
    
    // Process facets
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        // Trim whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        
        // Check for facet start
        if (line.substr(0, 6) == "facet ") {
            // Start of new facet, reset current triangle
            currentTriangle.clear();
            
            // Read normal vector
            std::istringstream normalStream(line.substr(6));
            std::string normalStr;
            normalStream >> normalStr; // Should be "normal"
            
            float nx, ny, nz;
            if (!(normalStream >> nx >> ny >> nz)) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: malformed normal vector";
                return false;
            }
            
            // Read outer loop
            if (!std::getline(file, line)) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: unexpected end of file";
                return false;
            }
            
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            // Remove trailing whitespace
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());
            
            if (line != "outer loop") {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: expected 'outer loop', got '" + line + "'";
                return false;
            }
            
            // Read three vertices
            for (int i = 0; i < 3; ++i) {
                if (!std::getline(file, line)) {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Invalid ASCII STL file: unexpected end of file";
                    return false;
                }
                
                // Trim whitespace
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                
                if (line.substr(0, 7) != "vertex ") {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Invalid ASCII STL file: expected 'vertex'";
                    return false;
                }
                
                // Read vertex coordinates
                std::istringstream vertexStream(line.substr(7));
                float x, y, z;
                if (!(vertexStream >> x >> y >> z)) {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Invalid ASCII STL file: malformed vertex coordinates";
                    return false;
                }
                
                // Add vertex to current triangle
                currentTriangle.push_back(x);
                currentTriangle.push_back(y);
                currentTriangle.push_back(z);
            }
            
            // Read endloop
            if (!std::getline(file, line)) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: unexpected end of file";
                return false;
            }
            
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            // Remove trailing whitespace
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());
            
            if (line != "endloop") {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: expected 'endloop', got '" + line + "'";
                return false;
            }
            
            // Read endfacet
            if (!std::getline(file, line)) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: unexpected end of file";
                return false;
            }
            
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            // Remove trailing whitespace
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());
            
            if (line != "endfacet") {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid ASCII STL file: expected 'endfacet', got '" + line + "'";
                return false;
            }
            
            // Add triangle to mesh data
            if (currentTriangle.size() == 9) { // 3 vertices * 3 coordinates
                // Add points
                size_t startIndex = meshData.points.size() / 3;
                meshData.points.insert(meshData.points.end(), currentTriangle.begin(), currentTriangle.end());
                
                // Add cell
                MeshData::Cell cell;
                cell.type = VtkCellType::TRIANGLE;
                cell.pointIndices.push_back(static_cast<uint32_t>(startIndex));
                cell.pointIndices.push_back(static_cast<uint32_t>(startIndex + 1));
                cell.pointIndices.push_back(static_cast<uint32_t>(startIndex + 2));
                meshData.cells.push_back(cell);
            }
        } else if (line.substr(0, 8) == "endsolid") {
            // End of solid, done reading
            break;
        }
    }
    
    // Check if any triangles were read
    if (meshData.cells.empty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "No triangles found in ASCII STL file";
        return false;
    }
    
    // Calculate metadata
    meshData.calculateMetadata();
    
    return true;
}

/**
 * @brief Read Binary STL format
 * @param file Input file stream
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readSTLBinary(std::ifstream& file,
                              MeshData& meshData,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // Skip 80-byte header
    char header[80];
    if (!file.read(header, 80)) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Invalid binary STL file: incomplete header";
        return false;
    }
    
    // Read number of triangles
    uint32_t triangleCount;
    if (!file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount))) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Invalid binary STL file: incomplete triangle count";
        return false;
    }
    
    // Validate triangle count
    const uint32_t MAX_TRIANGLES = 10000000; // 10 million triangles as reasonable limit
    if (triangleCount == 0) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "Binary STL file contains no triangles";
        return false;
    }
    if (triangleCount > MAX_TRIANGLES) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Binary STL file declares too many triangles: " + std::to_string(triangleCount) + 
                  " (maximum allowed: " + std::to_string(MAX_TRIANGLES) + ")";
        return false;
    }
    
    // Calculate expected file size to validate
    std::streampos currentPos = file.tellg();
    file.seekg(0, std::ios::end);
    std::streampos fileEnd = file.tellg();
    file.seekg(currentPos);
    
    // Calculate total file size and data size
    uint64_t totalFileSize = static_cast<uint64_t>(fileEnd);
    uint64_t expectedDataSize = static_cast<uint64_t>(triangleCount) * 50; // 50 bytes per triangle
    uint64_t expectedTotalSize = 84 + expectedDataSize; // 84 bytes header + data
    
    if (totalFileSize < expectedTotalSize) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Binary STL file is too small for declared triangle count";
        return false;
    }
    
    // Reserve space for points and cells
    try {
        meshData.points.reserve(triangleCount * 9); // 3 vertices * 3 coordinates per triangle
        meshData.cells.reserve(triangleCount);
    } catch (const std::bad_alloc&) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Insufficient memory to read STL file: triangle count too large";
        return false;
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Error reserving memory for STL file: " + std::string(e.what());
        return false;
    }
    
    // Read triangles
    for (uint32_t i = 0; i < triangleCount; ++i) {
        // Read normal vector (3 floats)
        float normal[3];
        if (!file.read(reinterpret_cast<char*>(normal), sizeof(normal))) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid binary STL file: incomplete normal vector";
            return false;
        }
        
        // Read 3 vertices (9 floats)
        float vertices[9];
        if (!file.read(reinterpret_cast<char*>(vertices), sizeof(vertices))) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid binary STL file: incomplete vertex data";
            return false;
        }
        
        // Read 2-byte attribute count (usually 0)
        uint16_t attributeCount;
        if (!file.read(reinterpret_cast<char*>(&attributeCount), sizeof(attributeCount))) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid binary STL file: incomplete attribute count";
            return false;
        }
        
        // Add points
        size_t startIndex = meshData.points.size() / 3;
        for (int j = 0; j < 9; ++j) {
            meshData.points.push_back(vertices[j]);
        }
        
        // Add cell
        MeshData::Cell cell;
        cell.type = VtkCellType::TRIANGLE;
        cell.pointIndices.push_back(static_cast<uint32_t>(startIndex));
        cell.pointIndices.push_back(static_cast<uint32_t>(startIndex + 1));
        cell.pointIndices.push_back(static_cast<uint32_t>(startIndex + 2));
        meshData.cells.push_back(cell);
    }
    
    // Check if any triangles were read
    if (meshData.cells.empty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "No triangles found in binary STL file";
        return false;
    }
    
    // Calculate metadata
    meshData.calculateMetadata();
    
    return true;
}

/**
 * @brief Read OBJ format file
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readOBJ(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Clear existing data
    meshData.clear();
    
    // Check if file exists
    if (!fileExists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File does not exist: " + filePath;
        return false;
    }
    
    try {
        // Open file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to open file: " + filePath;
            return false;
        }
        
        std::string line;
        std::vector<float> vertices;
        std::vector<std::vector<uint32_t>> faces;
        
        // Read file line by line
        while (std::getline(file, line)) {
            // Skip empty lines
            if (line.empty()) continue;
            
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            // Skip comments
            if (line.empty() || line[0] == '#') continue;
            
            // Check if it's a vertex definition
            if (line.substr(0, 2) == "v ") {
                std::istringstream vertexStream(line.substr(2));
                float x, y, z;
                if (vertexStream >> x >> y >> z) {
                    vertices.push_back(x);
                    vertices.push_back(y);
                    vertices.push_back(z);
                }
            }
            // Check if it's a face definition
            else if (line.substr(0, 2) == "f ") {
                std::istringstream faceStream(line.substr(2));
                std::vector<uint32_t> faceIndices;
                std::string vertexRef;
                
                while (faceStream >> vertexRef) {
                    // OBJ uses 1-based indices, so subtract 1 to make it 0-based
                    size_t slashPos = vertexRef.find('/');
                    std::string vertexIndexStr = vertexRef.substr(0, slashPos);
                    uint32_t vertexIndex = std::stoi(vertexIndexStr) - 1;
                    faceIndices.push_back(vertexIndex);
                }
                
                if (!faceIndices.empty()) {
                    faces.push_back(faceIndices);
                }
            }
            // Check if it's a line definition
            else if (line.substr(0, 2) == "l ") {
                std::istringstream lineStream(line.substr(2));
                std::vector<uint32_t> lineIndices;
                std::string vertexRef;
                
                while (lineStream >> vertexRef) {
                    // OBJ uses 1-based indices, so subtract 1 to make it 0-based
                    size_t slashPos = vertexRef.find('/');
                    std::string vertexIndexStr = vertexRef.substr(0, slashPos);
                    uint32_t vertexIndex = std::stoi(vertexIndexStr) - 1;
                    lineIndices.push_back(vertexIndex);
                }
                
                if (!lineIndices.empty()) {
                    // For lines, create line cells
                    MeshData::Cell cell;
                    cell.type = VtkCellType::LINE;
                    
                    for (uint32_t index : lineIndices) {
                        cell.pointIndices.push_back(index);
                    }
                    
                    meshData.cells.push_back(cell);
                }
            }
        }
        
        // Check if any data was read
        if (vertices.empty()) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "No vertices found in OBJ file";
            return false;
        }
        
        // Check if vertices form complete triplets
        if (vertices.size() % 3 != 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid vertex data in OBJ file";
            return false;
        }
        
        // Add vertices to meshData
        meshData.points = vertices;
        
        // Add faces to meshData
        for (const auto& faceIndices : faces) {
            if (faceIndices.size() < 3) {
                // Skip faces with less than 3 vertices
                continue;
            }
            
            MeshData::Cell cell;
            
            // Determine cell type based on number of vertices
            if (faceIndices.size() == 3) {
                cell.type = VtkCellType::TRIANGLE;
            } else if (faceIndices.size() == 4) {
                cell.type = VtkCellType::QUAD;
            } else {
                // For polygons with more than 4 vertices, use POLYGON type
                cell.type = VtkCellType::POLYGON;
            }
            
            // Add vertex indices
            for (uint32_t index : faceIndices) {
                cell.pointIndices.push_back(index);
            }
            
            meshData.cells.push_back(cell);
        }
        
        // Check if any cells were added
        if (meshData.cells.empty()) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "No valid cells found in OBJ file";
            return false;
        }
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        return true;
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading OBJ file: ") + e.what();
        return false;
    }
}

/**
 * @brief Read PLY format file (ASCII/Binary)
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readPLY(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Clear existing data
    meshData.clear();
    
    // Check if file exists
    if (!fileExists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File does not exist: " + filePath;
        return false;
    }
    
    try {
        // Open file
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to open file: " + filePath;
            return false;
        }
        
        // Read header
        std::string line;
        bool headerEnd = false;
        bool isBinary = false;
        uint32_t vertexCount = 0;
        uint32_t faceCount = 0;
        
        // Read first line
        if (!std::getline(file, line)) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid PLY file: empty file";
            return false;
        }
        
        // Check if it's a PLY file
        if (line != "ply") {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid PLY file: missing 'ply' header";
            return false;
        }
        
        // Read header lines
        while (!headerEnd && std::getline(file, line)) {
            // Trim whitespace
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            
            if (line.empty()) continue;
            
            // Check for format
            if (line.substr(0, 7) == "format ") {
                std::istringstream formatStream(line.substr(7));
                std::string format, version;
                formatStream >> format >> version;
                
                if (format == "binary_little_endian") {
                    isBinary = true;
                } else if (format == "binary_big_endian") {
                    isBinary = true;
                } else if (format == "ascii") {
                    isBinary = false;
                } else {
                    errorCode = MeshErrorCode::READ_FAILED;
                    errorMsg = "Invalid PLY file: unsupported format";
                    return false;
                }
            }
            // Check for element vertex
            else if (line.substr(0, 8) == "element ") {
                std::istringstream elementStream(line.substr(8));
                std::string elementName;
                uint32_t count;
                elementStream >> elementName >> count;
                
                if (elementName == "vertex") {
                    vertexCount = count;
                } else if (elementName == "face") {
                    faceCount = count;
                }
            }
            // Check for end of header
            else if (line == "end_header") {
                headerEnd = true;
            }
        }
        
        // Check if header is valid
        if (!headerEnd) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Invalid PLY file: missing 'end_header'";
            return false;
        }
        
        if (vertexCount == 0) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "No vertices found in PLY file";
            return false;
        }
        
        // Read vertices
        std::vector<float> vertices;
        vertices.reserve(vertexCount * 3);
        
        // Read vertex data
        for (uint32_t i = 0; i < vertexCount; ++i) {
            float x, y, z;
            
            // Read x coordinate
            if (!file.read(reinterpret_cast<char*>(&x), sizeof(float))) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid PLY file: incomplete vertex data";
                return false;
            }
            
            // Read y coordinate
            if (!file.read(reinterpret_cast<char*>(&y), sizeof(float))) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid PLY file: incomplete vertex data";
                return false;
            }
            
            // Read z coordinate
            if (!file.read(reinterpret_cast<char*>(&z), sizeof(float))) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Invalid PLY file: incomplete vertex data";
                return false;
            }
            
            // Add vertex
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
        
        // Read faces
        std::vector<MeshData::Cell> cells;
        cells.reserve(faceCount);
        
        // Read face data
        for (uint32_t i = 0; i < faceCount; ++i) {
            uint8_t vertexCountPerFace;
            
            // Read vertex count per face
            if (!file.read(reinterpret_cast<char*>(&vertexCountPerFace), sizeof(uint8_t))) {
                // Skip face data if we can't read it
                // This allows us to read the file even if there's an issue with the face data
                break;
            }
            
            // Read vertex indices
            std::vector<int> indices(vertexCountPerFace);
            if (!file.read(reinterpret_cast<char*>(indices.data()), vertexCountPerFace * sizeof(int))) {
                // Skip face data if we can't read it
                // This allows us to read the file even if there's an issue with the face data
                break;
            }
            
            // Create cell
            if (vertexCountPerFace >= 3) {
                MeshData::Cell cell;
                
                // Determine cell type based on number of vertices
                if (vertexCountPerFace == 3) {
                    cell.type = VtkCellType::TRIANGLE;
                } else if (vertexCountPerFace == 4) {
                    cell.type = VtkCellType::QUAD;
                } else {
                    cell.type = VtkCellType::POLYGON;
                }
                
                // Add vertex indices
                for (int index : indices) {
                    cell.pointIndices.push_back(static_cast<uint32_t>(index));
                }
                
                cells.push_back(cell);
            }
        }
        
        // Add data to meshData
        meshData.points = vertices;
        meshData.cells = cells;
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        return true;
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading PLY file: ") + e.what();
        return false;
    }
}

/**
 * @brief Read OFF format file
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readOFF(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Clear existing mesh data
    meshData.clear();
    
    // Open file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File not found or cannot be opened";
        return false;
    }
    
    // Read header
    std::string header;
    if (!std::getline(file, header)) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Failed to read OFF header";
        file.close();
        return false;
    }
    
    // Check header
    std::istringstream headerStream(header);
    std::string magic;
    headerStream >> magic;
    
    if (magic != "OFF") {
        errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
        errorMsg = "Invalid OFF header. Expected 'OFF'";
        file.close();
        return false;
    }
    
    // Read vertex, face, edge counts
    int numVertices, numFaces, numEdges;
    if (!(file >> numVertices >> numFaces >> numEdges)) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = "Failed to read vertex, face, edge counts";
        file.close();
        return false;
    }
    
    // Read vertices
    meshData.points.reserve(numVertices * 3);
    for (int i = 0; i < numVertices; ++i) {
        float x, y, z;
        if (!(file >> x >> y >> z)) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to read vertex coordinates";
            file.close();
            return false;
        }
        meshData.points.push_back(x);
        meshData.points.push_back(y);
        meshData.points.push_back(z);
    }
    
    // Read faces
    meshData.cells.reserve(numFaces);
    for (int i = 0; i < numFaces; ++i) {
        int numFaceVertices;
        if (!(file >> numFaceVertices)) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to read face vertex count";
            file.close();
            return false;
        }
        
        std::vector<uint32_t> pointIndices;
        pointIndices.reserve(numFaceVertices);
        
        for (int j = 0; j < numFaceVertices; ++j) {
            int vertexIndex;
            if (!(file >> vertexIndex)) {
                errorCode = MeshErrorCode::READ_FAILED;
                errorMsg = "Failed to read face vertex index";
                file.close();
                return false;
            }
            pointIndices.push_back(static_cast<uint32_t>(vertexIndex));
        }
        
        // Create cell based on number of vertices
        MeshData::Cell cell;
        switch (numFaceVertices) {
        case 3:
            cell.type = VtkCellType::TRIANGLE;
            break;
        case 4:
            cell.type = VtkCellType::QUAD;
            break;
        default:
            cell.type = VtkCellType::POLYGON;
            break;
        }
        cell.pointIndices = std::move(pointIndices);
        meshData.cells.push_back(std::move(cell));
    }
    
    // Close file
    file.close();
    
    // Calculate metadata
    meshData.calculateMetadata();
    meshData.metadata.format = MeshFormat::OFF;
    
    // Set success
    errorCode = MeshErrorCode::SUCCESS;
    errorMsg = "";
    return true;
}

/**
 * @brief Read SU2 format file
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readSU2(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Implement SU2 format read logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "SU2 format read not implemented";
    return false;
}

/**
 * @brief Read OpenFOAM format file
 * @param filePath File path (UTF-8 encoded)
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether reading is successful
 */
bool MeshReader::readOpenFOAM(const std::string& filePath,
                             MeshData& meshData,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg) {
    // Implement OpenFOAM format read logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "OpenFOAM format read not implemented";
    return false;
}

// --------------------------------------------------------------------------
// VTK intermediate format related method implementations
// --------------------------------------------------------------------------

/**
 * @brief Convert MeshData to vtkUnstructuredGrid
 * @param meshData Input mesh data
 * @return vtkUnstructuredGrid pointer
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::meshDataToVTK(const MeshData& meshData) {
    // Create vtkUnstructuredGrid
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    
    // Create point set
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    
    // Add points
    size_t pointCount = meshData.points.size() / 3;
    for (size_t i = 0; i < pointCount; ++i) {
        float x = meshData.points[i * 3];
        float y = meshData.points[i * 3 + 1];
        float z = meshData.points[i * 3 + 2];
        points->InsertNextPoint(x, y, z);
    }
    
    grid->SetPoints(points);
    
    // Add cells
    for (const auto& cell : meshData.cells) {
        switch (cell.type) {
        case VtkCellType::TETRA:
            if (cell.pointIndices.size() == 4) {
                vtkIdType ids[4];
                for (int i = 0; i < 4; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_TETRA, 4, ids);
            }
            break;
        case VtkCellType::HEXAHEDRON:
            if (cell.pointIndices.size() == 8) {
                vtkIdType ids[8];
                for (int i = 0; i < 8; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_HEXAHEDRON, 8, ids);
            }
            break;
        case VtkCellType::WEDGE:
            if (cell.pointIndices.size() == 6) {
                vtkIdType ids[6];
                for (int i = 0; i < 6; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_WEDGE, 6, ids);
            }
            break;
        case VtkCellType::PYRAMID:
            if (cell.pointIndices.size() == 5) {
                vtkIdType ids[5];
                for (int i = 0; i < 5; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_PYRAMID, 5, ids);
            }
            break;
        case VtkCellType::TRIANGLE:
            if (cell.pointIndices.size() == 3) {
                vtkIdType ids[3];
                for (int i = 0; i < 3; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_TRIANGLE, 3, ids);
            }
            break;
        case VtkCellType::QUAD:
            if (cell.pointIndices.size() == 4) {
                vtkIdType ids[4];
                for (int i = 0; i < 4; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_QUAD, 4, ids);
            }
            break;
        case VtkCellType::LINE:
            if (cell.pointIndices.size() == 2) {
                vtkIdType ids[2];
                for (int i = 0; i < 2; ++i) {
                    ids[i] = cell.pointIndices[i];
                }
                grid->InsertNextCell(VTK_LINE, 2, ids);
            }
            break;
        case VtkCellType::VERTEX:
            if (cell.pointIndices.size() == 1) {
                vtkIdType ids[1];
                ids[0] = cell.pointIndices[0];
                grid->InsertNextCell(VTK_VERTEX, 1, ids);
            }
            break;
        default:
            break;
        }
    }
    
    // Copy point data from meshData to vtkUnstructuredGrid
    for (const auto& [name, values] : meshData.pointData) {
        vtkSmartPointer<vtkFloatArray> array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetName(name.c_str());
        array->SetNumberOfComponents(1);
        array->SetNumberOfTuples(values.size());
        for (size_t i = 0; i < values.size(); ++i) {
            array->SetValue(i, values[i]);
        }
        grid->GetPointData()->AddArray(array);
    }
    
    // Copy cell data from meshData to vtkUnstructuredGrid
    for (const auto& [name, values] : meshData.cellData) {
        vtkSmartPointer<vtkFloatArray> array = vtkSmartPointer<vtkFloatArray>::New();
        array->SetName(name.c_str());
        array->SetNumberOfComponents(1);
        array->SetNumberOfTuples(values.size());
        for (size_t i = 0; i < values.size(); ++i) {
            array->SetValue(i, values[i]);
        }
        grid->GetCellData()->AddArray(array);
    }
    
    return grid;
}

/**
 * @brief Auto-detect file format and read as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readAutoToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg) {
    // Detect format first
    MeshFormat format = detectFormatFromHeader(filePath);
    
    // Use format-specific VTK readers for better compatibility
    switch (format) {
        case MeshFormat::PLY_ASCII:
        case MeshFormat::PLY_BINARY:
            return readPLYToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::VTK_LEGACY:
        case MeshFormat::VTK_XML:
            return readVTKToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::CGNS:
            return readCGNSToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::GMSH_V2:
        case MeshFormat::GMSH_V4:
            return readGmshToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::STL_ASCII:
        case MeshFormat::STL_BINARY:
            return readSTLToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::OBJ:
            return readOBJToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::OFF:
            return readOFFToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::SU2:
            return readSU2ToVTK(filePath, errorCode, errorMsg);
        case MeshFormat::OPENFOAM:
            return readOpenFOAMToVTK(filePath, errorCode, errorMsg);
        default:
            // Fall back to MeshData method for unknown formats
            MeshData meshData;
            bool success = readAuto(filePath, meshData, errorCode, errorMsg);
            if (!success) {
                return nullptr;
            }
            return meshDataToVTK(meshData);
    }
}

/**
 * @brief Read VTK format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readVTKToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // Detect VTK format type
    MeshFormat format = detectFormatFromHeader(filePath);
    
    vtkSmartPointer<vtkUnstructuredGrid> grid;
    
    try {
        if (format == MeshFormat::VTK_LEGACY) {
            // First try to read as UnstructuredGrid
            vtkSmartPointer<vtkUnstructuredGridReader> ugReader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
            ugReader->SetFileName(filePath.c_str());
            ugReader->Update();
            grid = ugReader->GetOutput();
            
            // If UnstructuredGrid failed, try PolyData
            if (!grid || grid->GetNumberOfPoints() == 0) {
                vtkSmartPointer<vtkPolyDataReader> pdReader = vtkSmartPointer<vtkPolyDataReader>::New();
                pdReader->SetFileName(filePath.c_str());
                pdReader->Update();
                vtkSmartPointer<vtkPolyData> polyData = pdReader->GetOutput();
                
                // Convert PolyData to UnstructuredGrid
                if (polyData && polyData->GetNumberOfPoints() > 0) {
                    grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                    grid->SetPoints(polyData->GetPoints());
                    
                    // Copy cells from PolyData to UnstructuredGrid
                    // Copy vertices
                    vtkCellArray* verts = polyData->GetVerts();
                    if (verts) {
                        vtkIdType npts;
                        const vtkIdType* pts;
                        verts->InitTraversal();
                        while (verts->GetNextCell(npts, pts)) {
                            grid->InsertNextCell(VTK_VERTEX, npts, pts);
                        }
                    }
                    
                    // Copy lines
                    vtkCellArray* lines = polyData->GetLines();
                    if (lines) {
                        vtkIdType npts;
                        const vtkIdType* pts;
                        lines->InitTraversal();
                        while (lines->GetNextCell(npts, pts)) {
                            if (npts == 2) {
                                grid->InsertNextCell(VTK_LINE, npts, pts);
                            } else {
                                grid->InsertNextCell(VTK_POLY_LINE, npts, pts);
                            }
                        }
                    }
                    
                    // Copy polygons
                    vtkCellArray* polys = polyData->GetPolys();
                    if (polys) {
                        vtkIdType npts;
                        const vtkIdType* pts;
                        polys->InitTraversal();
                        while (polys->GetNextCell(npts, pts)) {
                            if (npts == 3) {
                                grid->InsertNextCell(VTK_TRIANGLE, npts, pts);
                            } else if (npts == 4) {
                                grid->InsertNextCell(VTK_QUAD, npts, pts);
                            } else if (npts > 4) {
                                grid->InsertNextCell(VTK_POLYGON, npts, pts);
                            }
                        }
                    }
                    
                    // Copy triangle strips
                    vtkCellArray* strips = polyData->GetStrips();
                    if (strips) {
                        vtkIdType npts;
                        const vtkIdType* pts;
                        strips->InitTraversal();
                        while (strips->GetNextCell(npts, pts)) {
                            grid->InsertNextCell(VTK_TRIANGLE_STRIP, npts, pts);
                        }
                    }
                    
                    // Copy cell data and point data
                    grid->GetCellData()->ShallowCopy(polyData->GetCellData());
                    grid->GetPointData()->ShallowCopy(polyData->GetPointData());
                    
                    std::cout << "Successfully converted PolyData to UnstructuredGrid" << std::endl;
                }
            }
        } else if (format == MeshFormat::VTK_XML) {
            // Read XML VTK format
            vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
            reader->SetFileName(filePath.c_str());
            reader->Update();
            grid = reader->GetOutput();
        } else {
            errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
            errorMsg = "Not a valid VTK file format";
            return nullptr;
        }
        
        // Check if reading was successful
        if (!grid || grid->GetNumberOfPoints() == 0) {
            errorCode = MeshErrorCode::READ_FAILED;
            errorMsg = "Failed to read VTK file or file is empty";
            return nullptr;
        }
        
        return grid;
        
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error reading VTK file: ") + e.what();
        return nullptr;
    }
}

/**
 * @brief Read CGNS format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @param baseIndex CGNS Base index (default 0, first Base)
 * @param zoneIndex CGNS Zone index (default 0, first Zone)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readCGNSToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg,
                                                              int baseIndex,
                                                              int zoneIndex) {
#ifdef HAVE_CGNS
    // First use existing readCGNS method to read as MeshData
    MeshData meshData;
    bool success = readCGNS(filePath, meshData, errorCode, errorMsg, baseIndex, zoneIndex);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
#else
    errorCode = MeshErrorCode::DEPENDENCY_MISSING;
    errorMsg = "CGNS support is not available (HAVE_CGNS not defined)";
    return nullptr;
#endif
}

/**
 * @brief Read Gmsh format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readGmshToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg) {
    // First use existing readGmsh method to read as MeshData
    MeshData meshData;
    bool success = readGmsh(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}

/**
 * @brief Read STL format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readSTLToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // First use existing readSTL method to read as MeshData
    MeshData meshData;
    bool success = readSTL(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}

/**
 * @brief Read OBJ format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readOBJToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // First use existing readOBJ method to read as MeshData
    MeshData meshData;
    bool success = readOBJ(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}

/**
 * @brief Read PLY format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readPLYToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // Use the existing readPLY method to read into MeshData which handles all PLY formats
    MeshData meshData;
    bool success = readPLY(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    vtkSmartPointer<vtkUnstructuredGrid> grid = meshDataToVTK(meshData);
    
    // If no cells but we have points, create vertex cells for each point
    if (grid && grid->GetNumberOfCells() == 0 && grid->GetNumberOfPoints() > 0) {
        for (vtkIdType i = 0; i < grid->GetNumberOfPoints(); ++i) {
            grid->InsertNextCell(VTK_VERTEX, 1, &i);
        }
    }
    
    return grid;
}

/**
 * @brief Read OFF format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readOFFToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // First use existing readOFF method to read as MeshData
    MeshData meshData;
    bool success = readOFF(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}

/**
 * @brief Read SU2 format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readSU2ToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg) {
    // First use existing readSU2 method to read as MeshData
    MeshData meshData;
    bool success = readSU2(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}

/**
 * @brief Read OpenFOAM format file as vtkUnstructuredGrid
 * @param filePath File path (UTF-8 encoded)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return vtkUnstructuredGrid pointer, returns nullptr on failure
 */
vtkSmartPointer<vtkUnstructuredGrid> MeshReader::readOpenFOAMToVTK(const std::string& filePath,
                                                                  MeshErrorCode& errorCode,
                                                                  std::string& errorMsg) {
    // First use existing readOpenFOAM method to read as MeshData
    MeshData meshData;
    bool success = readOpenFOAM(filePath, meshData, errorCode, errorMsg);
    if (!success) {
        return nullptr;
    }
    
    // Convert MeshData to vtkUnstructuredGrid
    return meshDataToVTK(meshData);
}
