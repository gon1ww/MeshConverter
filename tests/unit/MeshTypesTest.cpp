#include <gtest/gtest.h>
#include "MeshTypes.h"

/**
 * @brief 测试MeshData类的基本操作
 */
TEST(MeshTypesTest, MeshDataBasicOperations) {
    MeshData meshData;

    // 测试isEmpty
    EXPECT_TRUE(meshData.isEmpty());

    // 添加一些数据
    meshData.points = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    meshData.cells.push_back({VtkCellType::LINE, {0, 1}});

    // 测试isEmpty
    EXPECT_FALSE(meshData.isEmpty());

    // 测试calculateMetadata
    meshData.calculateMetadata();
    EXPECT_EQ(meshData.metadata.pointCount, 2);
    EXPECT_EQ(meshData.metadata.cellCount, 1);
    EXPECT_EQ(meshData.metadata.cellTypeCount[VtkCellType::LINE], 1);

    // 测试clear
    meshData.clear();
    EXPECT_TRUE(meshData.isEmpty());
    EXPECT_EQ(meshData.metadata.pointCount, 0);
    EXPECT_EQ(meshData.metadata.cellCount, 0);
}

/**
 * @brief 测试网格类型判断
 */
TEST(MeshTypesTest, MeshTypeDetection) {
    MeshData volumeMesh;
    volumeMesh.cells.push_back({VtkCellType::TETRA, {0, 1, 2, 3}});
    volumeMesh.calculateMetadata();
    EXPECT_EQ(volumeMesh.metadata.meshType, MeshType::VOLUME_MESH);

    MeshData surfaceMesh;
    surfaceMesh.cells.push_back({VtkCellType::TRIANGLE, {0, 1, 2}});
    surfaceMesh.calculateMetadata();
    EXPECT_EQ(surfaceMesh.metadata.meshType, MeshType::SURFACE_MESH);
}

/**
 * @brief 测试属性数据处理
 */
TEST(MeshTypesTest, AttributeData) {
    MeshData meshData;
    meshData.points = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    meshData.cells.push_back({VtkCellType::LINE, {0, 1}});

    // 添加点属性
    meshData.pointData["pressure"] = {1.0f, 2.0f};
    meshData.pointData["velocity"] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    // 添加单元属性
    meshData.cellData["jacobian"] = {1.0f};

    // 计算元数据
    meshData.calculateMetadata();

    // 检查属性名称是否正确提取
    EXPECT_EQ(meshData.metadata.pointDataNames.size(), 2);
    EXPECT_EQ(meshData.metadata.cellDataNames.size(), 1);
}
