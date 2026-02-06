#include "transform.h"
#include "ui_transform.h"

#include <QAction>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QItemSelection>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>



#include <cmath>

#include <vtkAppendFilter.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDataSetWriter.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkOBJReader.h>
#include <vtkOBJWriter.h>
#include <vtkOFFReader.h>
#include <vtkPLYReader.h>
#include <vtkPLYWriter.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

// Include MeshReader from src directory
#include "MeshReader.h"
#include "MeshTypes.h"
#include "MeshException.h"
#include "VTKConverter.h"
#include "MeshHelper.h"

#ifdef HAS_VTK_IOCGNS
#include <vtkCGNSReader.h>
#if __has_include(<vtkCGNSWriter.h>)
#include <vtkCGNSWriter.h>
#define HAS_VTK_CGNS_WRITER
#endif
#endif

#ifdef HAS_VTK_GMSH
#include <vtkGmshReader.h>
#if __has_include(<vtkGmshWriter.h>)
#include <vtkGmshWriter.h>
#define HAS_VTK_GMSH_WRITER
#endif
#endif

const QSet<QString> kSupportedExtensions = {"vtk", "vtu", "cgns", "msh", "obj", "off", "stl", "ply"};

struct ExportResult {
    bool ok = false;
    QString message;
};

vtkSmartPointer<vtkDataSet> extractFirstDataSet(vtkMultiBlockDataSet* multiBlock)
{
    if (!multiBlock) {
        return nullptr;
    }

    const unsigned int count = multiBlock->GetNumberOfBlocks();
    for (unsigned int i = 0; i < count; ++i) {
        auto* block = multiBlock->GetBlock(i);
        auto* dataSet = vtkDataSet::SafeDownCast(block);
        if (dataSet) {
            return dataSet;
        }
    }

    return nullptr;
}

vtkSmartPointer<vtkDataSet> loadMeshDataSet(const QString& filePath, QString* errorMessage)
{
    // 使用MeshReader::readAuto方法读取网格文件
    const std::string filePathStd = filePath.toUtf8().toStdString();
    MeshData meshData;
    MeshErrorCode errorCode;
    std::string errorMsg;
    
    bool success = MeshReader::readAuto(filePathStd, meshData, errorCode, errorMsg);
    if (!success) {
        if (errorMessage) {
            *errorMessage = QString::fromStdString(errorMsg);
        }
        return nullptr;
    }
    
    // 检查网格数据是否为空
    if (meshData.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "网格数据为空";
        }
        return nullptr;
    }
    
    // 将MeshData转换为vtkUnstructuredGrid
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    
    // 设置点数据
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    size_t pointCount = meshData.points.size() / 3;
    for (size_t i = 0; i < pointCount; ++i) {
        float x = meshData.points[i * 3];
        float y = meshData.points[i * 3 + 1];
        float z = meshData.points[i * 3 + 2];
        points->InsertNextPoint(x, y, z);
    }
    grid->SetPoints(points);
    
    // 设置单元数据
    for (const auto& cell : meshData.cells) {
        vtkIdType* pointIds = new vtkIdType[cell.pointIndices.size()];
        for (size_t i = 0; i < cell.pointIndices.size(); ++i) {
            pointIds[i] = cell.pointIndices[i];
        }
        
        // 转换单元类型
        int vtkCellType = 0;
        switch (cell.type) {
        case VtkCellType::VERTEX:
            vtkCellType = VTK_VERTEX;
            break;
        case VtkCellType::LINE:
            vtkCellType = VTK_LINE;
            break;
        case VtkCellType::TRIANGLE:
            vtkCellType = VTK_TRIANGLE;
            break;
        case VtkCellType::QUAD:
            vtkCellType = VTK_QUAD;
            break;
        case VtkCellType::TETRA:
            vtkCellType = VTK_TETRA;
            break;
        case VtkCellType::HEXAHEDRON:
            vtkCellType = VTK_HEXAHEDRON;
            break;
        case VtkCellType::WEDGE:
            vtkCellType = VTK_WEDGE;
            break;
        case VtkCellType::PYRAMID:
            vtkCellType = VTK_PYRAMID;
            break;
        case VtkCellType::TRIANGLE_STRIP:
            vtkCellType = VTK_TRIANGLE_STRIP;
            break;
        case VtkCellType::POLYGON:
            vtkCellType = VTK_POLYGON;
            break;
        default:
            delete[] pointIds;
            continue;
        }
        
        grid->InsertNextCell(vtkCellType, static_cast<int>(cell.pointIndices.size()), pointIds);
        delete[] pointIds;
    }
    
    return grid;
}

vtkSmartPointer<vtkPolyData> toSurfaceMesh(vtkDataSet* dataSet)
{
    if (!dataSet) {
        return nullptr;
    }
    auto surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
    surfaceFilter->SetInputData(dataSet);
    surfaceFilter->Update();
    return surfaceFilter->GetOutput();
}

vtkSmartPointer<vtkUnstructuredGrid> toUnstructuredGrid(vtkDataSet* dataSet)
{
    if (!dataSet) {
        return nullptr;
    }
    if (auto* grid = vtkUnstructuredGrid::SafeDownCast(dataSet)) {
        return grid;
    }
    auto appendFilter = vtkSmartPointer<vtkAppendFilter>::New();
    appendFilter->AddInputData(dataSet);
    appendFilter->Update();
    return appendFilter->GetOutput();
}

ExportResult exportMeshFile(const QString& sourcePath,
                            const QString& outputPath,
                            const QString& formatExt,
                            bool exportSurface,
                            bool binary)
{
    ExportResult result;

    // Convert paths to std::string
    const std::string srcPath = sourcePath.toUtf8().toStdString();
    const std::string dstPath = outputPath.toUtf8().toStdString();
    const std::string ext = formatExt.toLower().toStdString();

    // Detect source format
    MeshFormat srcFormat = MeshHelper::detectFormat(srcPath);
    if (srcFormat == MeshFormat::UNKNOWN) {
        result.message = "无法检测源文件格式";
        return result;
    }

    // Determine target format based on extension
    MeshFormat dstFormat = MeshFormat::UNKNOWN;
    if (ext == "vtk") {
        dstFormat = MeshFormat::VTK_LEGACY;
    } else if (ext == "vtu") {
        dstFormat = MeshFormat::VTK_XML;
    } else if (ext == "cgns") {
        dstFormat = MeshFormat::CGNS;
    } else if (ext == "msh") {
        dstFormat = MeshFormat::GMSH_V4;
    } else if (ext == "obj") {
        dstFormat = MeshFormat::OBJ;
    } else if (ext == "off") {
        dstFormat = MeshFormat::OFF;
    } else if (ext == "stl") {
        dstFormat = binary ? MeshFormat::STL_BINARY : MeshFormat::STL_ASCII;
    } else if (ext == "ply") {
        dstFormat = binary ? MeshFormat::PLY_BINARY : MeshFormat::PLY_ASCII;
    } else {
        result.message = "不支持的目标格式";
        return result;
    }

    // Create processing options
    VTKConverter::VTKProcessingOptions processingOptions;
    if (exportSurface) {
        // Enable surface extraction if needed
        processingOptions.enableTriangulation = true;
    }

    // Create write options
    FormatWriteOptions writeOptions;
    writeOptions.isBinary = binary;

    // Perform conversion using VTKConverter
    MeshErrorCode errorCode;
    std::string errorMsg;
    bool success = VTKConverter::convert(srcPath, dstPath, srcFormat, dstFormat, processingOptions, writeOptions, errorCode, errorMsg);

    if (success) {
        result.ok = true;
    } else {
        result.message = QString::fromStdString(errorMsg);
    }

    return result;
}

