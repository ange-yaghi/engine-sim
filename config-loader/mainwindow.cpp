/******************************************************************************
 * A config loader with GUI.\
 *   Users select an engine and theme,\
 *   then "main.mr" will be automatically generated.
 * @File: mainwindow.cpp
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

// TODO: read old "main.mr" as default config,\
//  so users dont need to modify evething.
// TODO: add options of modifying units
// TODO: provide preview of themes
// TODO: more beautiful(icon, theme(synchronizing with engine sim), layout...)
// ...

#include "mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "./ui_mainwindow.h"
#include "selectingdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  /* set path of folders and "main.mr" */
  // assets_folder_path = "C:/Program Files/Engine Sim/assets/";
  assets_folder_path = "../assets/";
  engines_folder_path = assets_folder_path + "engines/";
  themes_folder_path = assets_folder_path + "themes/";
  main_file_path = assets_folder_path + "main.mr";

  /* check path */
  if (!QDir(assets_folder_path).exists())
  {
    QMessageBox::critical(NULL, "ERROR", "Can not find folder \"assets\".",
                          QMessageBox::Close);
    exit(EXIT_FAILURE);
  }
  if (!QDir(engines_folder_path).exists())
  {
    QMessageBox::critical(NULL, "ERROR",
                          "Can not find folder \"assets/engines\".",
                          QMessageBox::Close);
    exit(EXIT_FAILURE);
  }
  if (!QDir(themes_folder_path).exists())
  {
    QMessageBox::critical(NULL, "ERROR",
                          "Can not find folder \"assets/themes\".",
                          QMessageBox::Close);
    exit(EXIT_FAILURE);
  }

  /****************************************************************************
   *
   * TODO: read old "mian.mr", store paths and set_commends,
   *   provide default values.
   *
   ***************************************************************************/

  /* set file filter */
  filter.clear();
  filter.append("*.mr");
  /* create file system model */
  file_model = new QFileSystemModel();
  /* root path and filter */
  file_model->setRootPath(assets_folder_path);
  file_model->setNameFilters(filter);
  file_model->setNameFilterDisables(false);
  file_model->setReadOnly(false);

  /* tree view of engine files */
  engine_files_tree();
  theme_files_tree();

  /* default theme */
  theme_file_name = "default.mr";
  theme_file_path = themes_folder_path + "default.mr";
  ui->theme_file_name_browser->setText("default");
}

MainWindow::~MainWindow()
{
  delete file_model;
  delete ui;
}

/* display engine files in tree view */
void MainWindow::engine_files_tree()
{
  /* bind file system model to tree view */
  ui->engine_files_tree_view->setAnimated(true);
  ui->engine_files_tree_view->setModel(file_model);
  ui->engine_files_tree_view->setRootIndex(
      file_model->index(engines_folder_path));

  /* hide headers and columns */
  ui->engine_files_tree_view->header()->hide();
  for (int i = 1; i < 4; ++i) ui->engine_files_tree_view->hideColumn(i);
}

/******************************************************************************
 *
 * TODO: need a function "void read_main_file()".
 *   store and set default values.
 *
 *****************************************************************************/

/* display theme files in tree view */
void MainWindow::theme_files_tree()
{
  /* bind file system model to tree view */
  ui->theme_files_tree_view->setAnimated(true);
  ui->theme_files_tree_view->setModel(file_model);
  ui->theme_files_tree_view->setRootIndex(
      file_model->index(themes_folder_path));

  /* hide headers and columns */
  ui->theme_files_tree_view->header()->hide();
  for (int i = 1; i < 4; ++i) ui->theme_files_tree_view->hideColumn(i);
}

/* read engine file to get public nodes */
QString MainWindow::read_engine_file(QFile &file)
{
  /* open and read the file line-by-line */
  QStringList pub_nodes;
  QByteArray line;
  bool main_exists = false;
  pub_nodes.clear();
  line.clear();
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    QMessageBox::critical(
        NULL, "ERROR",
        "Engine file \"" + engine_file_name + "\" is not accessible.",
        QMessageBox::Ok);
  else
  {
    while (!file.atEnd())
    {
      line = file.readLine();
      /* search for public nodes */
      /* break if "main" exists */
      if (line.startsWith("public node main"))
      {
        main_exists = true;
        break;
      }
      /* store the name of public node */
      if (line.startsWith("public node")) pub_nodes.append(line.split(' ')[2]);
    }
    file.close();
  }

  QString set_engine_command;
  if (main_exists)
    set_engine_command = "main()";
  else if (pub_nodes.size() < 1)
    QMessageBox::critical(NULL, "ERROR", "Invalid engine file.",
                          QMessageBox::Ok);
  else if (pub_nodes.size() == 1)
    set_engine_command = "set_engine(" + pub_nodes[0] + "())";
  else
  {
    /* selecting dialog */
    SelectingDialog *dialog = new SelectingDialog;
    dialog->file_path = engine_file_path;
    dialog->set_item(pub_nodes);
    int dialog_ret = dialog->exec();
    if (dialog_ret == QDialog::Accepted)
    {
      if (dialog->engine_idx != 0)
        set_engine_command =
            "set_engine(" + pub_nodes[dialog->engine_idx - 1] + "())";
      if (dialog->transmission_idx != 0)
        set_engine_command += "\nset_transmission(" +
                              pub_nodes[dialog->transmission_idx - 1] + "())";
      if (dialog->vehicle_idx != 0)
        set_engine_command +=
            "\nset_vehicle(" + pub_nodes[dialog->vehicle_idx - 1] + "())";
    }
    else if (dialog_ret == QDialog::Rejected)
      set_engine_command.clear();
    delete dialog;
  }

  return set_engine_command;
}

