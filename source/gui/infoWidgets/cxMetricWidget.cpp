/*
 * cxMetricWidget.cpp
 *
 *  Created on: Jul 5, 2011
 *      Author: christiana
 */

#include <cxMetricWidget.h>

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QVBoxLayout>
#include <QHeaderView>

#include "sscMessageManager.h"
#include "sscTypeConversions.h"
#include "sscCoordinateSystemHelpers.h"
#include "cxToolManager.h"
#include "cxViewManager.h"
#include "cxViewGroup.h"
#include "cxViewWrapper.h"
#include "cxPointMetric.h"
#include "cxDistanceMetric.h"
#include "sscDataManager.h"
#include "sscLabeledComboBoxWidget.h"
#include "cxVector3DWidget.h"
#include "sscRegistrationTransform.h"

namespace cx
{


//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

MetricWidget::MetricWidget(QWidget* parent) :
  BaseWidget(parent, "MetricWidget", "Metrics/3D ruler"),
  mVerticalLayout(new QVBoxLayout(this)),
  mTable(new QTableWidget(this)),
  mActiveLandmark(""),
//  mAddPointButton(new QPushButton("New Pt", this)),
//  mAddDistButton(new QPushButton("New Dist", this)),
  mRemoveButton(new QPushButton("Remove", this)),
  mLoadReferencePointsButton(new QPushButton("Load reference points", this))
{
  connect(ssc::toolManager(), SIGNAL(configured()), this, SLOT(updateSlot()));
  connect(ssc::dataManager(), SIGNAL(dataLoaded()), this, SLOT(updateSlot()));

  //table widget
  connect(mTable, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
  connect(mTable, SIGNAL(cellChanged(int, int)), this, SLOT(cellChangedSlot(int, int)));

  this->setLayout(mVerticalLayout);

  mEditWidgets = new QStackedWidget;

  mRemoveButton->setDisabled(true);
  connect(mRemoveButton, SIGNAL(clicked()), this, SLOT(removeButtonClickedSlot()));
  connect(mLoadReferencePointsButton, SIGNAL(clicked()), this, SLOT(loadReferencePointsSlot()));

  //layout
  mVerticalLayout->addWidget(mTable, 1);
  mVerticalLayout->addWidget(mEditWidgets, 0);

  QHBoxLayout* buttonLayout = new QHBoxLayout;
  mVerticalLayout->addLayout(buttonLayout);

  this->createAction(buttonLayout, "", "New Pt", "Create a new Point Metric",      SLOT(addPointButtonClickedSlot()));
  this->createAction(buttonLayout, "", "New Dist", "Create a new Distance Metric", SLOT(addDistanceButtonClickedSlot()));
  this->createAction(buttonLayout, "", "New Angle", "Create a new Angle Metric",   SLOT(addAngleButtonClickedSlot()));
  this->createAction(buttonLayout, "", "New Plane", "Create a new Plane Metric",   SLOT(addPlaneButtonClickedSlot()));

  buttonLayout->addWidget(mRemoveButton);
  mVerticalLayout->addWidget(mLoadReferencePointsButton);
}

MetricWidget::~MetricWidget()
{}

template<class T>
QAction* MetricWidget::createAction(QLayout* layout, QString iconName, QString text, QString tip, T slot)
{
  QAction* action = new QAction(QIcon(iconName), text, this);
  action->setStatusTip(tip);
  action->setToolTip(tip);
  connect(action, SIGNAL(triggered()), this, slot);
  QToolButton* button = new QToolButton();
  button->setDefaultAction(action);
  layout->addWidget(button);
  return action;
}

QString MetricWidget::defaultWhatsThis() const
{
  return "<html>"
      "<h3>Utility for sampling points in 3D</h3>"
      "<p>Lets you sample points in 3D and get the distance between sampled points.</p>"
      "<p><i></i></p>"
      "</html>";
}

void MetricWidget::cellChangedSlot(int row, int col)
{
  if (col==0) // data name changed
  {
    QTableWidgetItem* item = mTable->item(row,col);
    ssc::DataPtr data = ssc::dataManager()->getData(item->data(Qt::UserRole).toString());
    if (data)
      data->setName(item->text());
  }
}


//
//void MetricWidget::testSlot()
//{
//	PointMetricPtr p0 = this->addPoint(ssc::Vector3D(0,0,0), ssc::CoordinateSystem(ssc::csPATIENTREF, ""));
////	PointMetricPtr p0(new PointMetric("point%1","point%1"));
////	p0->setFrame(ssc::CoordinateSystem(ssc::csPATIENTREF, ""));
////	p0->setCoordinate(ssc::Vector3D(0,0,0));
////	ssc::dataManager()->loadData(p0);
//
//	PointMetricPtr p1 = this->addPoint(ssc::Vector3D(0,0,0), ssc::CoordinateSystem(ssc::csTOOL, "ManualTool"));
////	PointMetricPtr p1(new PointMetric("point%1","point%1"));
////	p1->setFrame(ssc::CoordinateSystem(ssc::csTOOL, "ManualTool"));
////	p1->setCoordinate(ssc::Vector3D(0,0,0));
////	ssc::dataManager()->loadData(p1);
//
//	DistanceMetricPtr d0(new DistanceMetric("distance%1","distance%1"));
//	d0->setPoint(0, p0);
//	d0->setPoint(1, p1);
//	ssc::dataManager()->loadData(d0);
//
//	d0->getDistance();
//}


void MetricWidget::itemSelectionChanged()
{
  QTableWidgetItem* item = mTable->currentItem();

  mActiveLandmark = item->data(Qt::UserRole).toString();
  mEditWidgets->setCurrentIndex(mTable->currentRow());
  enablebuttons();
}


void MetricWidget::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);