QString formatFileSize(qint64 bytes)
{
    static const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unitIndex = 0;
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        ++unitIndex;
    }
    const int precision = unitIndex == 0 ? 0 : 1;
    return QString::number(size, 'f', precision) + " " + units[unitIndex];
}

QIcon makeEmojiIcon(const QString& emoji)
{
    static QHash<QString, QIcon> cache;
    if (cache.contains(emoji)) {
        return cache.value(emoji);
    }

    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    painter.end();

    QIcon icon(pixmap);
    cache.insert(emoji, icon);
    return icon;
}

bool isSupportedExtension(const QString& suffix)
{
    return kSupportedExtensions.contains(suffix.toLower());
}

class MeshFileSystemModel : public QFileSystemModel
{
public:
    explicit MeshFileSystemModel(QObject* parent = nullptr)
        : QFileSystemModel(parent)
    {
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid()) {
            return QFileSystemModel::data(index, role);
        }

        const QFileInfo info = fileInfo(index);
        if (role == Qt::ToolTipRole && info.isFile() && isSupportedExtension(info.suffix())) {
            const QString sizeText = formatFileSize(info.size());
            const QString timeText = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
            return QString("%1\n大小：%2\n修改时间：%3")
                .arg(info.absoluteFilePath(), sizeText, timeText);
        }

        if (role == Qt::DecorationRole && index.column() == 0 && info.isFile() && isSupportedExtension(info.suffix())) {
            const QString suffix = info.suffix().toLower();
            if (suffix == "vtk" || suffix == "vtu") {
                return makeEmojiIcon("📊");
            }
            if (suffix == "cgns") {
                return makeEmojiIcon("🗂️");
            }
            if (suffix == "msh") {
                return makeEmojiIcon("🔍");
            }
        }

        return QFileSystemModel::data(index, role);
    }
};

class MeshFilterProxyModel : public QSortFilterProxyModel
{
public:
    enum class FilterType { All, Vtk, Cgns, Gmsh, Obj, Off, Stl, Ply };

    explicit MeshFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void setFilterType(FilterType type)
    {
        if (filterType == type) {
            return;
        }
        filterType = type;
        beginFilterChange();
        endFilterChange();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!index.isValid()) {
            return false;
        }

        auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsModel) {
            return true;
        }

        QFileInfo info = fsModel->fileInfo(index);
        if (info.isDir()) {
            return true;
        }

        const QString suffix = info.suffix().toLower();
        if (!isSupportedExtension(suffix)) {
            return false;
        }

        switch (filterType) {
        case FilterType::All:
            return true;
        case FilterType::Vtk:
            return suffix == "vtk" || suffix == "vtu";
        case FilterType::Cgns:
            return suffix == "cgns";
        case FilterType::Gmsh:
            return suffix == "msh";
        case FilterType::Obj:
            return suffix == "obj";
        case FilterType::Off:
            return suffix == "off";
        case FilterType::Stl:
            return suffix == "stl";
        case FilterType::Ply:
            return suffix == "ply";
        default:
            return true;
        }
    }

private:
    FilterType filterType = FilterType::All;
};

transform::transform(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::transform)
{
    ui->setupUi(this);
    // 设置默认窗口大小
    resize(1200, 800);
    setupFileBrowser();
    setupExportPanel();
    setupVTKWidget();
    setupLoadedMeshesTab();
    setupSplitterSizes();
}

void transform::setupSplitterSizes()
{
    if (!ui->mainSplitter) {
        return;
    }

    // 设置左、中、右三个区域的宽度比例为2:3:2
    QList<int> sizes;
    int totalWidth = ui->mainSplitter->width();
    int leftWidth = totalWidth * 2 / 7;
    int centerWidth = totalWidth * 3 / 7;
    int rightWidth = totalWidth * 2 / 7;

    sizes << leftWidth << centerWidth << rightWidth;
    ui->mainSplitter->setSizes(sizes);
}

void transform::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    setupSplitterSizes();
}

void transform::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    setupSplitterSizes();
}

void transform::setupLoadedMeshesTab()
{
    if (!ui->loadedMeshesTreeWidget) {
        return;
    }

    // 设置列宽
    ui->loadedMeshesTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->loadedMeshesTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->loadedMeshesTreeWidget->header()->resizeSection(1, 80);

    // 连接信号
    connect(ui->loadedMeshesTreeWidget, &QTreeWidget::itemClicked,
            this, &transform::onLoadedMeshSelected);
    connect(ui->loadedMeshesTreeWidget, &QTreeWidget::customContextMenuRequested,
            this, &transform::onLoadedMeshContextMenuRequested);
}

void transform::addLoadedMesh(const QString& filePath, const MeshData& meshData)
{
    QFileInfo info(filePath);
    QString fileName = info.fileName();
    QString format = info.suffix().toUpper();

    LoadedMesh mesh;
    mesh.filePath = filePath;
    mesh.fileName = fileName;
    mesh.format = format;
    mesh.meshData = meshData;

    loadedMeshes.append(mesh);
    updateLoadedMeshesTree();
}

void transform::updateLoadedMeshesTree()
{
    if (!ui->loadedMeshesTreeWidget) {
        return;
    }

    // 清空树形控件
    ui->loadedMeshesTreeWidget->clear();
    meshItemMap.clear();

    // 添加已加载的网格
    for (int i = 0; i < loadedMeshes.size(); ++i) {
        const LoadedMesh& mesh = loadedMeshes[i];
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->loadedMeshesTreeWidget);
        item->setText(0, mesh.fileName);
        item->setText(1, mesh.format);
        meshItemMap.insert(item, i);
    }

    // 展开所有节点
    ui->loadedMeshesTreeWidget->expandAll();
}

