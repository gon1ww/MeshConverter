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

class transform : public QMainWindow {
    Q_OBJECT

public:
    transform(QWidget* parent = nullptr);
    ~transform();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

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
};
