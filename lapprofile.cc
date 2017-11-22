//***************************************************************************
// Group Linslot / Linux - Slotrace Manager
// File lapprofile.cc
// Date 03.03.08 - Jörg Wendel
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <QSqlQuery>
#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QMessageBox>
#include <QCheckBox>

#include <common.hpp>
#include <lapprofile.hpp>

//***************************************************************************
// RenderArea
//***************************************************************************

RenderArea::RenderArea(QWidget* parent)
   : QWidget(parent)
{
   setBackgroundRole(QPalette::Base);
   setAutoFillBackground(true);

   xScale = 10;         // pixel / 100ms
   xStart = 0;          // start time of x axis in seconds
   model = 0;
   editDialog = 0;
   tableView = 0;
   gcScale = 100.0;

   showVolt = showAmpere = showPower = yes;
}

//***************************************************************************
// Scale X Axis
//***************************************************************************

void RenderArea::scaleUpClicked(bool)
{
   xScale++;
   repaint();
}

void RenderArea::scaleDownClicked(bool)
{
   if (xScale > 2)
      xScale--;

   repaint();
}

//***************************************************************************
// Scroll
//***************************************************************************

void RenderArea::scrollLeftClicked(bool)
{
   if (xStart > 0)
      xStart--;

   repaint();
}

void RenderArea::scrollRightClicked(bool)
{
   xStart++;
   repaint();
}

//***************************************************************************
// View Flags
//***************************************************************************

void RenderArea::checkVolt(int state)
{
   showVolt = state;
   repaint();
}

void RenderArea::checkAmpere(int state)
{
   showAmpere = state;
   repaint();
}

void RenderArea::checkPower(int state)
{
   showPower = state;
   repaint();
}

//***************************************************************************
// Get Color
//***************************************************************************

void RenderArea::getColor(bool)
{
   QColor color = colorButton->palette().color(QPalette::Button);

   color = QColorDialog::getColor(color, editDialog);

   if (color.isValid())
   {
      QPalette p = colorButton->palette();
      p.setColor(QPalette::Button, color);
      colorButton->setPalette(p);
   }
}

//***************************************************************************
// Edit
//***************************************************************************

void RenderArea::editClicked(bool)
{
   QColor color;
   int row = tableView->currentIndex().row();
   int profileId = model->index(row, model->fieldIndex("PROFILE_ID")).data().toInt();

   QSqlQuery query("select NAME, COLOR, ACTIVE from profiles where PROFILE_ID = '" +
               QString::number(profileId) + "';");

   query.next();

   QString name = query.value(0).toString();
   QString colorName = query.value(1).toString();
   int checked = query.value(2).toBool();

   color.setNamedColor(colorName);

   editDialog = new QDialog(this);
   colorButton = new QPushButton(this);

   QGridLayout* layout = new QGridLayout;
   QLineEdit* nameEdit = new QLineEdit(this);
   QLabel* label1 = new QLabel(this);
   QLabel* label2 = new QLabel(this);
   QCheckBox* checkBoxActive = new QCheckBox(this);
   QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                      QDialogButtonBox::Cancel);
   label1->setText("Name:");
   label2->setText("Farbe:");
   colorButton->setText("");
   nameEdit->setText(name);
   checkBoxActive->setText("in Fahrerliste");
   checkBoxActive->setChecked(checked);

   QPalette p = colorButton->palette();
   p.setColor(QPalette::Button, color);
   colorButton->setPalette(p);

   layout->addWidget(label1, 0, 0);
   layout->addWidget(nameEdit, 1, 0);
   layout->addWidget(label2, 2, 0);
   layout->addWidget(colorButton, 3, 0);
   layout->addWidget(checkBoxActive, 4, 0);
   layout->addWidget(buttonBox, 5, 0);

   connect(buttonBox, SIGNAL(accepted()), editDialog, SLOT(accept()));
   connect(buttonBox, SIGNAL(rejected()), editDialog, SLOT(reject()));
   connect(colorButton, SIGNAL(clicked(bool)), this, SLOT(getColor(bool)));

   editDialog->setWindowTitle("Eigenschaften");
   editDialog->setLayout(layout);
   editDialog->resize(300, 150);

   if (editDialog->exec() == QDialog::Accepted)
   {
      tell(eloAlways, "accepted");

      QSqlQuery query("update profiles set "
                      "NAME = '" + nameEdit->text() + "', "
                      "ACTIVE = '" + (checkBoxActive->isChecked() ? "true" : "false") + "', "
                      "COLOR = '" + colorButton->palette().color(QPalette::Button).name() + "' "
                      "where PROFILE_ID = " + QString::number(profileId) + ";");

      model->select();
   }

   delete editDialog;
}

//***************************************************************************
// Remove
//***************************************************************************