void transform::onLoadedMeshSelected(QTreeWidgetItem* item, int column)
{
    if (!item || !meshItemMap.contains(item)) {
        return;
    }

    int index = meshItemMap[item];
    if (index < 0 || index >= loadedMeshes.size()) {
        return;
    }

    const LoadedMesh& mesh = loadedMeshes[index];

    // 直接使用已加载的网格数据更新界面，避免重新加载文件
    QFileInfo info(mesh.filePath);
    QString suffix = info.suffix().toLower();

    QString formatText = suffix;
    if (suffix == "vtk") {
        formatText = "VTK Legacy";
    } else if (suffix == "vtu") {
        formatText = "VTK XML";
    } else if (suffix == "cgns") {
        formatText = "CGNS";
    } else if (suffix == "msh") {
        formatText = "Gmsh";
    } else if (suffix == "stl") {
        formatText = "STL";
    } else if (suffix == "obj") {
        formatText = "OBJ";
    } else if (suffix == "off") {
        formatText = "OFF";
    } else if (suffix == "ply") {
        formatText = "PLY";
    }

    const QString sizeText = formatFileSize(info.size());
    const QString importTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 计算网格维度
    QString dimensionText = "-";
    if (!mesh.meshData.cells.empty()) {
        // 简单判断维度：如果有四面体、六面体等3D单元，则为3D网格
        bool has3DCells = false;
        for (const auto& cell : mesh.meshData.cells) {
            switch (cell.type) {
            case VtkCellType::TETRA:
            case VtkCellType::HEXAHEDRON:
            case VtkCellType::WEDGE:
            case VtkCellType::PYRAMID:
                has3DCells = true;
                break;
            default:
                break;
            }
        }
        dimensionText = has3DCells ? "3D" : "2D";
    }

    // 更新基础信息
    if (ui->meshNameValue) {
        ui->meshNameValue->setText(info.fileName());
    }
    if (ui->meshTypeValue) {
        ui->meshTypeValue->setText(formatText);
    }
    if (ui->meshFormatValue) {
        ui->meshFormatValue->setText(formatText);
    }
    if (ui->meshSizeValue) {
        ui->meshSizeValue->setText(sizeText);
    }
    if (ui->meshImportTimeValue) {
        ui->meshImportTimeValue->setText(importTime);
    }
    if (ui->meshDimensionValue) {
        ui->meshDimensionValue->setText(dimensionText);
    }

    // 更新单元统计信息
    updateCellStats(mesh.meshData);

    // 更新属性信息
    updateAttributeInfo(mesh.meshData);

    // 自动填充导出格式和路径
    if (ui->exportFormatCombo) {
        // 根据网格文件格式设置默认导出格式
        int formatIndex = -1;
        for (int i = 0; i < ui->exportFormatCombo->count(); ++i) {
            if (ui->exportFormatCombo->itemData(i).toString() == suffix) {
                formatIndex = i;
                break;
            }
        }
        if (formatIndex != -1) {
            ui->exportFormatCombo->setCurrentIndex(formatIndex);
        } else {
            // 默认选择第一个格式
            ui->exportFormatCombo->setCurrentIndex(0);
        }
    }

    if (ui->exportPathEdit) {
        // 设置默认导出路径，基于选中的网格文件
        QString ext = currentExportExt();
        QString baseName = info.completeBaseName();
        QString dirPath = info.absolutePath();
        QString exportPath = QDir(dirPath).filePath(QString("%1_导出.%2").arg(baseName, ext));
        ui->exportPathEdit->setText(exportPath);
        validateExportPath();
    }

    // 切换到右侧面板的「网格信息」标签页
    if (ui->rightPanelTabs) {
        // 查找「网格信息」标签页的索引
        int tabIndex = -1;
        for (int i = 0; i < ui->rightPanelTabs->count(); ++i) {
            if (ui->rightPanelTabs->tabText(i) == "网格信息") {
                tabIndex = i;
                break;
            }
        }
        if (tabIndex != -1) {
            ui->rightPanelTabs->setCurrentIndex(tabIndex);
        }
    }

    // TODO: 更新3D视图，聚焦并高亮显示当前选中的网格对象
}

void transform::onLoadedMeshContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->loadedMeshesTreeWidget->itemAt(pos);
    if (!item || !meshItemMap.contains(item)) {
        return;
    }

    QMenu menu(this);
    QAction* exportAction = menu.addAction("导出");
    QAction* removeAction = menu.addAction("移除");
    QAction* propertiesAction = menu.addAction("查看属性");

    QAction* selectedAction = menu.exec(ui->loadedMeshesTreeWidget->viewport()->mapToGlobal(pos));
    if (!selectedAction) {
        return;
    }

    if (selectedAction == exportAction) {
        exportSelectedMesh();
    } else if (selectedAction == removeAction) {
        removeSelectedMesh();
    } else if (selectedAction == propertiesAction) {
        viewSelectedMeshProperties();
    }
}

void transform::exportSelectedMesh()
{
    QTreeWidgetItem* selectedItem = ui->loadedMeshesTreeWidget->currentItem();
    if (!selectedItem || !meshItemMap.contains(selectedItem)) {
        return;
    }

    int index = meshItemMap[selectedItem];
    if (index < 0 || index >= loadedMeshes.size()) {
        return;
    }

    const LoadedMesh& mesh = loadedMeshes[index];

    // 设置导出路径
    if (ui->exportPathEdit) {
        QString exportPath = buildDefaultExportPath(currentExportExt());
        ui->exportPathEdit->setText(exportPath);
        validateExportPath();
    }

    // 切换到导出配置标签页
    if (ui->rightPanelTabs) {
        // 查找「导出配置」标签页的索引
        int tabIndex = -1;
        for (int i = 0; i < ui->rightPanelTabs->count(); ++i) {
            if (ui->rightPanelTabs->tabText(i) == "导出配置") {
                tabIndex = i;
                break;
            }
        }
        if (tabIndex != -1) {
            ui->rightPanelTabs->setCurrentIndex(tabIndex);
        }
    }
}

void transform::removeSelectedMesh()
{
    QTreeWidgetItem* selectedItem = ui->loadedMeshesTreeWidget->currentItem();
    if (!selectedItem || !meshItemMap.contains(selectedItem)) {
        return;
    }

    int index = meshItemMap[selectedItem];
    if (index < 0 || index >= loadedMeshes.size()) {
        return;
    }

    // 从列表中移除
    loadedMeshes.removeAt(index);
    updateLoadedMeshesTree();

    // 清空网格信息
    if (ui->meshNameValue) {
        ui->meshNameValue->setText("-");
    }
    if (ui->meshTypeValue) {
        ui->meshTypeValue->setText("-");
    }
    if (ui->meshFormatValue) {
        ui->meshFormatValue->setText("-");
    }
    if (ui->meshSizeValue) {
        ui->meshSizeValue->setText("-");
    }
    if (ui->meshImportTimeValue) {
        ui->meshImportTimeValue->setText("-");
    }
    if (ui->meshDimensionValue) {
        ui->meshDimensionValue->setText("-");
    }
    if (ui->cellStatsTable) {
        ui->cellStatsTable->clearContents();
        ui->cellStatsTable->setRowCount(0);
    }
    if (ui->attrTree) {
        ui->attrTree->clear();
    }
}

