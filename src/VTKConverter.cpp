// Include standard library headers
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>

#include "VTKConverter.h"
#include "MeshReader.h"
#include "MeshWriter.h"
#include "MeshTypes.h"
#include <vtkUnstructuredGrid.h>
#include <vtkPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>

#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkPLYReader.h>
#include <vtkPLYWriter.h>
#include <vtkOBJReader.h>
#include <vtkGLTFWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkDataSetWriter.h>
#include <vtkPolyDataWriter.h>
#include <vtkOBJWriter.h>
#include <vtkGeometryFilter.h>
#ifdef HAVE_CGNS
#include "cgnslib.h"
#endif
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
static bool fileExists(const std::string& filePath) {
    return std::filesystem::exists(filePath);
}

/**
 * @brief Convert source format file to VTK format
 * @param srcFilePath Source file path
 * @param vtkGrid Output VTK unstructured grid
 * @param errorCode Output error code
 * @param errorMsg Output error message
 * @return Whether conversion is successful
 */
bool VTKConverter::convertToVTK(const std::string& srcFilePath, 
                                 vtkSmartPointer<vtkUnstructuredGrid>& vtkGrid, 
                                 MeshErrorCode& errorCode, 
                                 std::string& errorMsg) {
    try {
        // Use MeshReader's readAutoToVTK method to handle various formats
        vtkGrid = MeshReader::readAutoToVTK(srcFilePath, errorCode, errorMsg);
        if (!vtkGrid) {
            return false;
        }

        // Validate VTK data
        if (vtkGrid->GetNumberOfPoints() == 0) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "No points found in VTK data";
            return false;
        }

        if (vtkGrid->GetNumberOfCells() == 0) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "No cells found in VTK data";
            return false;
        }

        std::cout << "Successfully converted to VTK format" << std::endl;
        std::cout << "- Number of points: " << vtkGrid->GetNumberOfPoints() << std::endl;
        std::cout << "- Number of cells: " << vtkGrid->GetNumberOfCells() << std::endl;
        std::cout << "- Number of cell data arrays: " << vtkGrid->GetCellData()->GetNumberOfArrays() << std::endl;
        for (int i = 0; i < vtkGrid->GetCellData()->GetNumberOfArrays(); ++i) {
            vtkDataArray* array = vtkGrid->GetCellData()->GetArray(i);
            std::cout << "  - Cell data " << i << ": " << array->GetName() << ", components: " << array->GetNumberOfComponents() << ", tuples: " << array->GetNumberOfTuples() << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::READ_FAILED;
        errorMsg = std::string("Error converting to VTK: ") + e.what();
        return false;
    }
}

/**
 * @brief Process and optimize VTK data
 * @param inputGrid Input VTK unstructured grid
 * @param options Processing options
 * @param outputGrid Output processed VTK unstructured grid
 * @param errorCode Output error code
 * @param errorMsg Output error message
 * @return Whether processing is successful
 */
