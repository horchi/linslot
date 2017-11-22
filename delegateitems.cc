//***************************************************************************
// Group
// File delegateitems.cc
// Date 05.12.06
//***************************************************************************

//***************************************************************************
// Include
//***************************************************************************

#include <QBrush>
#include <QPainter>
#include <QLocale>
#include <QCalendarWidget>
#include <QKeyEvent>
#include <QItemDelegate>
#include <QApplication>
#include <QComboBox>

#include <delegateitems.hpp>

//***************************************************************************
// Class ExtItemDelegate
//***************************************************************************

QSizeF ExtItemDelegate::doTextLayout(int lineWidth) const
{
   QFontMetrics fontMetrics(textLayout.font());
   int leading = fontMetrics.leading();
   qreal height = 0;
   qreal widthUsed = 0;
   textLayout.beginLayout();

   while (true)
   {
      QTextLine line = textLayout.createLine();

      if (!line.isValid())
         break;

      line.setLineWidth(lineWidth);
      height += leading;
      line.setPosition(QPointF(0, height));
      height += line.height();
      widthUsed = qMax(widthUsed, line.naturalTextWidth());
   }

   textLayout.endLayout();
   return QSizeF(widthUsed, height);
}

void ExtItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem & option,
                            const QModelIndex & index) const
{
   QStyleOptionViewItem opt = setOptions(index, option);
   const QStyleOptionViewItem *v2 = qstyleoption_cast<const QStyleOptionViewItem *>(&option);
   opt.features = v2 ? v2->features : QStyleOptionViewItem::ViewItemFeatures(QStyleOptionViewItem::None);

   painter->save();

   QVariant value;
   QPixmap pixmap;
   QRect decorationRect;
   QString text;
   QRect displayRect;

   value = index.data(Qt::DecorationRole);

   if (value.isValid())
   {
      pixmap = decoration(opt, value);
      decorationRect = QRect(QPoint(0, 0), pixmap.size());
   }

   value = index.data(Qt::DisplayRole);

   if (value.isValid())
   {
      if (value.type() == QVariant::Double)
         text = QLocale().toString(value.toDouble());
      else
         text = value.toString();

      displayRect = textRectangle(painter, opt.rect, opt.font, text);
   }

   QRect checkRect;
   Qt::CheckState checkState = Qt::Unchecked;

   value = index.data(Qt::CheckStateRole);

   if (value.isValid())
   {
      checkState = static_cast<Qt::CheckState>(value.toInt());
      //checkRect = check(opt, opt.rect, value);  // check is missing in QT5 - needed??
   }

   doLayout(opt, &checkRect, &decorationRect, &displayRect, false);

   drawBackglap(painter, opt, index);

   drawCheck(painter, opt, checkRect, checkState);
   drawDecoration(painter, opt, decorationRect, pixmap);
   // opt.state &= ~QStyle::State_Selected;                   // due to bug in QItemDelegate::drawDisplay(...)
   drawDisplay(painter, opt, displayRect, text);
   drawFocus(painter, opt, text.isEmpty() ? QRect() : displayRect);

   painter->restore();
}

void ExtItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                                  const QRect &rect, const QString &text) const
{
   if (text.isEmpty())
      return;

   QPen pen = painter->pen();
   QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                             ? QPalette::Normal : QPalette::Disabled;
   if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
      cg = QPalette::Inactive;
   if (option.state & QStyle::State_Selected)
   {
      // painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight)); due to this draw display is overwritten !!! :((
      painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
   }
   else
   {
      painter->setPen(option.palette.color(cg, QPalette::Text));
   }

   if (option.state & QStyle::State_Editing)
   {
      painter->save();
      painter->setPen(option.palette.color(cg, QPalette::Text));
      painter->drawRect(rect.adjusted(0, 0, -1, -1));
      painter->restore();
   }

   const QStyleOptionViewItem opt = option;
   const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
   QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
   const bool wrapText = opt.features & QStyleOptionViewItem::WrapText;
   textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
   textOption.setTextDirection(option.direction);
   textOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
   textLayout.setTextOption(textOption);
   textLayout.setFont(option.font);
   textLayout.setText(replaceNewLine(text));

   QSizeF textLayoutSize = doTextLayout(textRect.width());

   if (textRect.width() < textLayoutSize.width()
         || textRect.height() < textLayoutSize.height())
   {
      QString elided;
      int start = 0;
      int end = text.indexOf(QChar::LineSeparator, start);

      if (end == -1)
      {
         elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
      }
      else
      {
         while (end != -1)
         {
            elided += option.fontMetrics.elidedText(text.mid(start, end - start),
                                                    option.textElideMode, textRect.width());
            start = end + 1;
            end = text.indexOf(QChar::LineSeparator, start);
         }
      }

      textLayout.setText(elided);
      textLayoutSize = doTextLayout(textRect.width());
   }

   const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
   const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                            layoutSize, textRect);
   textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
}

void ExtItemDelegate::drawBackglap(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
   QItemDelegate::drawBackground(painter, option, index);
}

//***************************************************************************
// ComboBoxDelegate
//***************************************************************************
//***************************************************************************
// Object
//***************************************************************************

ComboBoxDelegate::ComboBoxDelegate(QObject* parent)
      : ExtItemDelegate(parent)
{}

QWidget* ComboBoxDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem &/*option*/,
                                        const QModelIndex& /*index*/) const
{
   QComboBox* editor = new QComboBox(parent);

   for (int i = 0; i < items.size(); ++i)
      editor->addItem(items.at(i).title);

   editor->installEventFilter(const_cast<ComboBoxDelegate*>(this));

   return editor;
}

void ComboBoxDelegate::addItem(QString s, QRgb bgColor, QRgb fgColor)
{
   SelectionItem item;

   item.title = s;
   item.bgColor = bgColor;
   item.fgColor = fgColor;

   items.append(item);
}

void ComboBoxDelegate::setEditorData(QWidget* editor,
                                     const QModelIndex &index) const
{
   QString value = index.model()->data(index, Qt::DisplayRole).toString();
   QComboBox* comboBox = static_cast<QComboBox*>(editor);
   int i = comboBox->findText(value);
   comboBox->setCurrentIndex(i);
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex &index) const
{
   QComboBox* comboBox = static_cast<QComboBox*>(editor);
   QString value = comboBox->currentText();

   model->setData(index, value);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor,
      const QStyleOptionViewItem &option,
      const QModelIndex &/*index*/) const
{
   editor->setGeometry(option.rect);
}

void ComboBoxDelegate::drawBackglap(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
   QColor c;

   ExtItemDelegate::drawBackglap(painter, option, index);

   if (!getColorFor(index, c))
      return ;

   QPointF oldBo = painter->brushOrigin();
   QRect r = option.rect;
   QBrush b = painter->brush();

   painter->setBrushOrigin(option.rect.topLeft());

   b.setColor(c);
   b.setStyle(Qt::SolidPattern);
   r.adjust(2,5,-2,-5);

   QPen p = painter->pen();
   p.setColor(c);
   p.setWidth(3);
   painter->setPen(p);

   painter->drawRoundRect(r, 5, 5);
   painter->fillRect(r, b);
   painter->setBrushOrigin(oldBo);
}

int ComboBoxDelegate::getColorFor(const QModelIndex &index, QColor &color) const
{
   QVariant value = index.data(Qt::DisplayRole);

   if (value.isValid())
   {
      int i;

      for (i = 0; i < items.size(); ++i)
         if (items.at(i).title == value.toString())
         {
            color = QColor(items.at(i).bgColor);
            break;
         }

      if (i >= items.size() || color == Qt::white)
         return false;

      return true;
   }

   return false;
}