void transform::viewSelectedMeshProperties()
{
    QTreeWidgetItem* selectedItem = ui->loadedMeshesTreeWidget->currentItem();
    if (!selectedItem || !meshItemMap.contains(selectedItem)) {
        return;
    }

    int index = meshItemMap[selectedItem];
    if (index < 0 || index >= loadedMeshes.size()) {
        return;
    }

    const LoadedMesh& mesh = loadedMeshes[index];

    // 显示网格属性信息
    QMessageBox::information(this, "网格属性",
        QString("文件路径: %1\n").arg(mesh.filePath) +
        QString("文件名称: %1\n").arg(mesh.fileName) +
        QString("文件格式: %1\n").arg(mesh.format) +
        QString("点数量: %1\n").arg(mesh.meshData.points.size() / 3) +
        QString("单元数量: %1").arg(mesh.meshData.cells.size()));
}

void transform::setupVTKWidget()
{
    // TODO: 实现VTK 3D视图的初始化
    // 由于当前环境配置问题，暂时作为占位符
}

void transform::appendExportLog(const QString& message, const QString& level)
{
    if (!ui->exportTerminalTextEdit) {
        return;
    }

    // 确保在UI线程中执行
    if (QThread::currentThread() != ui->exportTerminalTextEdit->thread()) {
        QMetaObject::invokeMethod(this, "appendExportLog", Qt::QueuedConnection,
                                  Q_ARG(QString, message), Q_ARG(QString, level));
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString levelText = level.isEmpty() ? "INFO" : level;
    QString logMessage;

    // 根据日志级别设置不同的格式
    if (levelText == "ERROR" || levelText == "FAILED") {
        logMessage = QString("<font color='red'>[%1] [%2] %3</font><br/>")
                     .arg(timestamp, levelText, message);
    } else if (levelText == "WARNING") {
        logMessage = QString("<font color='orange'>[%1] [%2] %3</font><br/>")
                     .arg(timestamp, levelText, message);
    } else if (levelText == "SUCCESS") {
        logMessage = QString("<font color='green'>[%1] [%2] %3</font><br/>")
                     .arg(timestamp, levelText, message);
    } else if (levelText == "IMPORTANT") {
        logMessage = QString("<b>[%1] [%2] %3</b><br/>")
                     .arg(timestamp, levelText, message);
    } else {
        logMessage = QString("[%1] [%2] %3<br/>")
                     .arg(timestamp, levelText, message);
    }

    ui->exportTerminalTextEdit->append(logMessage);

    // 自动滚动到底部
    QTextCursor cursor = ui->exportTerminalTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->exportTerminalTextEdit->setTextCursor(cursor);
}

void transform::appendExportLog(const QString& message)
{
    appendExportLog(message, "INFO");
}

void transform::copyExportLog()
{
    if (!ui->exportTerminalTextEdit) {
        return;
    }

    ui->exportTerminalTextEdit->selectAll();
    ui->exportTerminalTextEdit->copy();
    statusBar()->showMessage("日志已复制到剪贴板", 3000);
}

void transform::saveExportLog()
{
    if (!ui->exportTerminalTextEdit) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "保存日志", 
                                                   QDir::homePath() + "/export_log.txt",
                                                   "文本文件 (*.txt);;所有文件 (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->exportTerminalTextEdit->toPlainText();
        file.close();
        statusBar()->showMessage("日志已保存", 3000);
    } else {
        statusBar()->showMessage("保存日志失败", 3000);
    }
}

transform::~transform()
{
    delete ui;
}

