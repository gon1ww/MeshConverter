#pragma once
#include <QMainWindow>
#include <QStringList>
#include <MeshTypes.h>

QT_BEGIN_NAMESPACE
namespace Ui { class transform; }
QT_END_NAMESPACE

class MeshFileSystemModel;
class MeshFilterProxyModel;
class QItemSelection;
class QModelIndex;
class QPoint;
class QTimer;
class QTreeWidgetItem;

class transform : public QMainWindow {
    Q_OBJECT

public:
    transform(QWidget* parent = nullptr);
    ~transform();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onOpenFileClicked();
    void onOpenFolderClicked();
    void onRefreshClicked();
    void onFilterChanged(int index);
    void onTreeDoubleClicked(const QModelIndex& index);
    void onTreeContextMenuRequested(const QPoint& pos);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onBrowseExportPathClicked();
    void onExportNowClicked();

private:
    void showMeshDetailedInfo(const MeshData& meshData, const QString& filePath);

private:
    void setupFileBrowser();
    void setupExportPanel();
    void setupVTKWidget();
    void setupSplitterSizes();
    void updateExportPathForFormat();
    QString buildDefaultExportPath(const QString& ext, const QString& baseDirOverride = QString()) const;
    bool isPathWritable(const QString& path) const;
    void validateExportPath();
    QString currentExportExt() const;
    void updateMeshInfo(const QString& filePath);
    void setRootPath(const QString& path);
    void selectFilesInTree(const QStringList& filePaths, bool clearSelection = true);
    void importMeshFile(const QString& filePath);
    bool isSupportedMeshFile(const QString& filePath) const;
    void updateCellStats(const MeshData& meshData);
    void updateAttributeInfo(const MeshData& meshData);
    void appendExportLog(const QString& message);
    void appendExportLog(const QString& message, const QString& level);
    void copyExportLog();
    void saveExportLog();

private:
    // 网格数据结构
    struct LoadedMesh {
        QString filePath;
        QString fileName;
        QString format;
        MeshData meshData;
    };

private:
    Ui::transform* ui;
    MeshFileSystemModel* fileModel = nullptr;
    MeshFilterProxyModel* fileProxy = nullptr;
    QString currentRootPath;
    QString exportFallbackPath;
    QString lastAutoExportPath;
    QTimer* exportProgressTimer = nullptr;
    int exportProgressValue = 0;
    bool exportInProgress = false;
    
    // 已加载网格数据
    QList<LoadedMesh> loadedMeshes;
    QHash<QTreeWidgetItem*, int> meshItemMap;

private:
    void setupLoadedMeshesTab();
    void addLoadedMesh(const QString& filePath, const MeshData& meshData);
    void updateLoadedMeshesTree();
    void onLoadedMeshSelected(QTreeWidgetItem* item, int column);
    void onLoadedMeshContextMenuRequested(const QPoint& pos);
    void exportSelectedMesh();
    void removeSelectedMesh();
    void viewSelectedMeshProperties();
};
