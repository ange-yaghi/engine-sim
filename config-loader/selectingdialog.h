/******************************************************************************
 * A config loader with GUI.\
 *   Users select an engine and theme,\
 *   then "main.mr" will be automatically generated.
 * @File: selectingdialog.h
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

#ifndef SELECTINGDIALOG_H
#define SELECTINGDIALOG_H

#include <QDialog>

namespace Ui
{
class SelectingDialog;
}

class SelectingDialog : public QDialog
{
  Q_OBJECT

 public:
  explicit SelectingDialog(QWidget *parent = nullptr);
  ~SelectingDialog();

  /* set item for combo boxes */
  void set_item(const QStringList &pub_nodes);

  /* user selection */
  int engine_idx;
  int transmission_idx;
  int vehicle_idx;

  /* file path */
  QString file_path;

 private slots:
  /* get user selection */
  void on_engine_selector_activated(int index);
  void on_transmission_selector_activated(int index);
  void on_vehicle_selector_activated(int index);

  /* open the file */
  void on_open_button_clicked();

 private:
  Ui::SelectingDialog *ui;
};

#endif  // SELECTINGDIALOG_H