bool transform::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->fileTreeView || obj == ui->fileTreeView->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            auto* dragEvent = static_cast<QDragEnterEvent*>(event);
            const QMimeData* mime = dragEvent->mimeData();
            if (mime && mime->hasUrls()) {
                for (const QUrl& url : mime->urls()) {
                    if (url.isLocalFile() && isSupportedMeshFile(url.toLocalFile())) {
                        dragEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
        } else if (event->type() == QEvent::DragMove) {
            auto* moveEvent = static_cast<QDragMoveEvent*>(event);
            const QMimeData* mime = moveEvent->mimeData();
            if (mime && mime->hasUrls()) {
                for (const QUrl& url : mime->urls()) {
                    if (url.isLocalFile() && isSupportedMeshFile(url.toLocalFile())) {
                        moveEvent->acceptProposedAction();
                        return true;
                    }
                }
            }
        } else if (event->type() == QEvent::Drop) {
            auto* dropEvent = static_cast<QDropEvent*>(event);
            const QMimeData* mime = dropEvent->mimeData();
            if (mime && mime->hasUrls()) {
                QStringList files;
                for (const QUrl& url : mime->urls()) {
                    if (url.isLocalFile()) {
                        const QString filePath = url.toLocalFile();
                        if (isSupportedMeshFile(filePath)) {
                            files.append(filePath);
                        }
                    }
                }
                if (!files.isEmpty()) {
                    setRootPath(QFileInfo(files.first()).absolutePath());
                    selectFilesInTree(files, true);
                    statusBar()->showMessage(QString("已定位 %1 个网格文件").arg(files.size()), 5000);
                    dropEvent->acceptProposedAction();
                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void transform::setupFileBrowser()
{
    auto* treeView = ui->fileTreeView;
    fileModel = new MeshFileSystemModel(this);
    fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Drives);
    fileModel->setRootPath(QDir::rootPath());

    fileProxy = new MeshFilterProxyModel(this);
    fileProxy->setSourceModel(fileModel);

    treeView->setModel(fileProxy);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setUniformRowHeights(true);
    treeView->setExpandsOnDoubleClick(false);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->setColumnHidden(1, true);
    treeView->setColumnHidden(2, true);
    treeView->setColumnHidden(3, true);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->formatFilterCombo->addItem("全部支持格式");
    ui->formatFilterCombo->addItem("VTK 系列（vtk/vtu）");
    ui->formatFilterCombo->addItem("CGNS（cgns）");
    ui->formatFilterCombo->addItem("Gmsh（msh）");
    ui->formatFilterCombo->addItem("OBJ（obj）");
    ui->formatFilterCombo->addItem("OFF（off）");
    ui->formatFilterCombo->addItem("STL（stl）");
    ui->formatFilterCombo->addItem("PLY（ply）");

    connect(ui->formatFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &transform::onFilterChanged);
    connect(treeView, &QTreeView::doubleClicked, this, &transform::onTreeDoubleClicked);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &transform::onTreeContextMenuRequested);
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &transform::onSelectionChanged);

    if (ui->menuFile) {
        QAction* openFileAction = ui->menuFile->addAction("打开文件...");
        QAction* openFolderAction = ui->menuFile->addAction("打开文件夹...");
        ui->menuFile->addSeparator();
        QAction* refreshAction = ui->menuFile->addAction("刷新文件列表");
        connect(openFileAction, &QAction::triggered, this, &transform::onOpenFileClicked);
        connect(openFolderAction, &QAction::triggered, this, &transform::onOpenFolderClicked);
        connect(refreshAction, &QAction::triggered, this, &transform::onRefreshClicked);
    }

    treeView->installEventFilter(this);
    treeView->viewport()->installEventFilter(this);

    // 不设置默认根路径，需要用户自行打开文件夹或者打开网格文件
}


void transform::setupExportPanel()
{
    if (!ui->exportFormatCombo) {
        return;
    }

    ui->exportFormatCombo->clear();
    ui->exportFormatCombo->addItem("VTK Legacy (.vtk)", "vtk");
    ui->exportFormatCombo->addItem("VTK XML (.vtu)", "vtu");
    ui->exportFormatCombo->addItem("CGNS (.cgns)", "cgns");
    ui->exportFormatCombo->addItem("Gmsh (.msh)", "msh");
    ui->exportFormatCombo->addItem("STL (.stl)", "stl");
    ui->exportFormatCombo->addItem("OBJ (.obj)", "obj");
    ui->exportFormatCombo->addItem("OFF (.off)", "off");
    ui->exportFormatCombo->addItem("PLY (.ply)", "ply");

    if (ui->surfaceMeshRadio) {
        ui->surfaceMeshRadio->setToolTip("仅保留面单元");
    }
    if (ui->binaryExportCheck) {
        ui->binaryExportCheck->setToolTip("文件更小，部分老软件兼容性略低");
        ui->binaryExportCheck->setChecked(true);
    }

    if (ui->exportPathEdit && ui->exportPathEdit->text().trimmed().isEmpty()) {
        updateExportPathForFormat();
    }

    if (ui->browseExportPathButton) {
        connect(ui->browseExportPathButton, &QToolButton::clicked,
                this, &transform::onBrowseExportPathClicked);
    }
    if (ui->exportNowButton) {
        ui->exportNowButton->setText("导出");
        connect(ui->exportNowButton, &QPushButton::clicked,
                this, &transform::onExportNowClicked);
    }
    if (ui->clearLogButton) {
        connect(ui->clearLogButton, &QPushButton::clicked, this, [this] {
            if (ui->exportTerminalTextEdit) {
                ui->exportTerminalTextEdit->clear();
            }
        });
    }

    if (ui->copyLogButton) {
        connect(ui->copyLogButton, &QPushButton::clicked, this, &transform::copyExportLog);
    }

    if (ui->saveLogButton) {
        connect(ui->saveLogButton, &QPushButton::clicked, this, &transform::saveExportLog);
    }

    if (ui->exportPathEdit) {
        connect(ui->exportPathEdit, &QLineEdit::textChanged, this, [this] {
            validateExportPath();
        });
    }

    connect(ui->exportFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        const QString formatExt = ui->exportFormatCombo->currentData().toString();
        const bool isStl = (formatExt == "stl");

        if (ui->volumeMeshRadio) {
            ui->volumeMeshRadio->setEnabled(!isStl);
        }

        if (isStl && ui->surfaceMeshRadio) {
            ui->surfaceMeshRadio->setChecked(true);
        } else if (!isStl && ui->volumeMeshRadio) {
            ui->volumeMeshRadio->setChecked(true);
        }

        updateExportPathForFormat();
        validateExportPath();
    });

    if (ui->exportFormatCombo->count() > 0) {
        ui->exportFormatCombo->setCurrentIndex(0);
    }

    if (!exportProgressTimer) {
        exportProgressTimer = new QTimer(this);
        exportProgressTimer->setInterval(200);
        connect(exportProgressTimer, &QTimer::timeout, this, [this] {
            exportProgressValue = qMin(exportProgressValue + 5, 95);
            statusBar()->showMessage(QString("导出中：%1%").arg(exportProgressValue));
        });
    }
}

QString transform::currentExportExt() const
{
    if (!ui->exportFormatCombo) {
        return "vtk";
    }
    const QString ext = ui->exportFormatCombo->currentData().toString();
    return ext.isEmpty() ? "vtk" : ext;
}

QString transform::buildDefaultExportPath(const QString& ext, const QString& baseDirOverride) const
{
    QString baseDir = baseDirOverride;
    QString baseName;

    if (baseDir.isEmpty() && ui->fileTreeView && ui->fileTreeView->selectionModel()) {
        const QModelIndexList indexes = ui->fileTreeView->selectionModel()->selectedIndexes();
        for (const QModelIndex& idx : indexes) {
            if (idx.column() != 0) {
                continue;
            }
            const QModelIndex sourceIndex = fileProxy ? fileProxy->mapToSource(idx) : idx;
            if (!sourceIndex.isValid()) {
                continue;
            }
            const QFileInfo info = fileModel ? fileModel->fileInfo(sourceIndex) : QFileInfo();
            if (info.isFile()) {
                baseDir = info.absolutePath();
                baseName = info.completeBaseName();
                break;
            }
        }
    }

    if (baseDir.isEmpty()) {
        baseDir = currentRootPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                            : currentRootPath;
    }
    if (baseName.isEmpty()) {
        baseName = "未命名";
    }

    const QString fileName = QString("%1_导出.%2").arg(baseName, ext);
    return QDir(baseDir).filePath(fileName);
}

void transform::updateExportPathForFormat()
{
    if (!ui->exportPathEdit) {
        return;
    }

    const QString ext = currentExportExt();
    const QString newAutoPath = buildDefaultExportPath(ext);

    const QString currentText = ui->exportPathEdit->text().trimmed();
    if (currentText.isEmpty() || currentText == lastAutoExportPath) {
        ui->exportPathEdit->setText(newAutoPath);
        lastAutoExportPath = newAutoPath;
    }
}

bool transform::isPathWritable(const QString& path) const
{
    if (path.trimmed().isEmpty()) {
        return false;
    }
    const QFileInfo info(path);
    const QString dirPath = info.absolutePath();
    if (dirPath.isEmpty()) {
        return false;
    }
    const QDir dir(dirPath);
    if (!dir.exists()) {
        return false;
    }
    const QFileInfo dirInfo(dirPath);
    return dirInfo.isWritable();
}

void transform::validateExportPath()
{
    if (!ui->exportPathEdit) {
        return;
    }

    const QString path = ui->exportPathEdit->text().trimmed();
    const bool writable = isPathWritable(path);

    if (writable) {
        ui->exportPathEdit->setStyleSheet(QString());
        ui->exportPathEdit->setToolTip(QString());
        exportFallbackPath.clear();
        return;
    }

    const QString fallbackDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    exportFallbackPath = buildDefaultExportPath(currentExportExt(), fallbackDir);
    ui->exportPathEdit->setStyleSheet("QLineEdit { border: 1px solid #d9534f; }");
    ui->exportPathEdit->setToolTip("路径不可写");
}

void transform::setRootPath(const QString& path)
{
    if (path.isEmpty()) {
        // 空路径时，设置一个无效的根索引，使文件树视图保持空白
        currentRootPath = "";
        ui->fileTreeView->setRootIndex(QModelIndex());
        return;
    }

    const QString resolvedPath = QDir(path).absolutePath();
    currentRootPath = resolvedPath;

    QModelIndex sourceIndex = fileModel->setRootPath(resolvedPath);
    QModelIndex proxyIndex = fileProxy->mapFromSource(sourceIndex);
    ui->fileTreeView->setRootIndex(proxyIndex);
    ui->fileTreeView->expand(proxyIndex);
}

void transform::selectFilesInTree(const QStringList& filePaths, bool clearSelection)
{
    if (!ui->fileTreeView->selectionModel()) {
        return;
    }

    QItemSelectionModel* selectionModel = ui->fileTreeView->selectionModel();
    if (clearSelection) {
        selectionModel->clearSelection();
    }

    for (const QString& filePath : filePaths) {
        QModelIndex sourceIndex = fileModel->index(filePath);
        if (!sourceIndex.isValid()) {
            continue;
        }
        QModelIndex proxyIndex = fileProxy->mapFromSource(sourceIndex);
        if (!proxyIndex.isValid()) {
            continue;
        }
        selectionModel->select(proxyIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        ui->fileTreeView->scrollTo(proxyIndex);
        ui->fileTreeView->expand(proxyIndex.parent());
    }
}

struct ImportResult {
    bool success;
    MeshData meshData;
    QString errorMessage;
};

ImportResult importMeshFileAsync(const QString& filePath)
{
    ImportResult result;
    result.success = false;
    
    try {
        // 使用 MeshReader 读取文件
        MeshData meshData;
        MeshErrorCode errorCode;
        std::string errorMsg;
        
        const std::string filePathStd = filePath.toStdString();
        bool success = MeshReader::readAuto(filePathStd, meshData, errorCode, errorMsg);
        
        if (success) {
            result.success = true;
            result.meshData = meshData;
        } else {
            result.errorMessage = QString::fromStdString(errorMsg);
        }
    } catch (const std::exception& e) {
        result.errorMessage = QString::fromStdString(e.what());
    } catch (...) {
        result.errorMessage = "导入过程中发生未知错误";
    }
    
    return result;
}

void transform::importMeshFile(const QString& filePath)
{
    statusBar()->showMessage(QString("正在导入：%1").arg(QFileInfo(filePath).fileName()), 5000);
    
    // 首先检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString errorMessage = QString("文件不存在：%1").arg(filePath);
        statusBar()->showMessage(QString("导入失败：%1").arg(errorMessage), 5000);
        QMessageBox::warning(this, "导入失败", errorMessage);
        return;
    }
    
    // 检查文件是否可读
    if (!fileInfo.isReadable()) {
        QString errorMessage = QString("文件不可读：%1").arg(filePath);
        statusBar()->showMessage(QString("导入失败：%1").arg(errorMessage), 5000);
        QMessageBox::warning(this, "导入失败", errorMessage);
        return;
    }
    
    // 检查文件扩展名是否支持
    QString suffix = fileInfo.suffix().toLower();
    if (!isSupportedExtension(suffix)) {
        QString errorMessage = QString("不支持的文件格式：%1").arg(suffix);
        statusBar()->showMessage(QString("导入失败：%1").arg(errorMessage), 5000);
        QMessageBox::warning(this, "导入失败", errorMessage);
        return;
    }
    
    // 使用QtConcurrent异步执行导入操作
    auto future = QtConcurrent::run(importMeshFileAsync, filePath);
    
    auto* watcher = new QFutureWatcher<ImportResult>(this);
    connect(watcher, &QFutureWatcher<ImportResult>::finished, this, [this, filePath, watcher] {
        ImportResult result = watcher->result();
        watcher->deleteLater();
        
        if (result.success) {
            // 读取成功，更新网格信息
            updateMeshInfo(filePath);
            statusBar()->showMessage(QString("导入成功：%1").arg(QFileInfo(filePath).fileName()), 5000);
            
            // 更新单元统计信息
            updateCellStats(result.meshData);
            
            // 更新属性信息
            updateAttributeInfo(result.meshData);
            
            // 加载到已加载网格区域
            addLoadedMesh(filePath, result.meshData);
            
            // TODO: 通知 3D 视图区刷新
        } else {
            // 读取失败，显示错误信息
            statusBar()->showMessage(QString("导入失败：%1").arg(result.errorMessage), 5000);
            QMessageBox::warning(this, "导入失败", result.errorMessage);
        }
    });
    
    watcher->setFuture(future);
}

void transform::updateMeshInfo(const QString& filePath)
{
    const QFileInfo info(filePath);
    const QString suffix = info.suffix().toLower();

    QString formatText = suffix;
    if (suffix == "vtk") {
        formatText = "VTK Legacy";
    } else if (suffix == "vtu") {
        formatText = "VTK XML";
    } else if (suffix == "cgns") {
        formatText = "CGNS";
    } else if (suffix == "msh") {
        formatText = "Gmsh";
    } else if (suffix == "stl") {
        formatText = "STL";
    }

    const QString sizeText = formatFileSize(info.size());
    const QString importTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QString dimensionText = "-";
    QString error;
    vtkSmartPointer<vtkDataSet> dataSet = loadMeshDataSet(filePath, &error);
    if (dataSet) {
        double bounds[6] = {0, 0, 0, 0, 0, 0};
        dataSet->GetBounds(bounds);
        const double zSpan = bounds[5] - bounds[4];
        const bool is2D = std::abs(zSpan) < 1e-6;
        dimensionText = is2D ? "2D" : "3D";
    }

    if (ui->meshNameValue) {
        ui->meshNameValue->setText(info.fileName());
    }
    if (ui->meshTypeValue) {
        ui->meshTypeValue->setText(formatText);
    }
    if (ui->meshFormatValue) {
        ui->meshFormatValue->setText(formatText);
    }
    if (ui->meshSizeValue) {
        ui->meshSizeValue->setText(sizeText);
    }
    if (ui->meshImportTimeValue) {
        ui->meshImportTimeValue->setText(importTime);
    }
    if (ui->meshDimensionValue) {
        ui->meshDimensionValue->setText(dimensionText);
    }
}

void transform::updateCellStats(const MeshData& meshData)
{
    if (!ui->cellStatsTable) {
        return;
    }
    
    // 清空表格
    ui->cellStatsTable->clear();
    ui->cellStatsTable->setRowCount(0);
    
    // 设置表头
    ui->cellStatsTable->setHorizontalHeaderLabels({"单元类型", "数量"});
    
    if (meshData.metadata.cellTypeCount.empty()) {
        // 无单元类型数据
        ui->cellStatsTable->setRowCount(1);
        ui->cellStatsTable->setColumnCount(2);
        ui->cellStatsTable->setItem(0, 0, new QTableWidgetItem("无单元类型数据"));
        ui->cellStatsTable->setSpan(0, 0, 1, 2);
    } else {
        // 填充单元类型数据
        int row = 0;
        for (const auto& pair : meshData.metadata.cellTypeCount) {
            QString cellTypeText;
            switch (pair.first) {
                case VtkCellType::VERTEX:
                    cellTypeText = "顶点";
                    break;
                case VtkCellType::LINE:
                    cellTypeText = "线段";
                    break;
                case VtkCellType::TRIANGLE:
                    cellTypeText = "三角形";
                    break;
                case VtkCellType::QUAD:
                    cellTypeText = "四边形";
                    break;
                case VtkCellType::TETRA:
                    cellTypeText = "四面体";
                    break;
                case VtkCellType::HEXAHEDRON:
                    cellTypeText = "六面体";
                    break;
                case VtkCellType::WEDGE:
                    cellTypeText = "楔形";
                    break;
                case VtkCellType::PYRAMID:
                    cellTypeText = "金字塔";
                    break;
                case VtkCellType::TRIANGLE_STRIP:
                    cellTypeText = "三角形带";
                    break;
                case VtkCellType::POLYGON:
                    cellTypeText = "多边形";
                    break;
                default:
                    cellTypeText = "未知";
                    break;
            }
            
            ui->cellStatsTable->insertRow(row);
            ui->cellStatsTable->setItem(row, 0, new QTableWidgetItem(cellTypeText));
            ui->cellStatsTable->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second)));
            row++;
        }
    }
    
    // 调整列宽
    ui->cellStatsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void transform::updateAttributeInfo(const MeshData& meshData)
{
    if (!ui->attrTree) {
        return;
    }
    
    // 清空树形控件
    ui->attrTree->clear();
    
    // 设置表头
    ui->attrTree->setHeaderLabels({"属性类型", "属性名称"});
    
    // 添加点属性
    QTreeWidgetItem* pointDataItem = new QTreeWidgetItem(ui->attrTree, {"点属性"});
    if (meshData.metadata.pointDataNames.empty()) {
        new QTreeWidgetItem(pointDataItem, {"无点属性数据"});
    } else {
        for (const auto& name : meshData.metadata.pointDataNames) {
            new QTreeWidgetItem(pointDataItem, {QString::fromStdString(name)});
        }
    }
    
    // 添加单元属性
    QTreeWidgetItem* cellDataItem = new QTreeWidgetItem(ui->attrTree, {"单元属性"});
    if (meshData.metadata.cellDataNames.empty()) {
        new QTreeWidgetItem(cellDataItem, {"无单元属性数据"});
    } else {
        for (const auto& name : meshData.metadata.cellDataNames) {
            new QTreeWidgetItem(cellDataItem, {QString::fromStdString(name)});
        }
    }
    
    // 添加物理区域
    QTreeWidgetItem* physicalItem = new QTreeWidgetItem(ui->attrTree, {"物理区域"});
    if (meshData.metadata.physicalRegions.empty()) {
        new QTreeWidgetItem(physicalItem, {"无物理区域数据"});
    } else {
        for (const auto& region : meshData.metadata.physicalRegions) {
            new QTreeWidgetItem(physicalItem, {QString::fromStdString(region)});
        }
    }
    
    // 展开所有节点
    ui->attrTree->expandAll();
    
    // 调整列宽
    ui->attrTree->header()->setSectionResizeMode(QHeaderView::Stretch);
}