bool VTKConverter::processVTKData(const vtkSmartPointer<vtkUnstructuredGrid>& inputGrid, 
                                   const VTKProcessingOptions& options, 
                                   vtkSmartPointer<vtkUnstructuredGrid>& outputGrid, 
                                   MeshErrorCode& errorCode, 
                                   std::string& errorMsg) {
    try {
        std::cout << "Processing VTK data..." << std::endl;
        std::cout << "- Input points: " << inputGrid->GetNumberOfPoints() << std::endl;
        std::cout << "- Input cells: " << inputGrid->GetNumberOfCells() << std::endl;
        std::cout << "- Input cell data arrays: " << inputGrid->GetCellData()->GetNumberOfArrays() << std::endl;
        for (int i = 0; i < inputGrid->GetCellData()->GetNumberOfArrays(); ++i) {
            vtkDataArray* array = inputGrid->GetCellData()->GetArray(i);
            std::cout << "  - " << array->GetName() << " (" << array->GetNumberOfComponents() << " components, " << array->GetNumberOfTuples() << " tuples)" << std::endl;
        }

        // Separate cells into surface cells (for polydata processing) and volumetric cells (for direct copying)
        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer<vtkPoints> surfacePoints = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkCellArray> surfaceCells = vtkSmartPointer<vtkCellArray>::New();
        
        // Count cell types
        int surfaceCellCount = 0;
        int volumetricCellCount = 0;

        // First pass: collect surface cells for polydata processing
        for (vtkIdType i = 0; i < inputGrid->GetNumberOfCells(); ++i) {
            vtkCell* cell = inputGrid->GetCell(i);
            int cellType = cell->GetCellType();
            
            if (cellType == VTK_TRIANGLE || cellType == VTK_QUAD) {
                // Surface cell - add to polydata
                vtkIdType npts = cell->GetNumberOfPoints();
                vtkIdType* pts = cell->GetPointIds()->GetPointer(0);
                surfaceCells->InsertNextCell(npts, pts);
                surfaceCellCount++;
            } else {
                // Volumetric cell - will be copied directly later
                volumetricCellCount++;
            }
        }

        std::cout << "- Surface cells (triangles/quads): " << surfaceCellCount << std::endl;
        std::cout << "- Volumetric cells: " << volumetricCellCount << std::endl;

        // Create output grid
        outputGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

        // Critical fix: If there are volumetric cells, we must NOT use point cleaning or other processing
        // that changes point indices, because volumetric cells reference original point indices!
        if (volumetricCellCount > 0) {
            std::cout << "Volumetric cells present - using safe processing mode" << std::endl;
            
            // Use original points directly for volumetric meshes
            outputGrid->SetPoints(inputGrid->GetPoints());
            
            // Copy ALL cells directly without any polydata processing
            // This preserves the original point indices for volumetric cells
            for (vtkIdType i = 0; i < inputGrid->GetNumberOfCells(); ++i) {
                vtkCell* cell = inputGrid->GetCell(i);
                int cellType = cell->GetCellType();
                vtkIdType npts = cell->GetNumberOfPoints();
                vtkIdType* pts = cell->GetPointIds()->GetPointer(0);
                
                switch (cellType) {
                    case VTK_TRIANGLE:
                        outputGrid->InsertNextCell(VTK_TRIANGLE, npts, pts);
                        break;
                    case VTK_QUAD:
                        outputGrid->InsertNextCell(VTK_QUAD, npts, pts);
                        break;
                    case VTK_TETRA:
                        outputGrid->InsertNextCell(VTK_TETRA, npts, pts);
                        break;
                    case VTK_HEXAHEDRON:
                        outputGrid->InsertNextCell(VTK_HEXAHEDRON, npts, pts);
                        break;
                    case VTK_WEDGE:
                        outputGrid->InsertNextCell(VTK_WEDGE, npts, pts);
                        break;
                    case VTK_PYRAMID:
                        outputGrid->InsertNextCell(VTK_PYRAMID, npts, pts);
                        break;
                    case VTK_LINE:
                        outputGrid->InsertNextCell(VTK_LINE, npts, pts);
                        break;
                    case VTK_VERTEX:
                        outputGrid->InsertNextCell(VTK_VERTEX, npts, pts);
                        break;
                    default:
                        // Skip unsupported cell types
                        break;
                }
            }
            
            // Copy cell data from input grid to output grid
            outputGrid->GetCellData()->DeepCopy(inputGrid->GetCellData());
            // Copy point data from input grid to output grid
            outputGrid->GetPointData()->DeepCopy(inputGrid->GetPointData());
        } else {
            // No volumetric cells - it's safe to use full polydata processing
            std::cout << "No volumetric cells - using full processing pipeline" << std::endl;
            
            // Process surface cells through polydata pipeline
            vtkSmartPointer<vtkPolyData> processedPolyData = nullptr;
            if (surfaceCellCount > 0) {
                // Copy all points from input grid to polydata
                surfacePoints->DeepCopy(inputGrid->GetPoints());
                polyData->SetPoints(surfacePoints);
                polyData->SetPolys(surfaceCells);
                
                // Copy cell data from input grid to polydata
                polyData->GetCellData()->DeepCopy(inputGrid->GetCellData());
                // Copy point data from input grid to polydata
                polyData->GetPointData()->DeepCopy(inputGrid->GetPointData());

                // Apply processing steps
                processedPolyData = polyData;

                // 1. Clean duplicate points
                if (options.enableCleaning) {
                    std::cout << "Applying point cleaning..." << std::endl;
                    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
                    cleaner->SetInputData(processedPolyData);
                    cleaner->SetTolerance(0.0001);
                    cleaner->Update();
                    processedPolyData = cleaner->GetOutput();
                    std::cout << "- After cleaning: " << processedPolyData->GetNumberOfPoints() << " points" << std::endl;
                }

                // 2. Triangulate polygons
                if (options.enableTriangulation) {
                    std::cout << "Applying triangulation..." << std::endl;
                    vtkSmartPointer<vtkTriangleFilter> triangulator = vtkSmartPointer<vtkTriangleFilter>::New();
                    triangulator->SetInputData(processedPolyData);
                    triangulator->Update();
                    processedPolyData = triangulator->GetOutput();
                    std::cout << "- After triangulation: " << processedPolyData->GetNumberOfCells() << " triangles" << std::endl;
                }

                // 3. Decimate mesh
                if (options.enableDecimation) {
                    std::cout << "Applying mesh decimation..." << std::endl;
                    vtkSmartPointer<vtkDecimatePro> decimator = vtkSmartPointer<vtkDecimatePro>::New();
                    decimator->SetInputData(processedPolyData);
                    decimator->SetTargetReduction(options.decimationTarget);
                    decimator->SetPreserveTopology(options.preserveTopology);
                    decimator->Update();
                    processedPolyData = decimator->GetOutput();
                    std::cout << "- After decimation: " << processedPolyData->GetNumberOfCells() << " cells" << std::endl;
                }

                // 4. Smooth mesh
                if (options.enableSmoothing) {
                    std::cout << "Applying mesh smoothing..." << std::endl;
                    vtkSmartPointer<vtkSmoothPolyDataFilter> smoother = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
                    smoother->SetInputData(processedPolyData);
                    smoother->SetNumberOfIterations(options.smoothingIterations);
                    smoother->SetRelaxationFactor(options.smoothingRelaxation);
                    smoother->Update();
                    processedPolyData = smoother->GetOutput();
                    std::cout << "- After smoothing: " << processedPolyData->GetNumberOfPoints() << " points" << std::endl;
                }

                // 5. Compute normals
                if (options.enableNormalComputation) {
                    std::cout << "Computing normals..." << std::endl;
                    vtkSmartPointer<vtkPolyDataNormals> normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New();
                    normalGenerator->SetInputData(processedPolyData);
                    normalGenerator->ComputeCellNormalsOn();
                    normalGenerator->ComputePointNormalsOn();
                    normalGenerator->Update();
                    processedPolyData = normalGenerator->GetOutput();
                    std::cout << "- Normals computed successfully" << std::endl;
                }
            }

            // Use processed points if available, otherwise original points
            if (processedPolyData) {
                outputGrid->SetPoints(processedPolyData->GetPoints());
                
                // Copy all cell types from processed polydata
                // Copy vertices
                vtkCellArray* verts = processedPolyData->GetVerts();
                if (verts) {
                    verts->InitTraversal();
                    vtkIdType npts;
                    const vtkIdType* pts;
                    while (verts->GetNextCell(npts, pts)) {
                        outputGrid->InsertNextCell(VTK_VERTEX, npts, const_cast<vtkIdType*>(pts));
                    }
                }
                
                // Copy lines
                vtkCellArray* lines = processedPolyData->GetLines();
                if (lines) {
                    lines->InitTraversal();
                    vtkIdType npts;
                    const vtkIdType* pts;
                    while (lines->GetNextCell(npts, pts)) {
                        if (npts == 2) {
                            outputGrid->InsertNextCell(VTK_LINE, npts, const_cast<vtkIdType*>(pts));
                        } else {
                            outputGrid->InsertNextCell(VTK_POLY_LINE, npts, const_cast<vtkIdType*>(pts));
                        }
                    }
                }
                
                // Copy polygons
                vtkCellArray* polys = processedPolyData->GetPolys();
                if (polys) {
                    polys->InitTraversal();
                    vtkIdType npts;
                    const vtkIdType* pts;
                    while (polys->GetNextCell(npts, pts)) {
                        if (npts == 3) {
                            outputGrid->InsertNextCell(VTK_TRIANGLE, npts, const_cast<vtkIdType*>(pts));
                        } else if (npts == 4) {
                            outputGrid->InsertNextCell(VTK_QUAD, npts, const_cast<vtkIdType*>(pts));
                        } else if (npts > 4) {
                            outputGrid->InsertNextCell(VTK_POLYGON, npts, const_cast<vtkIdType*>(pts));
                        }
                    }
                }
                
                // Copy triangle strips
                vtkCellArray* strips = processedPolyData->GetStrips();
                if (strips) {
                    strips->InitTraversal();
                    vtkIdType npts;
                    const vtkIdType* pts;
                    while (strips->GetNextCell(npts, pts)) {
                        outputGrid->InsertNextCell(VTK_TRIANGLE_STRIP, npts, const_cast<vtkIdType*>(pts));
                    }
                }
                
                // Copy cell data from processed polydata to output grid
                outputGrid->GetCellData()->DeepCopy(processedPolyData->GetCellData());
                // Copy point data from processed polydata to output grid
                outputGrid->GetPointData()->DeepCopy(processedPolyData->GetPointData());
            } else {
                // Copy all cells from input grid
                for (vtkIdType i = 0; i < inputGrid->GetNumberOfCells(); ++i) {
                    vtkCell* cell = inputGrid->GetCell(i);
                    int cellType = cell->GetCellType();
                    vtkIdType npts = cell->GetNumberOfPoints();
                    vtkIdType* pts = cell->GetPointIds()->GetPointer(0);
                    
                    switch (cellType) {
                        case VTK_TRIANGLE:
                            outputGrid->InsertNextCell(VTK_TRIANGLE, npts, pts);
                            break;
                        case VTK_QUAD:
                            outputGrid->InsertNextCell(VTK_QUAD, npts, pts);
                            break;
                        case VTK_TETRA:
                            outputGrid->InsertNextCell(VTK_TETRA, npts, pts);
                            break;
                        case VTK_HEXAHEDRON:
                            outputGrid->InsertNextCell(VTK_HEXAHEDRON, npts, pts);
                            break;
                        case VTK_WEDGE:
                            outputGrid->InsertNextCell(VTK_WEDGE, npts, pts);
                            break;
                        case VTK_PYRAMID:
                            outputGrid->InsertNextCell(VTK_PYRAMID, npts, pts);
                            break;
                        case VTK_LINE:
                            outputGrid->InsertNextCell(VTK_LINE, npts, pts);
                            break;
                        case VTK_VERTEX:
                            outputGrid->InsertNextCell(VTK_VERTEX, npts, pts);
                            break;
                        case VTK_POLYGON:
                            outputGrid->InsertNextCell(VTK_POLYGON, npts, pts);
                            break;
                        case VTK_TRIANGLE_STRIP:
                            outputGrid->InsertNextCell(VTK_TRIANGLE_STRIP, npts, pts);
                            break;
                        default:
                            outputGrid->InsertNextCell(cellType, npts, pts);
                            break;
                    }
                }
                
                outputGrid->SetPoints(inputGrid->GetPoints());
                
                // Copy cell data from input grid to output grid
                outputGrid->GetCellData()->DeepCopy(inputGrid->GetCellData());
                // Copy point data from input grid to output grid
                outputGrid->GetPointData()->DeepCopy(inputGrid->GetPointData());
            }
        }

        // Validate output
        if (outputGrid->GetNumberOfPoints() == 0 || outputGrid->GetNumberOfCells() == 0) {
            errorCode = MeshErrorCode::MESH_EMPTY;
            errorMsg = "Processing resulted in empty mesh";
            std::cerr << "Processing error: " << errorMsg << std::endl;
            return false;
        }

        std::cout << "Processing completed successfully" << std::endl;
        std::cout << "- Output points: " << outputGrid->GetNumberOfPoints() << std::endl;
        std::cout << "- Output cells: " << outputGrid->GetNumberOfCells() << std::endl;
        std::cout << "- Output cell data arrays: " << outputGrid->GetCellData()->GetNumberOfArrays() << std::endl;
        for (int i = 0; i < outputGrid->GetCellData()->GetNumberOfArrays(); ++i) {
            vtkDataArray* array = outputGrid->GetCellData()->GetArray(i);
            std::cout << "  - " << array->GetName() << " (" << array->GetNumberOfComponents() << " components, " << array->GetNumberOfTuples() << " tuples)" << std::endl;
        }

        return true;
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = std::string("Error processing VTK data: ") + e.what();
        std::cerr << "Processing error: " << errorMsg << std::endl;
        return false;
    }
}

