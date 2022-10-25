/******************************************************************************
 * A config loader with GUI.\
 *   Users select an engine and theme,\
 *   then "main.mr" will be automatically generated.
 * @File: mainwindow.h
 * @Brief: Mainwindow.
 * @Author: Gol3vka<gol3vka@163.com>
 * @Version: 2.0.0
 * @What's new:
 *   1) Modifying theme is available.\
 *   2) Add a dialog for engine files where the program can not decide\
 *      which is the engine node.
 * @Created date: 2022/10/21
 * @Last modified date: 2022/10/25
 *****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QFileSystemModel>
#include <QMainWindow>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private slots:
  /* get selected item from engine files tree */
  void on_engine_files_tree_view_clicked(const QModelIndex &index);
  /* get selected item from theme files tree */
  void on_theme_files_tree_view_clicked(const QModelIndex &index);

  /* apply and write into "main.mr" */
  void on_apply_button_clicked();
  /* quit */
  void on_quit_button_clicked();

 private:
  /* display engine files in tree view */
  void engine_files_tree();
  /* display theme files in tree view */
  void theme_files_tree();

  /* read engine file to get public nodes */
  QString read_engine_file(QFile &file);
  /* read theme file to get public nodes */
  QString read_theme_file(QFile &file);

  /* generate content of "main.mr" */
  QStringList generate_main_content(const QString &set_theme_command,
                                    const QString &set_engine_command);
  /* write into "main.mr" file */
  void write_main_file(QFile &file, const QStringList &content);

  Ui::MainWindow *ui;

  QFileSystemModel *file_model;
  QStringList filter;

  QString assets_folder_path;
  QString engines_folder_path;
  QString themes_folder_path;
  QString main_file_path;

  QString engine_file_name;
  QString engine_file_path;
  QString theme_file_name;
  QString theme_file_path;
};
#endif  // MAINWINDOW_H