bool transform::isSupportedMeshFile(const QString& filePath) const
{
    return isSupportedExtension(QFileInfo(filePath).suffix());
}

void transform::onOpenFileClicked()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        "打开网格文件",
        currentRootPath.isEmpty() ? QDir::homePath() : currentRootPath,
        "网格文件 (*.vtk *.vtu *.cgns *.msh *.obj *.off *.stl *.ply);;VTK文件 (*.vtk *.vtu);;CGNS文件 (*.cgns);;Gmsh文件 (*.msh);;OBJ文件 (*.obj);;OFF文件 (*.off);;STL文件 (*.stl);;PLY文件 (*.ply);;所有文件 (*.*)"
    );

    if (files.isEmpty()) {
        return;
    }

    setRootPath(QFileInfo(files.first()).absolutePath());
    selectFilesInTree(files, true);
}

void transform::onOpenFolderClicked()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this,
        "打开网格文件夹",
        currentRootPath.isEmpty() ? QDir::homePath() : currentRootPath
    );

    if (folder.isEmpty()) {
        return;
    }

    setRootPath(folder);
}

void transform::onRefreshClicked()
{
    if (currentRootPath.isEmpty()) {
        // 当currentRootPath为空时，保持文件树视图空白
        statusBar()->showMessage("请先打开文件夹或文件", 3000);
        return;
    }

    fileModel->setRootPath(currentRootPath);
    setRootPath(currentRootPath);
    statusBar()->showMessage("已刷新文件列表", 3000);
}