  ViewGroupDataPtr data = viewManager()->getViewGroups()[0]->getData();
  ViewGroupData::Options options = data->getOptions();
  options.mShowPointPickerProbe = true;
  data->setOptions(options);

  this->updateSlot();
}

void MetricWidget::hideEvent(QHideEvent* event)
{
  QWidget::hideEvent(event);
}

MetricBasePtr MetricWidget::createMetricWrapper(ssc::DataPtr data)
{
  if (boost::shared_dynamic_cast<PointMetric>(data))
  {
    return MetricBasePtr(new PointMetricWrapper(boost::shared_dynamic_cast<PointMetric>(data)));
  }
  else if (boost::shared_dynamic_cast<DistanceMetric>(data))
  {
    return MetricBasePtr(new DistanceMetricWrapper(boost::shared_dynamic_cast<DistanceMetric>(data)));
  }
  else if (boost::shared_dynamic_cast<PlaneMetric>(data))
  {
    return MetricBasePtr(new PlaneMetricWrapper(boost::shared_dynamic_cast<PlaneMetric>(data)));
  }
  else if (boost::shared_dynamic_cast<AngleMetric>(data))
  {
    return MetricBasePtr(new AngleMetricWrapper(boost::shared_dynamic_cast<AngleMetric>(data)));
  }

	return MetricBasePtr();
}

/** create new metric wrappers for all metrics in PaSM
 *
 */
std::vector<MetricBasePtr> MetricWidget::createMetricWrappers()
{
	std::vector<MetricBasePtr> retval;
  std::map<QString, ssc::DataPtr> all = ssc::dataManager()->getData();
  for (std::map<QString, ssc::DataPtr>::iterator iter=all.begin(); iter!=all.end(); ++iter)
  {
  	MetricBasePtr wrapper = this->createMetricWrapper(iter->second);
  	if (wrapper)
  	{
  		retval.push_back(wrapper);
  	}
  }
  return retval;
}

/**update contents of table.
 * rebuild table only if necessary
 *
 */
