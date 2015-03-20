//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File lapprofile.hpp
// Date 03.03.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <QWidget>
#include <QPen>
#include <QPainter>
#include <QModelIndex>
#include <QSqlTableModel>
#include <QPushButton>
#include <QTableView>

//***************************************************************************
// class RenderArea
//***************************************************************************

class RenderArea : public QWidget
{
      Q_OBJECT

   public:

      struct Line
      {
         QString name;
         QColor color;
         QList<int> volts;
         QList<int> amperes;
      };

      RenderArea(QWidget *parent = 0);

      void setTableView(QTableView* v) { tableView = v, model = (QSqlTableModel*)tableView->model(); }
      void setGcScale(int scale)       { gcScale = scale; }

   public slots:

      void scaleUpClicked(bool checked);
      void scaleDownClicked(bool checked);
      void scrollLeftClicked(bool checked);
      void scrollRightClicked(bool checked);
      void editClicked(bool checked);
      void removeClicked(bool checked);
      void getColor(bool checked);
      void checkVolt(int state);
      void checkAmpere(int state);
      void checkPower(int state);
      void doubleClicked(const QModelIndex& index);

   protected:

      void paintEvent(QPaintEvent *event);
      void paintXAxis();
      void paintYAxis();

      // data

      QPainter* painter;
      int xNull;
      int yNull;
      int xWidth;
      int yHeight;
      int yScale;

      int showVolt;
      int showAmpere;
      int showPower;
      QList<Line> lines;
      int xScale;
      int xStart;
      QSqlTableModel* model;
      QTableView* tableView;
      double gcScale;

      // edit dialog stuff

      QDialog* editDialog;
      QPushButton* colorButton;
};
