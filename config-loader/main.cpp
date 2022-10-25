/******************************************************************************
 * A config loader with GUI.\
 *   Users select an engine and theme,\
 *   then "main.mr" will be automatically generated.
 * @File: main.cpp
 * @Author: Gol3vka<gol3vka@163.com>
 * @Version: 2.0.0
 * @What's new:
 *   1) Modifying theme is available.\
 *   2) Add a dialog for engine files where the program can not decide\
 *      which is the engine node.
 * @Created date: 2022/10/21
 * @Last modified date: 2022/10/25
 *****************************************************************************/

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  w.show();
  return a.exec();
}