void transform::onFilterChanged(int index)
{
    auto* proxy = static_cast<MeshFilterProxyModel*>(fileProxy);
    switch (index) {
    case 0:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::All);
        break;
    case 1:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Vtk);
        break;
    case 2:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Cgns);
        break;
    case 3:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Gmsh);
        break;
    case 4:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Obj);
        break;
    case 5:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Off);
        break;
    case 6:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Stl);
        break;
    case 7:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::Ply);
        break;
    default:
        proxy->setFilterType(MeshFilterProxyModel::FilterType::All);
        break;
    }
}

void transform::onBrowseExportPathClicked()
{
    if (!ui->exportPathEdit || !ui->exportFormatCombo) {
        return;
    }

    const QString ext = currentExportExt();
    const QString filter = QString("%1 (*.%2)")
                               .arg(ui->exportFormatCombo->currentText(), ext);
    const QString startPath = ui->exportPathEdit->text().trimmed().isEmpty()
        ? buildDefaultExportPath(ext)
        : ui->exportPathEdit->text().trimmed();

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "选择导出路径",
        startPath,
        filter
    );

    if (filePath.isEmpty()) {
        return;
    }

    ui->exportPathEdit->setText(filePath);
    lastAutoExportPath = filePath;
    validateExportPath();
}