/**
 * @brief Convert VTK data to target format
 * @param vtkGrid Input VTK unstructured grid
 * @param dstFilePath Destination file path
 * @param dstFormat Target format
 * @param writeOptions Write options
 * @param errorCode Output error code
 * @param errorMsg Output error message
 * @return Whether conversion is successful
 */
bool VTKConverter::convertFromVTK(const vtkSmartPointer<vtkUnstructuredGrid>& vtkGrid, 
                                   const std::string& dstFilePath, 
                                   MeshFormat dstFormat, 
                                   const FormatWriteOptions& writeOptions, 
                                   MeshErrorCode& errorCode, 
                                   std::string& errorMsg) {
    try {
        std::cout << "Converting to target format: " << dstFilePath << std::endl;
        std::cout << "- Input cell data arrays: " << vtkGrid->GetCellData()->GetNumberOfArrays() << std::endl;
        for (int i = 0; i < vtkGrid->GetCellData()->GetNumberOfArrays(); ++i) {
            vtkDataArray* array = vtkGrid->GetCellData()->GetArray(i);
            std::cout << "  - " << array->GetName() << " (" << array->GetNumberOfComponents() << " components, " << array->GetNumberOfTuples() << " tuples)" << std::endl;
        }

        // Convert to polydata for certain formats
        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->SetPoints(vtkGrid->GetPoints());
        
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        for (vtkIdType i = 0; i < vtkGrid->GetNumberOfCells(); ++i) {
            vtkCell* cell = vtkGrid->GetCell(i);
            int cellType = cell->GetCellType();
            
            if (cellType == VTK_TRIANGLE || cellType == VTK_QUAD) {
                vtkIdType npts = cell->GetNumberOfPoints();
                vtkIdType* pts = cell->GetPointIds()->GetPointer(0);
                cells->InsertNextCell(npts, pts);
            }
        }
        polyData->SetPolys(cells);

        // Handle specific formats using VTK writers
        switch (dstFormat) {
            // case MeshFormat::GLTF:
            //     // Use vtkGLTFWriter for GLB format
            //     {
            //         vtkSmartPointer<vtkGLTFWriter> writer = vtkSmartPointer<vtkGLTFWriter>::New();
            //         writer->SetInputData(polyData);
            //         writer->SetFileName(dstFilePath.c_str());
            //         writer->Update();
            //     }
            //     std::cout << "Successfully wrote GLB format" << std::endl;
            //     return true;
                
            // case MeshFormat::USDZ:
            //     // Use vtkUSDLWriter for USDZ format
            //     {
            //         vtkSmartPointer<vtkUSDLWriter> writer = vtkSmartPointer<vtkUSDLWriter>::New();
            //         writer->SetInputData(polyData);
            //         writer->SetFileName(dstFilePath.c_str());
            //         writer->Update();
            //     }
            //     std::cout << "Successfully wrote USDZ format" << std::endl;
            //     return true;
                
            // case MeshFormat::STEP:
            //     // Use vtkSTEPWriter for STEP format
            //     {
            //         vtkSmartPointer<vtkSTEPWriter> writer = vtkSmartPointer<vtkSTEPWriter>::New();
            //         writer->SetInputData(polyData);
            //         writer->SetFileName(dstFilePath.c_str());
            //         writer->Update();
            //     }
            //     std::cout << "Successfully wrote STEP format" << std::endl;
            //     return true;
                
            case MeshFormat::VTK_LEGACY:
                {
                    // Check if the input is primarily surface cells (triangles/quads) - if yes, use POLYDATA
                    bool isSurfaceOnly = true;
                    for (vtkIdType i = 0; i < vtkGrid->GetNumberOfCells(); ++i) {
                        int cellType = vtkGrid->GetCell(i)->GetCellType();
                        if (cellType != VTK_TRIANGLE && cellType != VTK_QUAD && cellType != VTK_POLYGON && 
                            cellType != VTK_TRIANGLE_STRIP && cellType != VTK_VERTEX && cellType != VTK_LINE) {
                            isSurfaceOnly = false;
                            break;
                        }
                    }
                    
                    if (isSurfaceOnly) {
                        // Use vtkPolyDataWriter for POLYDATA format (best for surface meshes like PLY)
                        std::cout << "- Detected surface-only mesh, writing as POLYDATA" << std::endl;
                        std::cout << "- Writing VTK POLYDATA file with CellData: " << vtkGrid->GetCellData()->GetNumberOfArrays() << " arrays" << std::endl;
                        for (int i = 0; i < vtkGrid->GetCellData()->GetNumberOfArrays(); ++i) {
                            vtkDataArray* array = vtkGrid->GetCellData()->GetArray(i);
                            std::cout << "  - " << array->GetName() << ": " << array->GetNumberOfTuples() << " tuples" << std::endl;
                        }
                        
                        // Convert vtkUnstructuredGrid to vtkPolyData
                        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
                        polyData->SetPoints(vtkGrid->GetPoints());
                        
                        vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
                        vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
                        vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
                        vtkSmartPointer<vtkCellArray> strips = vtkSmartPointer<vtkCellArray>::New();
                        
                        for (vtkIdType i = 0; i < vtkGrid->GetNumberOfCells(); ++i) {
                            vtkCell* cell = vtkGrid->GetCell(i);
                            int cellType = cell->GetCellType();
                            vtkIdType npts = cell->GetNumberOfPoints();
                            vtkIdType* pts = cell->GetPointIds()->GetPointer(0);
                            
                            switch (cellType) {
                                case VTK_TRIANGLE:
                                case VTK_QUAD:
                                case VTK_POLYGON:
                                    polys->InsertNextCell(npts, pts);
                                    break;
                                case VTK_VERTEX:
                                    verts->InsertNextCell(npts, pts);
                                    break;
                                case VTK_LINE:
                                    lines->InsertNextCell(npts, pts);
                                    break;
                                case VTK_TRIANGLE_STRIP:
                                    strips->InsertNextCell(npts, pts);
                                    break;
                                default:
                                    break;
                            }
                        }
                        
                        polyData->SetPolys(polys);
                        polyData->SetVerts(verts);
                        polyData->SetLines(lines);
                        polyData->SetStrips(strips);
                        
                        // Copy PointData and CellData
                        polyData->GetPointData()->DeepCopy(vtkGrid->GetPointData());
                        polyData->GetCellData()->DeepCopy(vtkGrid->GetCellData());
                        
                        // Write using vtkPolyDataWriter
                        vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
                        writer->SetInputData(polyData);
                        writer->SetFileName(dstFilePath.c_str());
                        writer->SetFileTypeToASCII();
                        writer->Update();
                        
                        std::cout << "Successfully wrote Legacy VTK POLYDATA format" << std::endl;
                    } else {
                        // Use vtkDataSetWriter for UNSTRUCTURED_GRID format (for volumetric meshes)
                        std::cout << "- Writing VTK UNSTRUCTURED_GRID file with CellData: " << vtkGrid->GetCellData()->GetNumberOfArrays() << " arrays" << std::endl;
                        for (int i = 0; i < vtkGrid->GetCellData()->GetNumberOfArrays(); ++i) {
                            vtkDataArray* array = vtkGrid->GetCellData()->GetArray(i);
                            std::cout << "  - " << array->GetName() << ": " << array->GetNumberOfTuples() << " tuples" << std::endl;
                        }
                        
                        vtkSmartPointer<vtkDataSetWriter> writer = vtkSmartPointer<vtkDataSetWriter>::New();
                        writer->SetInputData(vtkGrid);
                        writer->SetFileName(dstFilePath.c_str());
                        writer->SetFileTypeToASCII();
                        writer->Update();
                        
                        std::cout << "Successfully wrote Legacy VTK UNSTRUCTURED_GRID format" << std::endl;
                    }
                    return true;
                }
                
            case MeshFormat::VTK_XML:
                // Use vtkXMLUnstructuredGridWriter for XML VTK format
                {
                    std::cout << "- Converting VTK to VTU format" << std::endl;
                    std::cout << "- Input point data arrays: " << vtkGrid->GetPointData()->GetNumberOfArrays() << std::endl;
                    for (int i = 0; i < vtkGrid->GetPointData()->GetNumberOfArrays(); ++i) {
                        vtkDataArray* array = vtkGrid->GetPointData()->GetArray(i);
                        std::cout << "  - Point data: " << array->GetName() 
                                  << " (" << array->GetNumberOfComponents() << " components, " 
                                  << array->GetNumberOfTuples() << " tuples)" << std::endl;
                    }
                    std::cout << "- Input cell data arrays: " << vtkGrid->GetCellData()->GetNumberOfArrays() << std::endl;
                    for (int i = 0; i < vtkGrid->GetCellData()->GetNumberOfArrays(); ++i) {
                        vtkDataArray* array = vtkGrid->GetCellData()->GetArray(i);
                        std::cout << "  - Cell data: " << array->GetName() 
                                  << " (" << array->GetNumberOfComponents() << " components, " 
                                  << array->GetNumberOfTuples() << " tuples)" << std::endl;
                    }
                    
                    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
                    writer->SetInputData(vtkGrid);
                    writer->SetFileName(dstFilePath.c_str());
                    writer->SetDataModeToBinary();
                    writer->SetCompressorTypeToZLib();
                    writer->SetEncodeAppendedData(1);
                    writer->Update();
                }
                std::cout << "Successfully wrote XML VTK format" << std::endl;
                return true;
                
            case MeshFormat::CGNS:
#ifdef HAVE_CGNS
                {
                    // Check if the mesh contains volumetric cells
                    bool hasVolumetricCells = false;
                    int surfaceCellCount = 0;
                    int volumetricCellCount = 0;
                    
                    vtkIdType numCells = vtkGrid->GetNumberOfCells();
                    for (vtkIdType i = 0; i < numCells; ++i) {
                        vtkCell* cell = vtkGrid->GetCell(i);
                        int cellType = cell->GetCellType();
                        
                        switch (cellType) {
                            case VTK_TETRA:
                            case VTK_HEXAHEDRON:
                            case VTK_WEDGE:
                            case VTK_PYRAMID:
                                hasVolumetricCells = true;
                                volumetricCellCount++;
                                break;
                            case VTK_TRIANGLE:
                            case VTK_QUAD:
                            case VTK_POLYGON:
                            case VTK_TRIANGLE_STRIP:
                                surfaceCellCount++;
                                break;
                        }
                    }
                    
                    // If only surface cells are present, reject conversion
                    if (!hasVolumetricCells && surfaceCellCount > 0) {
                        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
                        errorMsg = "CGNS format conversion is only supported for volumetric meshes. "
                                   "The input mesh contains only surface cells (" + 
                                   std::to_string(surfaceCellCount) + 
                                   " surface cells, 0 volumetric cells). "
                                   "CGNS is primarily designed for CFD volumetric meshes.";
                        std::cerr << "Conversion error: " << errorMsg << std::endl;
                        return false;
                    }
                    
                    // If mesh is empty, reject conversion
                    if (surfaceCellCount == 0 && volumetricCellCount == 0) {
                        errorCode = MeshErrorCode::MESH_EMPTY;
                        errorMsg = "Mesh is empty, cannot convert to CGNS format";
                        return false;
                    }
                    
                    std::cout << "- Mesh analysis: " << volumetricCellCount << " volumetric cells, " 
                              << surfaceCellCount << " surface cells" << std::endl;
                    
                    int cgnsFile = -1;
                    
                    // Set file type to ADF for better compatibility with older CGNS libraries
                    // CG_FILE_ADF = 1 (older, more compatible format)
                    // CG_FILE_HDF5 = 2 (newer HDF5-based format)
                    // CG_FILE_ADF2 = 3 (ADF version 2)
                    int ier = cg_set_file_type(CG_FILE_ADF);
                    if (ier != CG_OK) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to set CGNS file type: ") + cg_get_error();
                        return false;
                    }
                    
                    // Open CGNS file for writing
                    ier = cg_open(dstFilePath.c_str(), CG_MODE_WRITE, &cgnsFile);
                    if (ier != CG_OK) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to open CGNS file for writing: ") + cg_get_error();
                        return false;
                    }
                    
                    // Create base
                    int baseIndex;
                    ier = cg_base_write(cgnsFile, "Base", 3, 3, &baseIndex);
                    if (ier != CG_OK) {
                        cg_close(cgnsFile);
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to create CGNS base: ") + cg_get_error();
                        return false;
                    }
                    
                    // Prepare zone data
                    vtkIdType numPoints = vtkGrid->GetNumberOfPoints();
                    
                    // Zone size: [vertex size, cell size, boundary vertex size]
                    cgsize_t zoneSize[3];
                    zoneSize[0] = static_cast<cgsize_t>(numPoints);
                    zoneSize[1] = static_cast<cgsize_t>(numCells);
                    zoneSize[2] = 0;
                    
                    // Create zone
                    int zoneIndex;
                    ier = cg_zone_write(cgnsFile, baseIndex, "Zone", zoneSize, Unstructured, &zoneIndex);
                    if (ier != CG_OK) {
                        cg_close(cgnsFile);
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to create CGNS zone: ") + cg_get_error();
                        return false;
                    }
                    
                    // Write coordinates
                    std::vector<double> xCoords(numPoints);
                    std::vector<double> yCoords(numPoints);
                    std::vector<double> zCoords(numPoints);
                    
                    for (vtkIdType i = 0; i < numPoints; ++i) {
                        double point[3];
                        vtkGrid->GetPoint(i, point);
                        xCoords[i] = point[0];
                        yCoords[i] = point[1];
                        zCoords[i] = point[2];
                    }
                    
                    int coordIndex;
                    ier = cg_coord_write(cgnsFile, baseIndex, zoneIndex, RealDouble, "CoordinateX", xCoords.data(), &coordIndex);
                    if (ier == CG_OK) {
                        ier = cg_coord_write(cgnsFile, baseIndex, zoneIndex, RealDouble, "CoordinateY", yCoords.data(), &coordIndex);
                    }
                    if (ier == CG_OK) {
                        ier = cg_coord_write(cgnsFile, baseIndex, zoneIndex, RealDouble, "CoordinateZ", zCoords.data(), &coordIndex);
                    }
                    if (ier != CG_OK) {
                        cg_close(cgnsFile);
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to write CGNS coordinates: ") + cg_get_error();
                        return false;
                    }
                    
                    // Prepare elements
                    std::vector<vtkIdType> tetraIndices;
                    std::vector<vtkIdType> hexaIndices;
                    std::vector<vtkIdType> wedgeIndices;
                    std::vector<vtkIdType> pyramidIndices;
                    std::vector<vtkIdType> triIndices;
                    std::vector<vtkIdType> quadIndices;
                    
                    for (vtkIdType i = 0; i < numCells; ++i) {
                        vtkCell* cell = vtkGrid->GetCell(i);
                        int cellType = cell->GetCellType();
                        vtkIdType npts = cell->GetNumberOfPoints();
                        
                        switch (cellType) {
                            case VTK_TETRA:
                                tetraIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    tetraIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                            case VTK_HEXAHEDRON:
                                hexaIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    hexaIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                            case VTK_WEDGE:
                                wedgeIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    wedgeIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                            case VTK_PYRAMID:
                                pyramidIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    pyramidIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                            case VTK_TRIANGLE:
                                triIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    triIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                            case VTK_QUAD:
                                quadIndices.push_back(i + 1);
                                for (vtkIdType j = 0; j < npts; ++j) {
                                    quadIndices.push_back(cell->GetPointId(j) + 1);
                                }
                                break;
                        }
                    }
                    
                    // Write elements
                    int sectionIndex;
                    if (!tetraIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Tetrahedrons", TETRA_4, 1, 
                            static_cast<int>(tetraIndices.size() / 5), 0, tetraIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write tetrahedrons: ") + cg_get_error();
                            return false;
                        }
                    }
                    if (!hexaIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Hexahedrons", HEXA_8, 1, 
                            static_cast<int>(hexaIndices.size() / 9), 0, hexaIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write hexahedrons: ") + cg_get_error();
                            return false;
                        }
                    }
                    if (!wedgeIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Wedges", PENTA_6, 1, 
                            static_cast<int>(wedgeIndices.size() / 7), 0, wedgeIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write wedges: ") + cg_get_error();
                            return false;
                        }
                    }
                    if (!pyramidIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Pyramids", PYRA_5, 1, 
                            static_cast<int>(pyramidIndices.size() / 6), 0, pyramidIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write pyramids: ") + cg_get_error();
                            return false;
                        }
                    }
                    if (!triIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Triangles", TRI_3, 1, 
                            static_cast<int>(triIndices.size() / 4), 0, triIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write triangles: ") + cg_get_error();
                            return false;
                        }
                    }
                    if (!quadIndices.empty()) {
                        ier = cg_section_write(cgnsFile, baseIndex, zoneIndex, "Quads", QUAD_4, 1, 
                            static_cast<int>(quadIndices.size() / 5), 0, quadIndices.data(), &sectionIndex);
                        if (ier != CG_OK) {
                            cg_close(cgnsFile);
                            errorCode = MeshErrorCode::WRITE_FAILED;
                            errorMsg = std::string("Failed to write quads: ") + cg_get_error();
                            return false;
                        }
                    }
                    
                    // Close CGNS file
                    ier = cg_close(cgnsFile);
                    if (ier != CG_OK) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = std::string("Failed to close CGNS file: ") + cg_get_error();
                        return false;
                    }
                    
                    std::cout << "Successfully wrote CGNS format" << std::endl;
                    return true;
                }
