//***************************************************************************
// Group
// File delegateitems.hpp
// Date 05.12.06
//***************************************************************************

#ifndef _DELEGATE_H_
#define _DELEGATE_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <QItemDelegate>
#include <QTextLayout>
#include <QTableView>
#include <QTextOption>

//***************************************************************************
// Class ExtItemDelegate
//***************************************************************************

class ExtItemDelegate : public QItemDelegate
{
   Q_OBJECT

   public:

      ExtItemDelegate(QObject* parent = 0) :  QItemDelegate(parent)  {}

   protected:

      virtual void paint(QPainter* painter, const QStyleOptionViewItem & option,
                         const QModelIndex & index ) const;

      virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                               const QRect &rect, const QString &text) const;

      virtual void drawBackglap(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const;

      QSizeF doTextLayout(int lineWidth) const;

      inline static QString replaceNewLine(QString text)
      {
         const QChar nl = QLatin1Char('\n');
         for (int i = 0; i < text.count(); ++i)
            if (text.at(i) == nl)
               text[i] = QChar::LineSeparator;
         return text;
      }

      // data

      mutable QTextLayout textLayout;
      mutable QTextOption textOption;
};

//***************************************************************************
// Class ComboBoxDelegate
//***************************************************************************

class ComboBoxDelegate : public ExtItemDelegate
{
      Q_OBJECT

   public:

      // declarations

      struct SelectionItem
      {
         QString title;
         QRgb bgColor;
         QRgb fgColor;
      };

      // object

      ComboBoxDelegate(QObject* parent = 0);

      // interface

      QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const;

      void setEditorData(QWidget* editor, const QModelIndex &index) const;

      void setModelData(QWidget* editor, QAbstractItemModel* model,
                        const QModelIndex &index) const;

      void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const;

      void addItem(QString s, QRgb bgColor = 0xFFFFFFFF, QRgb fgColor = 0xFF000000);

   protected:

      int getColorFor(const QModelIndex &index, QColor &bgColor) const;

      void drawBackglap(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

      QList<SelectionItem> items;
};

//***************************************************************************
#endif //  _DELEGATE_H_