void transform::onExportNowClicked()
{
    if (!ui->exportFormatCombo) {
        return;
    }

    if (exportInProgress) {
        return;
    }

    QString sourcePath;
    if (ui->fileTreeView && ui->fileTreeView->selectionModel()) {
        const QModelIndexList indexes = ui->fileTreeView->selectionModel()->selectedRows(0);
        for (const QModelIndex& idx : indexes) {
            const QModelIndex sourceIndex = fileProxy ? fileProxy->mapToSource(idx) : idx;
            const QFileInfo info = fileModel ? fileModel->fileInfo(sourceIndex) : QFileInfo();
            if (info.isFile()) {
                sourcePath = info.absoluteFilePath();
                break;
            }
        }
    }

    // 如果文件浏览器中没有选中文件，检查已加载网格中是否有选中项
    if (sourcePath.isEmpty() && ui->loadedMeshesTreeWidget) {
        QTreeWidgetItem* selectedItem = ui->loadedMeshesTreeWidget->currentItem();
        if (selectedItem && meshItemMap.contains(selectedItem)) {
            int index = meshItemMap[selectedItem];
            if (index >= 0 && index < loadedMeshes.size()) {
                sourcePath = loadedMeshes[index].filePath;
            }
        }
    }

    if (sourcePath.isEmpty()) {
        QMessageBox::warning(this, "导出", "请先选择一个网格文件。");
        return;
    }

    QString exportPath = ui->exportPathEdit ? ui->exportPathEdit->text().trimmed() : QString();
    if (exportPath.isEmpty()) {
        exportPath = buildDefaultExportPath(currentExportExt());
        if (ui->exportPathEdit) {
            ui->exportPathEdit->setText(exportPath);
        }
    }

    if (exportPath.isEmpty()) {
        QMessageBox::warning(this, "导出", "请先选择导出路径。");
        return;
    }

    if (!isPathWritable(exportPath)) {
        if (!exportFallbackPath.isEmpty()) {
            exportPath = exportFallbackPath;
        }
        if (ui->exportPathEdit) {
            ui->exportPathEdit->setText(exportPath);
        }
        QMessageBox::warning(this, "导出", "路径不可写，已切换到文档目录备用路径。"
        );
    }

    const QString formatExt = currentExportExt();
    const QString formatText = ui->exportFormatCombo->currentText();
    const bool isVolume = ui->volumeMeshRadio ? ui->volumeMeshRadio->isChecked() : true;
    const bool isBinary = ui->binaryExportCheck ? ui->binaryExportCheck->isChecked() : true;

    const QString meshTypeText = isVolume ? "体网格" : "面网格";
    const QString modeText = isBinary ? "二进制" : "ASCII";

    const QString desiredSuffix = "." + formatExt;
    if (!exportPath.endsWith(desiredSuffix, Qt::CaseInsensitive)) {
        const QFileInfo info(exportPath);
        exportPath = QDir(info.absolutePath()).filePath(info.completeBaseName() + desiredSuffix);
        if (ui->exportPathEdit) {
            ui->exportPathEdit->setText(exportPath);
        }
    }

    exportInProgress = true;
    exportProgressValue = 5;
    if (ui->exportNowButton) {
        ui->exportNowButton->setText("导出中...");
        ui->exportNowButton->setEnabled(false);
    }
    if (exportProgressTimer) {
        exportProgressTimer->start();
    }

    appendExportLog(QString("开始导出：%1").arg(QFileInfo(sourcePath).fileName()), "INFO");
    appendExportLog(QString("目标格式：%1").arg(ui->exportFormatCombo->currentText()), "INFO");
    appendExportLog(QString("导出路径：%1").arg(exportPath), "INFO");
    appendExportLog(QString("网格类型：%1").arg(isVolume ? "体网格" : "面网格"), "INFO");
    appendExportLog(QString("输出模式：%1").arg(isBinary ? "二进制" : "ASCII"), "INFO");
    appendExportLog("正在准备导出数据...", "INFO");
    appendExportLog("正在读取源文件...", "INFO");
    appendExportLog(QString("进度：10%"), "INFO");
    appendExportLog("正在处理网格数据...", "INFO");
    appendExportLog(QString("进度：30%"), "INFO");
    appendExportLog("正在转换格式...", "INFO");
    appendExportLog(QString("进度：60%"), "INFO");
    appendExportLog("正在写入目标文件...", "INFO");
    appendExportLog(QString("进度：80%"), "INFO");

    auto future = QtConcurrent::run([sourcePath, exportPath, formatExt, isVolume, isBinary]() {
        return exportMeshFile(sourcePath, exportPath, formatExt, !isVolume, isBinary);
    });

    auto* watcher = new QFutureWatcher<ExportResult>(this);
    connect(watcher, &QFutureWatcher<ExportResult>::finished, this, [this, watcher, exportPath] {
        const ExportResult result = watcher->result();
        watcher->deleteLater();

        if (exportProgressTimer) {
            exportProgressTimer->stop();
        }

        exportInProgress = false;
        if (ui->exportNowButton) {
            ui->exportNowButton->setText("导出");
            ui->exportNowButton->setEnabled(true);
        }

        appendExportLog(QString("进度：100%"), "INFO");

        if (result.ok) {
            // 导出成功，绿色显示结果
            statusBar()->showMessage("导出成功", 3000);
            appendExportLog("导出成功！", "SUCCESS");
            appendExportLog(QString("文件已保存至：%1").arg(exportPath), "SUCCESS");
            appendExportLog("导出操作已完成。", "SUCCESS");
        } else {
            // 导出失败，红色显示结果
            statusBar()->showMessage("导出失败", 3000);
            appendExportLog(QString("导出失败：%1").arg(result.message.isEmpty() ? "未知错误" : result.message), "ERROR");
            appendExportLog("导出操作已终止。", "ERROR");
            QMessageBox::warning(this, "导出失败", result.message.isEmpty() ? "导出失败" : result.message);
        }
    });
    watcher->setFuture(future);
}

void transform::onTreeDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    QModelIndex sourceIndex = fileProxy->mapToSource(index);
    QFileInfo info = fileModel->fileInfo(sourceIndex);
    if (info.isDir()) {
        setRootPath(info.absoluteFilePath());
        return;
    }

    if (info.isFile() && isSupportedMeshFile(info.absoluteFilePath())) {
        importMeshFile(info.absoluteFilePath());
    }
}

void transform::onTreeContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = ui->fileTreeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QModelIndex sourceIndex = fileProxy->mapToSource(index);
    QFileInfo info = fileModel->fileInfo(sourceIndex);
    if (!info.isFile() || !isSupportedMeshFile(info.absoluteFilePath())) {
        return;
    }

    QMenu menu(this);
    QAction* importAction = menu.addAction("导入网格");
    QAction* openDirAction = menu.addAction("打开所在目录");
    QAction* diagnoseAction = menu.addAction("格式诊断");
    QAction* batchAction = menu.addAction("添加到批量转换列表");

    QAction* selected = menu.exec(ui->fileTreeView->viewport()->mapToGlobal(pos));
    if (!selected) {
        return;
    }

    if (selected == importAction) {
        importMeshFile(info.absoluteFilePath());
    } else if (selected == openDirAction) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
    } else if (selected == diagnoseAction) {
        QMessageBox::information(this, "格式诊断", "已进行格式诊断（示例占位）。");
    } else if (selected == batchAction) {
        QMessageBox::information(this, "批量转换", "已添加到批量转换列表（示例占位）。");
    }
}

void transform::onSelectionChanged(const QItemSelection& selected, const QItemSelection&)
{
    const QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) {
        return;
    }

    QModelIndex firstColumnIndex;
    for (const QModelIndex& idx : indexes) {
        if (idx.column() == 0) {
            firstColumnIndex = idx;
            break;
        }
    }
    if (!firstColumnIndex.isValid()) {
        return;
    }

    QModelIndex sourceIndex = fileProxy->mapToSource(firstColumnIndex);
    QFileInfo info = fileModel->fileInfo(sourceIndex);
    if (info.isFile()) {
        const QString sizeText = formatFileSize(info.size());
        statusBar()->showMessage(QString("格式：%1  大小：%2  路径：%3")
                                     .arg(info.suffix().toLower(), sizeText, info.absoluteFilePath()));
    } else if (info.isDir()) {
        statusBar()->showMessage(QString("文件夹：%1").arg(info.absoluteFilePath()));
    }
}