void MetricWidget::updateSlot()
{
//  std::cout << "update " << std::endl;

  mTable->blockSignals(true);

  std::vector<MetricBasePtr> newMetrics = this->createMetricWrappers();

  bool rebuild = newMetrics.size()!=mMetrics.size();

  // check if we need to rebuild the table from scratch:
  if (!rebuild)
  {
  	for (unsigned i=0; i<mMetrics.size(); ++i)
  	{
  		rebuild = rebuild || mMetrics[i]->getData()!=newMetrics[i]->getData();
  	}
  }

  // rebuild all:
  if (rebuild)
  {
//    std::cout << "rebuild " << newMetrics.size() << std::endl;
    mTable->clear();

    while (mEditWidgets->count())
    {
    	mEditWidgets->removeWidget(mEditWidgets->widget(0));
    }

    for (unsigned i=0; i<mMetrics.size(); ++i)
    {
    	disconnect(mMetrics[i]->getData().get(), SIGNAL(transformChanged()), this, SLOT(updateSlot()));
    }

    mMetrics = newMetrics;

    for (unsigned i=0; i<mMetrics.size(); ++i)
    {
    	MetricBasePtr wrapper = mMetrics[i];
  		connect(wrapper->getData().get(), SIGNAL(transformChanged()), this, SLOT(updateSlot()));
//  		mEditWidgets->addWidget(wrapper->createWidget());

  		QGroupBox* groupBox = new QGroupBox(wrapper->getData()->getName(), this);
  		groupBox->setFlat(true);
//  	  QFrame* groupBox = new QFrame(this);
  	  QVBoxLayout* gbLayout = new QVBoxLayout(groupBox);
//  	  groupBox->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
//  	  groupBox->setLineWidth(3);
  	  gbLayout->setMargin(4);
  	  gbLayout->addWidget(wrapper->createWidget());
  	  mEditWidgets->addWidget(groupBox);
    }

    mEditWidgets->setCurrentIndex(-1);

    //ready the table widget
    mTable->setRowCount(mMetrics.size());
    mTable->setColumnCount(4);
    QStringList headerItems(QStringList() << "Name" << "Value" << "Arguments" << "Type");
    mTable->setHorizontalHeaderLabels(headerItems);
    mTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->verticalHeader()->hide();

    for (unsigned i = 0; i < mMetrics.size(); ++i)
    {
    	MetricBasePtr current = mMetrics[i];

      for (unsigned j = 0; j < 4; ++j)
      {
      	QTableWidgetItem* item = new QTableWidgetItem("empty");
        item->setData(Qt::UserRole, current->getData()->getUid());
        mTable->setItem(i, j, item);
//        std::cout << "set item " << i << " " << j << std::endl;
      }
    }
//    std::cout << "rebuild end"  << std::endl;
  }

  // update contents:
  for (unsigned i = 0; i < mMetrics.size(); ++i)
  {
  	MetricBasePtr current = mMetrics[i];
    if (!mTable->item(i,0))
    {
      std::cout << "no qitem for:: " << i << " " << current->getData()->getName() << std::endl;
      continue;
    }
  	mTable->item(i,0)->setText(current->getData()->getName());
    mTable->item(i,1)->setText(current->getValue());
    mTable->item(i,2)->setText(current->getArguments());
    mTable->item(i,3)->setText(current->getType());

    //highlight selected row
    if (current->getData()->getUid() == mActiveLandmark)
    {
      mTable->setCurrentCell(i,1);
      mEditWidgets->setCurrentIndex(i);
    }
  }

  mTable->blockSignals(false);

  this->enablebuttons();
}

void MetricWidget::enablebuttons()
{
  mRemoveButton->setEnabled(mActiveLandmark!="");
  mLoadReferencePointsButton->setEnabled(ssc::toolManager()->getReferenceTool());
}

PointMetricPtr MetricWidget::addPoint(ssc::Vector3D point, ssc::CoordinateSystem frame)
{
	PointMetricPtr p1(new PointMetric("point%1","point%1"));
  p1->get_rMd_History()->setParentFrame("reference");
	p1->setFrame(frame);
	p1->setCoordinate(point);
	ssc::dataManager()->loadData(p1);

	viewManager()->getViewGroups()[0]->getData()->addData(p1);

	return p1;
}

void MetricWidget::addPointButtonClickedSlot()
{
  ssc::CoordinateSystem ref = ssc::SpaceHelpers::getR();
  ssc::Vector3D p_ref = ssc::SpaceHelpers::getDominantToolTipPoint(ref, true);

	this->addPoint(p_ref, ref);
}