#else
                errorCode = MeshErrorCode::DEPENDENCY_MISSING;
                errorMsg = "CGNS dependency library missing for write operation";
                return false;
#endif
                
            case MeshFormat::OBJ:
                {
                    std::cout << "- Converting VTK to OBJ format using vtkOBJWriter" << std::endl;
                    
                    // Use vtkGeometryFilter to convert UnstructuredGrid to PolyData
                    vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
                    geometryFilter->SetInputData(vtkGrid);
                    geometryFilter->Update();
                    
                    vtkSmartPointer<vtkPolyData> polyData = geometryFilter->GetOutput();
                    
                    if (!polyData || polyData->GetNumberOfPoints() == 0) {
                        errorCode = MeshErrorCode::MESH_EMPTY;
                        errorMsg = "Failed to convert VTK grid to PolyData for OBJ output";
                        std::cerr << "OBJ conversion error: " << errorMsg << std::endl;
                        return false;
                    }
                    
                    std::cout << "- Extracted PolyData: " << polyData->GetNumberOfPoints() << " points, " 
                              << polyData->GetNumberOfCells() << " cells" << std::endl;
                    
                    // Generate normals for better visualization
                    vtkSmartPointer<vtkPolyDataNormals> normalsFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
                    normalsFilter->SetInputData(polyData);
                    normalsFilter->ComputePointNormalsOn();
                    normalsFilter->ComputeCellNormalsOn();
                    normalsFilter->SplittingOff();
                    normalsFilter->Update();
                    
                    polyData = normalsFilter->GetOutput();
                    
                    // Write OBJ file using vtkOBJWriter
                    vtkSmartPointer<vtkOBJWriter> writer = vtkSmartPointer<vtkOBJWriter>::New();
                    writer->SetInputData(polyData);
                    writer->SetFileName(dstFilePath.c_str());
                    writer->Update();
                    
                    std::cout << "Successfully wrote OBJ format" << std::endl;
                    return true;
                }
                
            case MeshFormat::OFF:
                {
                    std::cout << "- Converting VTK to OFF format" << std::endl;
                    
                    // Use vtkGeometryFilter to convert UnstructuredGrid to PolyData
                    vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
                    geometryFilter->SetInputData(vtkGrid);
                    geometryFilter->Update();
                    
                    vtkSmartPointer<vtkPolyData> polyData = geometryFilter->GetOutput();
                    
                    if (!polyData || polyData->GetNumberOfPoints() == 0) {
                        errorCode = MeshErrorCode::MESH_EMPTY;
                        errorMsg = "Failed to convert VTK grid to PolyData for OFF output";
                        std::cerr << "OFF conversion error: " << errorMsg << std::endl;
                        return false;
                    }
                    
                    std::cout << "- Extracted PolyData: " << polyData->GetNumberOfPoints() << " points, " 
                              << polyData->GetNumberOfCells() << " cells" << std::endl;
                    
                    // Write OFF file
                    std::ofstream file(dstFilePath);
                    if (!file.is_open()) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = "Failed to open OFF file for writing";
                        return false;
                    }
                    
                    vtkIdType numPoints = polyData->GetNumberOfPoints();
                    vtkIdType numCells = polyData->GetNumberOfCells();
                    
                    // Write OFF header
                    file << "OFF" << std::endl;
                    file << numPoints << " " << numCells << " 0" << std::endl;
                    
                    // Write vertices
                    for (vtkIdType i = 0; i < numPoints; ++i) {
                        double point[3];
                        polyData->GetPoint(i, point);
                        file << point[0] << " " << point[1] << " " << point[2] << std::endl;
                    }
                    
                    // Write faces
                    for (vtkIdType i = 0; i < numCells; ++i) {
                        vtkCell* cell = polyData->GetCell(i);
                        vtkIdType npts = cell->GetNumberOfPoints();
                        file << npts;
                        for (vtkIdType j = 0; j < npts; ++j) {
                            file << " " << cell->GetPointId(j);
                        }
                        file << std::endl;
                    }
                    
                    file.close();
                    
                    std::cout << "Successfully wrote OFF format" << std::endl;
                    return true;
                }
                
            case MeshFormat::PLY_ASCII:
            case MeshFormat::PLY_BINARY:
                {
                    std::cout << "- Converting VTK to PLY format using vtkPLYWriter" << std::endl;
                    
                    // Use vtkGeometryFilter to convert UnstructuredGrid to PolyData
                    vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
                    geometryFilter->SetInputData(vtkGrid);
                    geometryFilter->Update();
                    
                    vtkSmartPointer<vtkPolyData> polyData = geometryFilter->GetOutput();
                    
                    if (!polyData || polyData->GetNumberOfPoints() == 0) {
                        errorCode = MeshErrorCode::MESH_EMPTY;
                        errorMsg = "Failed to convert VTK grid to PolyData for PLY output";
                        std::cerr << "PLY conversion error: " << errorMsg << std::endl;
                        return false;
                    }
                    
                    std::cout << "- Extracted PolyData: " << polyData->GetNumberOfPoints() << " points, " 
                              << polyData->GetNumberOfCells() << " cells" << std::endl;
                    
                    // Write PLY file
                    vtkSmartPointer<vtkPLYWriter> writer = vtkSmartPointer<vtkPLYWriter>::New();
                    writer->SetInputData(polyData);
                    writer->SetFileName(dstFilePath.c_str());
                    if (dstFormat == MeshFormat::PLY_BINARY) {
                        writer->SetFileTypeToBinary();
                    } else {
                        writer->SetFileTypeToASCII();
                    }
                    writer->Update();
                    
                    std::cout << "Successfully wrote PLY format" << std::endl;
                    return true;
                }
                
            case MeshFormat::STL_ASCII:
            case MeshFormat::STL_BINARY:
                {
                    std::cout << "- Converting VTK to STL format using vtkSTLWriter" << std::endl;
                    
                    // Use vtkGeometryFilter to convert UnstructuredGrid to PolyData
                    vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
                    geometryFilter->SetInputData(vtkGrid);
                    geometryFilter->Update();
                    
                    vtkSmartPointer<vtkPolyData> polyData = geometryFilter->GetOutput();
                    
                    if (!polyData || polyData->GetNumberOfPoints() == 0) {
                        errorCode = MeshErrorCode::MESH_EMPTY;
                        errorMsg = "Failed to convert VTK grid to PolyData for STL output";
                        std::cerr << "STL conversion error: " << errorMsg << std::endl;
                        return false;
                    }
                    
                    std::cout << "- Extracted PolyData: " << polyData->GetNumberOfPoints() << " points, " 
                              << polyData->GetNumberOfCells() << " cells" << std::endl;
                    
                    // Write STL file
                    vtkSmartPointer<vtkSTLWriter> writer = vtkSmartPointer<vtkSTLWriter>::New();
                    writer->SetInputData(polyData);
                    writer->SetFileName(dstFilePath.c_str());
                    if (dstFormat == MeshFormat::STL_BINARY) {
                        writer->SetFileTypeToBinary();
                    } else {
                        writer->SetFileTypeToASCII();
                    }
                    writer->Update();
                    
                    std::cout << "Successfully wrote STL format" << std::endl;
                    return true;
                }
                
            case MeshFormat::GMSH_V2:
            case MeshFormat::GMSH_V4:
