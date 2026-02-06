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
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

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

const QSet<QString> kSupportedExtensions = {"vtk", "vtu", "cgns", "msh"};

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
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == "vtk") {
        auto reader = vtkSmartPointer<vtkDataSetReader>::New();
        reader->SetFileName(filePath.toLocal8Bit().constData());
        reader->Update();
        return reader->GetOutput();
    }
    if (suffix == "vtu") {
        auto reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filePath.toLocal8Bit().constData());
        reader->Update();
        return reader->GetOutput();
    }
    if (suffix == "stl") {
        auto reader = vtkSmartPointer<vtkSTLReader>::New();
        reader->SetFileName(filePath.toLocal8Bit().constData());
        reader->Update();
        return reader->GetOutput();
    }
#ifdef HAS_VTK_IOCGNS
    if (suffix == "cgns") {
        auto reader = vtkSmartPointer<vtkCGNSReader>::New();
        reader->SetFileName(filePath.toLocal8Bit().constData());
        reader->Update();
        auto* mb = reader->GetOutput();
        return extractFirstDataSet(mb);
    }
#endif
#ifdef HAS_VTK_GMSH
    if (suffix == "msh") {
        auto reader = vtkSmartPointer<vtkGmshReader>::New();
        reader->SetFileName(filePath.toLocal8Bit().constData());
        reader->Update();
        return reader->GetOutput();
    }
#endif

    if (errorMessage) {
        *errorMessage = "不支持的网格格式";
    }
    return nullptr;
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

    QString error;
    vtkSmartPointer<vtkDataSet> dataSet = loadMeshDataSet(sourcePath, &error);
    if (!dataSet) {
        result.message = error.isEmpty() ? "读取网格失败" : error;
        return result;
    }

    vtkSmartPointer<vtkDataSet> exportDataSet = dataSet;
    vtkSmartPointer<vtkPolyData> surfaceData;

    if (exportSurface) {
        surfaceData = toSurfaceMesh(dataSet);
        if (!surfaceData) {
            result.message = "提取面网格失败";
            return result;
        }
    }

    const QString ext = formatExt.toLower();
    const std::string outPath = outputPath.toLocal8Bit().toStdString();

    if (ext == "vtk") {
        auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
        writer->SetFileName(outPath.c_str());
        vtkDataSet* dataToWrite = exportSurface
            ? static_cast<vtkDataSet*>(surfaceData)
            : exportDataSet.GetPointer();
        writer->SetInputData(dataToWrite);
        binary ? writer->SetFileTypeToBinary() : writer->SetFileTypeToASCII();
        if (writer->Write()) {
            result.ok = true;
            return result;
        }
        result.message = "VTK 写出失败";
        return result;
    }

    if (ext == "vtu") {
        vtkSmartPointer<vtkUnstructuredGrid> grid = exportSurface
            ? toUnstructuredGrid(surfaceData)
            : toUnstructuredGrid(exportDataSet);
        if (!grid) {
            result.message = "VTU 需要非结构网格";
            return result;
        }
        auto writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
        writer->SetFileName(outPath.c_str());
        writer->SetInputData(grid);
        binary ? writer->SetDataModeToBinary() : writer->SetDataModeToAscii();
        if (writer->Write()) {
            result.ok = true;
            return result;
        }
        result.message = "VTU 写出失败";
        return result;
    }

    if (ext == "stl") {
        vtkSmartPointer<vtkPolyData> polyData = exportSurface ? surfaceData : toSurfaceMesh(exportDataSet);
        if (!polyData) {
            result.message = "STL 需要面网格";
            return result;
        }
        auto writer = vtkSmartPointer<vtkSTLWriter>::New();
        writer->SetFileName(outPath.c_str());
        writer->SetInputData(polyData);
        binary ? writer->SetFileTypeToBinary() : writer->SetFileTypeToASCII();
        if (writer->Write()) {
            result.ok = true;
            return result;
        }
        result.message = "STL 写出失败";
        return result;
    }

#ifdef HAS_VTK_CGNS_WRITER
    if (ext == "cgns") {
        vtkSmartPointer<vtkUnstructuredGrid> grid = exportSurface
            ? toUnstructuredGrid(surfaceData)
            : toUnstructuredGrid(exportDataSet);
        if (!grid) {
            result.message = "CGNS 需要非结构网格";
            return result;
        }
        auto writer = vtkSmartPointer<vtkCGNSWriter>::New();
        writer->SetFileName(outPath.c_str());
        writer->SetInputData(grid);
        if (writer->Write()) {
            result.ok = true;
            return result;
        }
        result.message = "CGNS 写出失败";
        return result;
    }
#endif

#ifdef HAS_VTK_GMSH_WRITER
    if (ext == "msh") {
        vtkSmartPointer<vtkUnstructuredGrid> grid = exportSurface
            ? toUnstructuredGrid(surfaceData)
            : toUnstructuredGrid(exportDataSet);
        if (!grid) {
            result.message = "Gmsh 需要非结构网格";
            return result;
        }
        auto writer = vtkSmartPointer<vtkGmshWriter>::New();
        writer->SetFileName(outPath.c_str());
        writer->SetInputData(grid);
        if (writer->Write()) {
            result.ok = true;
            return result;
        }
        result.message = "Gmsh 写出失败";
        return result;
    }
#endif

    result.message = "当前构建未包含该格式写出支持";
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
    enum class FilterType { All, Vtk, Cgns, Gmsh };

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
    setupFileBrowser();
    setupExportPanel();
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

    setRootPath(QDir::rootPath());
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
    const QString resolvedPath = path.isEmpty() ? QDir::rootPath() : QDir(path).absolutePath();
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

void transform::importMeshFile(const QString& filePath)
{
    statusBar()->showMessage(QString("已触发导入：%1").arg(QFileInfo(filePath).fileName()), 5000);
    updateMeshInfo(filePath);
    // TODO: 调用 MeshReader 读取文件，加载到已加载网格区域，并通知 3D 视图区刷新。
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
        "网格文件 (*.vtk *.vtu *.cgns *.msh)"
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
        currentRootPath = QDir::rootPath();
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

    auto future = QtConcurrent::run([sourcePath, exportPath, formatExt, isVolume, isBinary]() {
        return exportMeshFile(sourcePath, exportPath, formatExt, !isVolume, isBinary);
    });

    auto* watcher = new QFutureWatcher<ExportResult>(this);
    connect(watcher, &QFutureWatcher<ExportResult>::finished, this, [this, watcher] {
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

        if (result.ok) {
            statusBar()->showMessage("导出成功", 3000);
        } else {
            statusBar()->showMessage("导出失败", 3000);
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