void MetricWidget::addPlaneButtonClickedSlot()
{
  ssc::CoordinateSystem ref = ssc::SpaceHelpers::getR();
  ssc::Vector3D p_ref = ssc::SpaceHelpers::getDominantToolTipPoint(ref, true);

  PlaneMetricPtr p1(new PlaneMetric("plane%1","plane%1"));
  p1->get_rMd_History()->setParentFrame("reference");
  p1->setFrame(ref);
  p1->setCoordinate(p_ref);
  p1->setNormal(ssc::Vector3D(1,0,0));
  ssc::dataManager()->loadData(p1);

  viewManager()->getViewGroups()[0]->getData()->addData(p1);
}

void MetricWidget::addDistanceButtonClickedSlot()
{
	DistanceMetricPtr d0(new DistanceMetric("distance%1","distance%1"));
  d0->get_rMd_History()->setParentFrame("reference");
	// first try to reuse existing points as distance arguments, otherwise create new ones.
  std::vector<ssc::DataPtr> args;

  for (unsigned i=0; i<mMetrics.size(); ++i)
  {
  	if (d0->validArgument(mMetrics[i]->getData()))
  		args.push_back(mMetrics[i]->getData());
  }

  while (args.size() > d0->getArgumentCount())
  	args.erase(args.begin());

  while (args.size() < d0->getArgumentCount())
  {
  	PointMetricPtr p0 = this->addPoint(ssc::Vector3D(0,0,0), ssc::CoordinateSystem(ssc::csREF, ""));
  	args.push_back(p0);
  }

  for (unsigned i=0; i<args.size(); ++i)
    d0->setArgument(i, args[i]);

	ssc::dataManager()->loadData(d0);

	viewManager()->getViewGroups()[0]->getData()->addData(d0);
}

void MetricWidget::addAngleButtonClickedSlot()
{
  AngleMetricPtr d0(new AngleMetric("angle%1","angle%1"));
  d0->get_rMd_History()->setParentFrame("reference");
	// first try to reuse existing points as distance arguments, otherwise create new ones.
  std::vector<ssc::DataPtr> args;

  for (unsigned i=0; i<mMetrics.size(); ++i)
  {
  	if (d0->validArgument(mMetrics[i]->getData()))
  		args.push_back(mMetrics[i]->getData());
  }

  while (args.size() > 3)
  	args.erase(args.begin());

  while (args.size() < 3)
  {
  	PointMetricPtr p0 = this->addPoint(ssc::Vector3D(0,0,0), ssc::CoordinateSystem(ssc::csREF, ""));
  	args.push_back(p0);
  }

  for (unsigned i=0; i<args.size(); ++i)
    d0->setArgument(i, args[i]);

  d0->setArgument(0, args[0]);
  d0->setArgument(1, args[1]);
  d0->setArgument(2, args[1]);
  d0->setArgument(3, args[2]);

  ssc::dataManager()->loadData(d0);

  viewManager()->getViewGroups()[0]->getData()->addData(d0);
}

void MetricWidget::removeButtonClickedSlot()
{
	ssc::dataManager()->removeData(mActiveLandmark);
}

void MetricWidget::loadReferencePointsSlot()
{
  ssc::ToolPtr refTool = ssc::toolManager()->getReferenceTool();
  if(!refTool) // we only load reference points from reference tools
  {
    ssc::messageManager()->sendDebug("No reference tool, cannot load reference points into the pointsampler");
    return;
  }

  std::map<int, ssc::Vector3D> referencePoints_s = refTool->getReferencePoints();
  if(referencePoints_s.empty())
  {
    ssc::messageManager()->sendWarning("No referenceppoints in reference tool "+refTool->getName());
    return;
  }

  ssc::CoordinateSystem ref = ssc::CoordinateSystemHelpers::getR();
  ssc::CoordinateSystem sensor = ssc::CoordinateSystemHelpers::getS(refTool);

  std::map<int, ssc::Vector3D>::iterator it = referencePoints_s.begin();
  for(; it != referencePoints_s.end(); ++it)
  {
    ssc::Vector3D P_ref = ssc::CoordinateSystemHelpers::get_toMfrom(sensor, ref).coord(it->second);
    this->addPoint(P_ref);
  }
}

}//end namespace cx