#ifdef HAVE_GMSH
                {
                    std::cout << "- Converting VTK to Gmsh format using Gmsh API" << std::endl;
                    
                    int ierr = 0;
                    
                    // Initialize Gmsh
                    gmshInitialize(0, nullptr, 0, 0, &ierr);
                    if (ierr != 0) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = "Failed to initialize Gmsh";
                        return false;
                    }
                    
                    gmshOptionSetNumber("General.Terminal", 0, &ierr);
                    
                    // Create a new model
                    gmshModelAdd("vtk_to_gmsh", &ierr);
                    
                    // Add points
                    vtkIdType numPoints = vtkGrid->GetNumberOfPoints();
                    std::vector<size_t> gmshPointTags;
                    gmshPointTags.reserve(numPoints);
                    std::vector<double> gmshPoints;
                    gmshPoints.reserve(numPoints * 3);
                    
                    for (vtkIdType i = 0; i < numPoints; ++i) {
                        double point[3];
                        vtkGrid->GetPoint(i, point);
                        gmshPointTags.push_back(i + 1);
                        gmshPoints.push_back(point[0]);
                        gmshPoints.push_back(point[1]);
                        gmshPoints.push_back(point[2]);
                    }
                    
                    // Add cells by type
                    vtkIdType numCells = vtkGrid->GetNumberOfCells();
                    
                    // Group cells by type
                    std::map<int, std::vector<std::vector<size_t>>> cellsByType;
                    
                    for (vtkIdType i = 0; i < numCells; ++i) {
                        vtkCell* cell = vtkGrid->GetCell(i);
                        int cellType = cell->GetCellType();
                        vtkIdType npts = cell->GetNumberOfPoints();
                        
                        std::vector<size_t> pointTags;
                        pointTags.reserve(npts);
                        
                        for (vtkIdType j = 0; j < npts; ++j) {
                            pointTags.push_back(gmshPointTags[cell->GetPointId(j)]);
                        }
                        
                        // Convert VTK cell type to Gmsh element type
                        int gmshType = 1; // Default to line
                        switch (cellType) {
                            case VTK_TRIANGLE:
                                gmshType = 2;
                                break;
                            case VTK_QUAD:
                                gmshType = 3;
                                break;
                            case VTK_TETRA:
                                gmshType = 4;
                                break;
                            case VTK_HEXAHEDRON:
                                gmshType = 5;
                                break;
                            case VTK_WEDGE:
                                gmshType = 6;
                                break;
                            case VTK_PYRAMID:
                                gmshType = 7;
                                break;
                            case VTK_VERTEX:
                                gmshType = 15;
                                break;
                        }
                        
                        cellsByType[gmshType].push_back(pointTags);
                    }
                    
                    // Add a discrete volume entity
                    int discreteTag = gmshModelAddDiscreteEntity(3, -1, nullptr, 0, &ierr);
                    
                    // Add nodes
                    gmshModelMeshAddNodes(3, discreteTag, gmshPointTags.data(), gmshPointTags.size(),
                                         gmshPoints.data(), gmshPoints.size(),
                                         nullptr, 0, &ierr);
                    
                    // Add elements by type
                    for (const auto& entry : cellsByType) {
                        int gmshType = entry.first;
                        const auto& elements = entry.second;
                        
                        std::vector<size_t> elementTags;
                        std::vector<size_t> nodeTags;
                        
                        elementTags.reserve(elements.size());
                        nodeTags.reserve(elements.size() * elements[0].size());
                        
                        for (size_t i = 0; i < elements.size(); ++i) {
                            elementTags.push_back(i + 1);
                            for (size_t nodeTag : elements[i]) {
                                nodeTags.push_back(nodeTag);
                            }
                        }
                        
                        gmshModelMeshAddElementsByType(discreteTag, gmshType, elementTags.data(), elementTags.size(),
                                                       nodeTags.data(), nodeTags.size(), &ierr);
                    }
                    
                    // Write file
                    gmshWrite(dstFilePath.c_str(), &ierr);
                    
                    // Finalize Gmsh
                    gmshFinalize(&ierr);
                    
                    if (ierr != 0) {
                        errorCode = MeshErrorCode::WRITE_FAILED;
                        errorMsg = "Error writing Gmsh file";
                        return false;
                    }
                    
                    std::cout << "Successfully wrote Gmsh format" << std::endl;
                    return true;
                }