void RenderArea::removeClicked(bool)
{
   int row = tableView->currentIndex().row();
   int profileId = model->index(row, model->fieldIndex("PROFILE_ID")).data().toInt();
   QString name = model->index(row, model->fieldIndex("NAME")).data().toString();

   if (QMessageBox::question(this, "Löschen", "'" + name + "' löschen?",
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
   {
      QSqlQuery query;

      query.exec("delete from profiles where PROFILE_ID = "
                 + QString::number(profileId) + ";");

      query.exec("delete from lap_profiles where PROFILE_ID = "
                 + QString::number(profileId) + ";");

      model->select();
   }
}

//***************************************************************************
// Double Clicked
//***************************************************************************

void RenderArea::doubleClicked(const QModelIndex& index)
{
   Line line;
   int count = 0;

   int profileId = model->index(index.row(), model->fieldIndex("PROFILE_ID")).data().toInt();

   QSqlQuery query("select NAME, COLOR from profiles where PROFILE_ID = '" +
               QString::number(profileId) + "';");

   query.next();

   QString name = query.value(0).toString();
   QString colorName = query.value(1).toString();

   // already in list .. ?

   for (int l = 0; l < lines.size(); l++)
   {
      if (lines.at(l).name == name)
      {
         lines.removeAt(l);
         repaint();
         tell(eloAlways, "removed from list");

         return ;
      }
   }

   // add to drwaing list

   QSqlQuery q("select VOLT, AMPERE from lap_profiles where PROFILE_ID = '" +
               QString::number(profileId) + "';");

   while (q.next())
   {
      line.volts.append(q.value(0).toInt());
      line.amperes.append(q.value(1).toInt());
      count++;
   }

   if (count)
   {
      line.color.setNamedColor(colorName);

      line.name = name;
      lines.append(line);

      const char* n = line.color.name().toLatin1();
      tell(eloAlways, "added (%d) values; color '%s', now (%d) lines in list",
           count, n, lines.size());

      repaint();
   }
}

//***************************************************************************
// Paint Axis
//***************************************************************************

void RenderArea::paintXAxis()
{
   // draw x axis

   painter->drawLine(xNull, yNull, xNull + xWidth, yNull);
   int sec = xStart;

   for (int x = 0; x < xWidth; x += xScale)
   {
      if (!(x % (10*xScale)))
      {
         painter->drawLine(xNull + x, yNull-4, xNull + x, yNull+4);
         painter->drawText(xNull + x-5, yNull+16, QString::number(sec++));
      }
      else
         painter->drawLine(xNull + x, yNull-2, xNull + x, yNull+2);
   }
}

void RenderArea::paintYAxis()
{
   painter->drawLine(xNull, yNull, xNull, yNull-yHeight);

   for (int y = 0; y < yHeight; y += yScale)
   {
      if (!(y % (5*yScale)))
      {
         painter->drawLine(xNull-4, yNull-y, xNull+4, yNull-y);
         painter->drawText(xNull-30, yNull-y+5, QString::number(y/yScale));
      }
      else
         painter->drawLine(xNull-2, yNull-y, xNull+2, yNull-y);
   }
}


//***************************************************************************
// Paint Event
//***************************************************************************

void RenderArea::paintEvent(QPaintEvent* )
{
   xNull = 40;
   xWidth = width() - xNull - 5;
   yNull = height() - 30;
   yHeight = height() - 30 -5;
   yScale = (int)(((double)yHeight)/100.0);   // alle 5% ein step

   painter = new QPainter(this);

   //

   paintXAxis();
   paintYAxis();

   // draw graph

   int iOff = (int)(xStart * 1000.0 / gcScale);

   //

   for (int l = 0; l < lines.size(); l++)
   {
      QPen pen = painter->pen();
      int yVNow = yNull;
      int yANow = yNull;
      int yPNow = yNull;
      int xDiff = (int)((double)xScale * (gcScale / 100.0));
      int xNow = xNull;

      pen.setColor(lines.at(l).color);

      if (lines.at(l).volts.size() > iOff)
      {
         for (int i = iOff; i < lines.at(l).volts.size(); i++)
         {
            int yVLast = yVNow;
            int yALast = yANow;
            int yPLast = yPNow;

            double p = lines.at(l).volts.at(i) * lines.at(l).amperes.at(i);

            int vp = (int)((double)lines.at(l).volts.at(i) / 255.0 * 100.0);
            int ap = (int)((double)lines.at(l).amperes.at(i) / 255.0 * 100.0);
            int pp = (int)(p / 255.0 * 100.0) / 150;  // 150 -> scale to fit window

            yVNow = yNull-(vp * yScale);
            yANow = yNull-(ap * yScale);
            yPNow = yNull-(pp * yScale);

            if (showVolt)
            {
               pen.setStyle(Qt::DashLine);
               painter->setPen(pen);
               painter->drawLine(xNow, yVLast, xNow+xDiff, yVNow);
            }
            if (showAmpere)
            {
               pen.setStyle(Qt::DotLine);
               painter->setPen(pen);
               painter->drawLine(xNow, yALast, xNow+xDiff, yANow);
            }
            if (showPower)
            {
               pen.setStyle(Qt::SolidLine);
               painter->setPen(pen);
               painter->drawLine(xNow, yPLast, xNow+xDiff, yPNow);
            }

            xNow += xDiff;

            if (i > xWidth)
               break;
         }
      }
   }

   // draw frame rect

   painter->setPen(palette().dark().color());
   painter->setBrush(Qt::NoBrush);
   painter->drawRect(QRect(0, 0, width() - 1, height() - 1));

   delete painter;
}
