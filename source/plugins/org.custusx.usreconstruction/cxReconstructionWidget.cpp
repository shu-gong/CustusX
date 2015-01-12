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
#include "cxReconstructionWidget.h"

#include "cxReconstructOutputValueParamsInterfaces.h"
#include "cxLogger.h"
#include "cxHelperWidgets.h"
#include "cxTypeConversions.h"
#include "cxTimedAlgorithmProgressBar.h"
#include "cxSettings.h"
#include "cxTimedAlgorithm.h"
#include "cxStringProperty.h"
#include "cxDoubleProperty.h"
#include "cxBoolProperty.h"
#include "cxUsReconstructionService.h"

namespace cx
{

ReconstructionWidget::ReconstructionWidget(QWidget* parent, UsReconstructionServicePtr reconstructer) :
	BaseWidget(parent, "USReconstruction", "US Reconstruction"),
	mReconstructer(reconstructer),
	mFileSelectWidget( new FileSelectWidget(this))
{
	connect(mReconstructer.get(), &UsReconstructionService::reconstructAboutToStart, this, &ReconstructionWidget::reconstructAboutToStartSlot);
	connect(mReconstructer.get(), &UsReconstructionService::reconstructStarted, this, &ReconstructionWidget::reconstructStartedSlot);
	connect(mReconstructer.get(), &UsReconstructionService::reconstructFinished, this, &ReconstructionWidget::reconstructFinishedSlot);

	connect(mReconstructer.get(), &UsReconstructionService::paramsChanged, this, &ReconstructionWidget::paramsChangedSlot);
	connect(mReconstructer.get(), &UsReconstructionService::inputDataSelected, this, &ReconstructionWidget::inputDataSelected);
	connect(mReconstructer.get(), &UsReconstructionService::algorithmChanged, this, &ReconstructionWidget::repopulateAlgorithmGroup);


	QVBoxLayout* topLayout = new QVBoxLayout(this);

	connect(mFileSelectWidget, &FileSelectWidget::fileSelected, this, &ReconstructionWidget::selectData);
	mFileSelectWidget->setNameFilter(QStringList() << "*.fts");
	connect(mReconstructer.get(), &UsReconstructionService::newInputDataAvailable, mFileSelectWidget, &FileSelectWidget::refresh);
	connect(mReconstructer.get(), &UsReconstructionService::newInputDataPath, this, &ReconstructionWidget::updateFileSelectorPath);

	QHBoxLayout* extentLayout = new QHBoxLayout;
	mExtentLineEdit = new QLineEdit(this);
	mExtentLineEdit->setReadOnly(true);
	extentLayout->addWidget(new QLabel("Extent", this));
	extentLayout->addWidget(mExtentLineEdit);

	mInputSpacingLineEdit = new QLineEdit(this);
	mInputSpacingLineEdit->setReadOnly(true);
	QLabel* inputSpacingLabel = new QLabel("Spacing In", this);

	mReconstructButton = new QPushButton("Reconstruct", this);
	connect(mReconstructButton, &QPushButton::clicked, this, &ReconstructionWidget::reconstruct);

	mTimedAlgorithmProgressBar = new cx::TimedAlgorithmProgressBar;

	QGridLayout* sizesLayout = new QGridLayout;
	QWidget* maxVolSizeWidget = sscCreateDataWidget(this, mReconstructer->getParam("Volume Size"));
	sizesLayout->addWidget(inputSpacingLabel, 0, 0);
	sizesLayout->addWidget(mInputSpacingLineEdit, 0, 1);
	sizesLayout->addLayout(extentLayout, 0, 2);
	mSpacingWidget = new SpinBoxGroupWidget(this, DoubleDataAdapterPtr(new DoubleDataAdapterSpacing(mReconstructer)), sizesLayout, 1);
	sizesLayout->addWidget(maxVolSizeWidget, 1, 2);

	QHBoxLayout* runLayout = new QHBoxLayout;
	topLayout->addLayout(runLayout);
	runLayout->addWidget(mReconstructButton);
	this->createAction(this,
	      QIcon(":/icons/open_icon_library/system-run-5.png"),
	      "Details", "Show Advanced Settings",
	      SLOT(toggleDetailsSlot()),
	      runLayout);

	topLayout->addWidget(mFileSelectWidget);
	topLayout->addLayout(sizesLayout);

	QWidget* presetTFWidget = sscCreateDataWidget(this, mReconstructer->getParam("Preset"));
	topLayout->addWidget(presetTFWidget);

	mOptionsWidget = this->createOptionsWidget();
	mOptionsWidget->setVisible(settings()->value("reconstruction/guiShowDetails").toBool());
	topLayout->addWidget(mOptionsWidget);

	topLayout->addStretch();
	topLayout->addWidget(mTimedAlgorithmProgressBar);
}

void ReconstructionWidget::updateFileSelectorPath(QString path)
{
	mFileSelectWidget->setPath(path);
	mFileSelectWidget->refresh();
}

QString ReconstructionWidget::defaultWhatsThis() const
{
	return "<html>"
		"<h3>US 3D Reconstruction.</h3>"
		"<p><i>Reconstruct 3D US data from acquired a 2D sequence.</i></br>"
		"</html>";
}

void ReconstructionWidget::toggleDetailsSlot()
{
  mOptionsWidget->setVisible(!mOptionsWidget->isVisible());
  settings()->setValue("reconstruction/guiShowDetails", mOptionsWidget->isVisible());
}

QWidget* ReconstructionWidget::createOptionsWidget()
{
	QWidget* retval = new QWidget(this);
	QGridLayout* layout = new QGridLayout(retval);
	layout->setMargin(0);

	int line = 0;

	layout->addWidget(this->createHorizontalLine(), line, 0, 1, 2);
	++line;

	mAlgorithmGroup = new QFrame(this);
	mAlgorithmGroup->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
	mAlgorithmGroup->setSizePolicy(mAlgorithmGroup->sizePolicy().horizontalPolicy(),QSizePolicy::Fixed);

	QVBoxLayout* algoOuterLayout = new QVBoxLayout(mAlgorithmGroup);
	QWidget* algorithmWidget = sscCreateDataWidget(this, mReconstructer->getParam("Algorithm"));
	algoOuterLayout->addWidget(algorithmWidget);
	mAlgoLayout = new QStackedLayout;
	this->repopulateAlgorithmGroup();
	algoOuterLayout->addLayout(mAlgoLayout);
	//algoOuterLayout->addStretch();
	layout->addWidget(mAlgorithmGroup, line, 0, 1, 2);
	++line;

	sscCreateDataWidget(this, mReconstructer->getParam("Angio data"), layout, line++);
	sscCreateDataWidget(this, mReconstructer->getParam("Dual Angio"), layout, line++);
	sscCreateDataWidget(this, mReconstructer->getParam("Orientation"), layout, line++);
	layout->addWidget(this->createHorizontalLine(), line++, 0, 1, 2);

	mDimXWidget = new SpinBoxGroupWidget(this, DoubleDataAdapterPtr(new DoubleDataAdapterXDim(mReconstructer)));
	mDimYWidget = new SpinBoxGroupWidget(this, DoubleDataAdapterPtr(new DoubleDataAdapterYDim(mReconstructer)));
	mDimZWidget = new SpinBoxGroupWidget(this, DoubleDataAdapterPtr(new DoubleDataAdapterZDim(mReconstructer)));
	QHBoxLayout* outputVolDimLayout = new QHBoxLayout;
	outputVolDimLayout->addWidget(mDimXWidget);
	outputVolDimLayout->addWidget(mDimYWidget);
	outputVolDimLayout->addWidget(mDimZWidget);
	layout->addLayout(outputVolDimLayout, line++, 0, 1, 2);

	sscCreateDataWidget(this, mReconstructer->getParam("Align timestamps"), layout, line++);
	sscCreateDataWidget(this, mReconstructer->getParam("Extra Temporal Calib"), layout, line++);
	sscCreateDataWidget(this, mReconstructer->getParam("Reduce mask (% in 1D)"), layout, line++);

	return retval;
}

void ReconstructionWidget::repopulateAlgorithmGroup()
{
	QString algoName = mReconstructer->getParam("Algorithm")->getValueAsVariant().toString();

	if (mAlgoLayout->currentWidget() && (algoName == mAlgoLayout->currentWidget()->objectName()))
		return;

	for (int i=0; i<mAlgoLayout->count(); ++i)
	{
		if (algoName==mAlgoLayout->widget(i)->objectName())
		{
			mAlgoLayout->setCurrentIndex(i);
			return;
		}
	}
	this->createNewStackedWidget(algoName);
}

void ReconstructionWidget::createNewStackedWidget(QString algoName)
{
	QWidget* oneAlgoWidget = new QWidget(this);
	oneAlgoWidget->setObjectName(algoName);
	mAlgoLayout->addWidget(oneAlgoWidget);
	QGridLayout* oneAlgoLayout = new QGridLayout(oneAlgoWidget);
	oneAlgoLayout->setMargin(0);

	std::vector<DataAdapterPtr> algoOption = mReconstructer->getAlgoOptions();
	unsigned row = 0;
	for (;row < algoOption.size(); ++row)
	{
		sscCreateDataWidget(oneAlgoWidget, algoOption[row], oneAlgoLayout, row);
	}
	oneAlgoLayout->setRowStretch(row, 1); // Set stretch on last row

	mAlgoLayout->setCurrentWidget(oneAlgoWidget);
}

QString ReconstructionWidget::getCurrentPath()
{
	return QFileInfo(mReconstructer->getSelectedFilename()).dir().absolutePath();
}

void ReconstructionWidget::reconstruct()
{
	mReconstructer->startReconstruction();
}


void ReconstructionWidget::reload()
{
	this->selectData(mReconstructer->getSelectedFilename());
}

/**Called when data is loaded into reconstructer.
 *
 */
void ReconstructionWidget::inputDataSelected(QString mhdFileName)
{
	if (mReconstructer->getSelectedFilename().isEmpty())
	{
		return;
	}	

	mFileSelectWidget->setFilename(mhdFileName);
}

void ReconstructionWidget::selectData(QString filename)
{
	if (filename.isEmpty())
	{
		reportWarning("no file selected");
		return;
	}

	mReconstructer->selectData(filename);
}

/** Called when parameters in the reconstructer has changed
 *
 */
void ReconstructionWidget::paramsChangedSlot()
{
	Vector3D range = mReconstructer->getOutputVolumeParams().getExtent().range();

	QString extText =
		QString("%1,  %2,  %3").arg(range[0], 0, 'f', 1).arg(range[1], 0, 'f', 1).arg(range[2], 0, 'f', 1);
	mExtentLineEdit->setText(extText);

	mInputSpacingLineEdit->setText(QString("%1").arg(mReconstructer->getOutputVolumeParams().getInputSpacing(), 0, 'f', 4));
}

void ReconstructionWidget::reconstructAboutToStartSlot()
{
	std::set<cx::TimedAlgorithmPtr> threads = mReconstructer->getThreadedReconstruction();
	mTimedAlgorithmProgressBar->attach(threads);
}

void ReconstructionWidget::reconstructStartedSlot()
{
	mReconstructButton->setEnabled(false);
}

void ReconstructionWidget::reconstructFinishedSlot()
{
	std::set<cx::TimedAlgorithmPtr> threads = mReconstructer->getThreadedReconstruction();
	mTimedAlgorithmProgressBar->detach(threads);
		mReconstructButton->setEnabled(true);
}


}//namespace