/* read theme file to get public nodes */
QString MainWindow::read_theme_file(QFile &file)
{
  /* open and read the file line-by-line */
  QString set_theme_command;
  QStringList pub_nodes;
  QByteArray line;
  pub_nodes.clear();
  line.clear();
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    QMessageBox::critical(
        NULL, "ERROR",
        "Theme file \"" + theme_file_name + "\" is not accessible.",
        QMessageBox::Ok);
  else
  {
    while (!file.atEnd())
    {
      line = file.readLine();
      /* search for public nodes */
      if (line.startsWith("public node"))
      {
        set_theme_command = line.split(' ')[2] + '(';
        break;
      }
    }
    file.close();
  }

  return set_theme_command;
}

/* generate content of "main.mr" */
QStringList MainWindow::generate_main_content(const QString &set_theme_command,
                                              const QString &set_engine_command)
{
  QDir assets_dir(assets_folder_path);
  QStringList content;
  content.clear();
  content.append("import \"engine_sim.mr\"");
  /* import theme */
  content.append("\n/********** import theme **********/");
  content.append("import \"" + assets_dir.relativeFilePath(theme_file_path) +
                 '\"');
  /* import engine */
  content.append("\n/********** import engine **********/");
  content.append("import \"" + assets_dir.relativeFilePath(engine_file_path) +
                 '\"');
  /* set theme & units */
  content.append("\n/********** set theme & units **********/");
  content.append("unit_names units()");
  content.append(set_theme_command);

  /****************************************************************************
   *
   * TODO: modify units here, use parameters or add member vars.
   *
   ***************************************************************************/

  content.append(")");
  /* set engine */
  content.append("\n/********** set engine **********/");
  content.append(set_engine_command);

  return content;
}

/* write into "main.mr" file */
void MainWindow::write_main_file(QFile &file, const QStringList &content)
{
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    QMessageBox::critical(NULL, "ERROR", "File \"main.mr\" is not accessible.",
                          QMessageBox::Ok);
  else
  {
    QTextStream stream(&file);
    for (int i = 0; i < content.size(); ++i) stream << content[i] << "\n";
    file.close();
  }
}

/* get selected item from engine files tree */
void MainWindow::on_engine_files_tree_view_clicked(const QModelIndex &index)
{
  /* get the type of selected item */
  QString type = file_model->type(index);
  if (type == "File Folder") return;

  /* display the name of selected file */
  engine_file_name = file_model->fileName(index);
  engine_file_path = file_model->filePath(index);
  ui->engine_file_name_browser->setText(
      engine_file_name.left(engine_file_name.size() - 3));
}

/* get selected item from theme files tree */
void MainWindow::on_theme_files_tree_view_clicked(const QModelIndex &index)
{
  /* get the type of selected item */
  QString type = file_model->type(index);
  if (type == "File Folder") return;

  /* display the name of selected file */
  theme_file_name = file_model->fileName(index);
  theme_file_path = file_model->filePath(index);
  ui->theme_file_name_browser->setText(
      theme_file_name.left(theme_file_name.size() - 3));
}

/* apply and write into "main.mr" */
void MainWindow::on_apply_button_clicked()
{
  /* ensure an engine file is selected */
  if (engine_file_name.isEmpty())
  {
    QMessageBox::critical(NULL, "ERROR", "Please select an engine.",
                          QMessageBox::Ok);
    return;
  }

  /* get the selected engine file */
  QFile engine_file(engine_file_path);
  if (!engine_file.exists())
  {
    QMessageBox::critical(
        NULL, "ERROR", "Can not find engine file \"" + engine_file_name + "\".",
        QMessageBox::Ok);
    return;
  }
  /* get the selected theme file */
  QFile theme_file(theme_file_path);
  if (!theme_file.exists())
  {
    QMessageBox::critical(
        NULL, "ERROR", "Can not find theme file \"" + theme_file_name + "\".",
        QMessageBox::Ok);
    return;
  }

  /* read the engine file */
  QString set_engine_command = read_engine_file(engine_file);
  /* user selects nothing or cancels */
  if (set_engine_command.isEmpty())
  {
    QMessageBox::information(NULL, "INFO", "Nothing is written in \"main.mr\".",
                             QMessageBox::Ok);
    return;
  }
  /* read the theme file */
  QString set_theme_command = read_theme_file(theme_file);

  /* generate content of "main.mr" */
  QStringList main_content =
      generate_main_content(set_theme_command, set_engine_command);

  /* remove old backup file */
  QFile main_file_backup(main_file_path + ".backup");
  if (!main_file_backup.remove())
    QMessageBox::critical(NULL, "ERROR", main_file_backup.errorString(),
                          QMessageBox::Ok);
  /* back up "main.mr" to "main.mr.backup" before modifying */
  QFile main_file(main_file_path);
  if (!main_file.copy(main_file.fileName() + ".backup"))
    QMessageBox::critical(NULL, "ERROR", main_file.errorString(),
                          QMessageBox::Ok);

  /* write into the main file */
  write_main_file(main_file, main_content);

  /* notify */
  QMessageBox::information(
      NULL, "INFO",
      "Successful!\nOld configuration is backed up to \"main.mr.backup\".",
      QMessageBox::Ok);
}

/* quit */
void MainWindow::on_quit_button_clicked()
{
  delete file_model;
  delete ui;
  exit(EXIT_SUCCESS);
}
