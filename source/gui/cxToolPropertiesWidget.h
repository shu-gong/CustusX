/*
 * cxToolWidget.h
 *
 *  Created on: Apr 22, 2010
 *      Author: christiana
 */
#ifndef CXTOOLPROPERTIESWIDGET_H_
#define CXTOOLPROPERTIESWIDGET_H_

#include <vector>
#include <QtGui>
#include "sscForwardDeclarations.h"

namespace cx
{

class SliderGroupWidget;

/**
 * \class ToolPropertiesWidget
 *
 * \date 2010.04.22
 * \author: Christian Askeland, SINTEF
 */
class ToolPropertiesWidget : public QWidget
{
  Q_OBJECT

public:
  ToolPropertiesWidget(QWidget* parent);
  virtual ~ToolPropertiesWidget();

signals:

protected slots:
  void updateSlot();
  void dominantToolChangedSlot();
  void referenceToolChangedSlot();

protected:
  virtual void showEvent(QShowEvent* event); ///<updates internal info before showing the widget
  virtual void hideEvent(QCloseEvent* event); ///<disconnects stuff

private:
  ToolPropertiesWidget();

  ssc::ToolPtr mReferenceTool;
  ssc::ToolPtr mActiveTool;

  SliderGroupWidget* mToolOffsetWidget;
  QLabel* mActiveToolVisibleLabel;
  QLabel* mToolNameLabel;
  QLabel* mReferenceStatusLabel;
  QLabel* mTrackingSystemStatusLabel;
};

}//end namespace cx


#endif /* CXTOOLPROPERTIESWIDGET_H_ */
