/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
   may be used to endorse or promote products derived from this software 
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#ifndef CXTOOLPROPERTIESWIDGET_H_
#define CXTOOLPROPERTIESWIDGET_H_

#include "cxGuiExport.h"

#include "cxBaseWidget.h"

#include <vector>
#include "cxForwardDeclarations.h"
#include "cxDoubleWidgets.h"
#include "cxSpaceProperty.h"
#include "cxStringProperty.h"
#include "cxTransform3DWidget.h"
#include "cxPointMetric.h"


class QCheckBox;
class QGroupBox;

class UsConfigGui;

namespace cx
{
class LabeledComboBoxWidget;

/**
 * \class ToolPropertiesWidget
 * \ingroup cx_gui
 *
 * \date 2010.04.22
 * \\author Christian Askeland, SINTEF
 */
class cxGui_EXPORT ToolPropertiesWidget : public BaseWidget
{
  Q_OBJECT

public:
  ToolPropertiesWidget(QWidget* parent);
  virtual ~ToolPropertiesWidget();

  virtual QString defaultWhatsThis() const;

signals:

protected slots:
  void updateSlot();
  void dominantToolChangedSlot();
  void referenceToolChangedSlot();
//  void configurationChangedSlot(int index);
//  void toolsSectorConfigurationChangedSlot();///< Update the combo box when the tools configuration is changed outside the widget. Also used initially to read the tools value.
  void manualToolChanged();
  void manualToolWidgetChanged();
  void spacesChangedSlot();

protected:
  virtual void showEvent(QShowEvent* event); ///<updates internal info before showing the widget
  virtual void hideEvent(QCloseEvent* event); ///<disconnects stuff

private:
  ToolPropertiesWidget();
//  void populateUSSectorConfigBox();

  ToolPtr mReferenceTool;
  ToolPtr mActiveTool;

  QVBoxLayout* mToptopLayout;
  QGroupBox* mManualGroup;
  Transform3DWidget* mManualToolWidget;
  SpaceDataAdapterXmlPtr mSpaceSelector;

//  SliderGroupWidget* mToolOffsetWidget;
  QLabel* mActiveToolVisibleLabel;
  QLabel* mToolNameLabel;
  QLabel* mReferenceStatusLabel;
  QLabel* mTrackingSystemStatusLabel;
  
  LabeledComboBoxWidget* mUSSectorConfigBox;
//  QLabel* mUSSectorConfigLabel;   ///< Label for the mUSSectorConfigBox
//  QComboBox* mUSSectorConfigBox;  ///< List of US sector config parameters: depth (and width)
};

}//end namespace cx


#endif /* CXTOOLPROPERTIESWIDGET_H_ */
