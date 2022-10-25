/******************************************************************************
 * A config loader with GUI.\
 *   Users select an engine and theme,\
 *   then "main.mr" will be automatically generated.
 * @File: selectingdialog.cpp
 * @Brife: A dialog for users to select engine, transmission and vehicle nodes.
 * @Author: Gol3vka<gol3vka@163.com>
 * @Version: 2.0.0
 * @What's new:
 *   1) Modifying theme is available.\
 *   2) Add a dialog for engine files where the program can not decide\
 *      which is the engine node.
 * @Created date: 2022/10/21
 * @Last modified date: 2022/10/25
 *****************************************************************************/

#include "selectingdialog.h"

#include <QDesktopServices>
#include <QDir>
#include <QUrl>

#include "ui_selectingdialog.h"

SelectingDialog::SelectingDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SelectingDialog)
{
  ui->setupUi(this);
  engine_idx = 0;
  transmission_idx = 0;
  vehicle_idx = 0;
}

SelectingDialog::~SelectingDialog() { delete ui; }

/* set item for combo boxes */
void SelectingDialog::set_item(const QStringList &pub_nodes)
{
  ui->engine_selector->addItem("--None--");
  ui->transmission_selector->addItem("--None--");
  ui->vehicle_selector->addItem("--None--");

  ui->engine_selector->addItems(pub_nodes);
  ui->transmission_selector->addItems(pub_nodes);
  ui->vehicle_selector->addItems(pub_nodes);
}

/* get user selection */
void SelectingDialog::on_engine_selector_activated(int index)
{
  engine_idx = index;
}
void SelectingDialog::on_transmission_selector_activated(int index)
{
  transmission_idx = index;
}
void SelectingDialog::on_vehicle_selector_activated(int index)
{
  vehicle_idx = index;
}

/* open the file */
void SelectingDialog::on_open_button_clicked()
{
  QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(file_path)));
}