#else
                errorCode = MeshErrorCode::DEPENDENCY_MISSING;
                errorMsg = "Gmsh dependency library missing for write operation";
                return false;
#endif
                
            default:
                // For other formats, use existing MeshWriter
                MeshData meshData;
                bool success = vtkToMeshData(vtkGrid, meshData, errorCode, errorMsg);
                if (!success) {
                    return false;
                }
                success = MeshWriter::write(meshData, dstFilePath, dstFormat, writeOptions, errorCode, errorMsg);
                if (success) {
                    std::cout << "Successfully wrote target format" << std::endl;
                }
                return success;
        }
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::WRITE_FAILED;
        errorMsg = std::string("Error writing target format: ") + e.what();
        std::cerr << "Writing error: " << errorMsg << std::endl;
        return false;
    }
}

/**
 * @brief Complete conversion workflow: source → VTK → processing → target
 * @param srcFilePath Source file path
 * @param dstFilePath Destination file path
 * @param srcFormat Source format
 * @param dstFormat Target format
 * @param processingOptions VTK processing options
 * @param writeOptions Write options
 * @param errorCode Output error code
 * @param errorMsg Output error message
 * @return Whether conversion is successful
 */
bool VTKConverter::convert(const std::string& srcFilePath, 
                           const std::string& dstFilePath, 
                           MeshFormat srcFormat, 
                           MeshFormat dstFormat, 
                           const VTKProcessingOptions& processingOptions, 
                           const FormatWriteOptions& writeOptions, 
                           MeshErrorCode& errorCode, 
                           std::string& errorMsg) {
    std::cout << "=== VTK-based 3D Model Conversion ===" << std::endl;
    std::cout << "Source: " << srcFilePath << std::endl;
    std::cout << "Destination: " << dstFilePath << std::endl;

    // Step 1: Validate input file
    std::cout << "\nStep 1: Validating input file..." << std::endl;
    if (!fileExists(srcFilePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "Source file does not exist: " + srcFilePath;
        std::cerr << "Validation error: " << errorMsg << std::endl;
        return false;
    }

    // Step 2: Convert source format to VTK
    std::cout << "\nStep 2: Converting source format to VTK..." << std::endl;
    vtkSmartPointer<vtkUnstructuredGrid> vtkGrid;
    bool success = convertToVTK(srcFilePath, vtkGrid, errorCode, errorMsg);
    if (!success) {
        std::cerr << "Conversion to VTK failed: " << errorMsg << std::endl;
        return false;
    }

    // Step 3: Process and optimize VTK data
    std::cout << "\nStep 3: Processing and optimizing VTK data..." << std::endl;
    vtkSmartPointer<vtkUnstructuredGrid> processedGrid;
    success = processVTKData(vtkGrid, processingOptions, processedGrid, errorCode, errorMsg);
    if (!success) {
        std::cerr << "VTK processing failed: " << errorMsg << std::endl;
        return false;
    }

    // Step 4: Convert VTK to target format
    std::cout << "\nStep 4: Converting VTK data to target format..." << std::endl;
    success = convertFromVTK(processedGrid, dstFilePath, dstFormat, writeOptions, errorCode, errorMsg);
    if (!success) {
        std::cerr << "Conversion to target format failed: " << errorMsg << std::endl;
        return false;
    }

    // Step 5: Validate output file
    std::cout << "\nStep 5: Validating output file..." << std::endl;
    if (!fileExists(dstFilePath)) {
        errorCode = MeshErrorCode::WRITE_FAILED;
        errorMsg = "Output file was not created: " + dstFilePath;
        std::cerr << "Validation error: " << errorMsg << std::endl;
        return false;
    }

    std::cout << "\n=== Conversion Completed Successfully ===" << std::endl;
    std::cout << "Source: " << srcFilePath << std::endl;
    std::cout << "Destination: " << dstFilePath << std::endl;
    std::cout << "Conversion successful!" << std::endl;

    return true;
}

/**
 * @brief Convert vtkUnstructuredGrid to MeshData
 * @param grid Input VTK unstructured grid
 * @param[out] meshData Output mesh data
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether conversion is successful
 */
bool VTKConverter::vtkToMeshData(const vtkSmartPointer<vtkUnstructuredGrid>& grid, 
                                  MeshData& meshData, 
                                  MeshErrorCode& errorCode, 
                                  std::string& errorMsg) {
    try {
        meshData.clear();
        
        // Convert points
        vtkIdType numPoints = grid->GetNumberOfPoints();
        meshData.points.reserve(numPoints * 3);
        
        for (vtkIdType i = 0; i < numPoints; ++i) {
            double point[3];
            grid->GetPoint(i, point);
            meshData.points.push_back(static_cast<float>(point[0]));
            meshData.points.push_back(static_cast<float>(point[1]));
            meshData.points.push_back(static_cast<float>(point[2]));
        }
        
        // Convert cells
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
        
        // Cell Data
        vtkCellData* cellData = grid->GetCellData();
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
        
        // Calculate metadata
        meshData.calculateMetadata();
        
        return true;
    } catch (const std::exception& e) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = std::string("Error converting VTK to MeshData: ") + e.what();
        std::cerr << "VTK to MeshData conversion error: " << errorMsg << std::endl;
        return false;
    }
}
