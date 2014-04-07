// This file is part of SSC,
// a C++ Library supporting Image Guided Therapy Applications.
//
// Copyright (C) 2008- SINTEF Medical Technology
// Copyright (C) 2008- Sonowand AS
//
// SSC is owned by SINTEF Medical Technology and Sonowand AS,
// hereafter named the owners. Each particular piece of code
// is owned by the part that added it to the library.
// SSC source code and binaries can only be used by the owners
// and those with explicit permission from the owners.
// SSC shall not be distributed to anyone else.
//
// SSC is distributed WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.
//
// See sscLicense.txt for more information.

#ifndef CXTOOLTRACER_H_
#define CXTOOLTRACER_H_

#include <QObject>
#include <QSize>
#include "vtkSmartPointer.h"
typedef vtkSmartPointer<class vtkImageData> vtkImageDataPtr;
typedef vtkSmartPointer<class vtkPolyData> vtkPolyDataPtr;
#include "cxTransform3D.h"

#include <boost/shared_ptr.hpp>
//#include "cxProbeData.h"
#include "vtkForwardDeclarations.h"
#include "cxForwardDeclarations.h"

class QColor;

namespace cx
{

typedef boost::shared_ptr<class ToolTracer> ToolTracerPtr;
typedef boost::shared_ptr<class SpaceProvider> SpaceProviderPtr;

/** \brief 3D Graphics class for displaying the trace path traversed by a tool.
 *
 * ToolTracer is used internally by ToolRep3D as an option.
 *
 * Used by CustusX.
 *
 * \ingroup cx_resource_visualization
 */
class ToolTracer : QObject
{
	Q_OBJECT
public:
	static ToolTracerPtr create(SpaceProviderPtr spaceProvider);
	void setTool(ToolPtr tool);
	vtkPolyDataPtr getPolyData();
	vtkActorPtr getActor();

	void setColor(QColor color);

	void start(); // start path tracking
	void stop(); // stop tracking
	void clear(); // erase stored tracking data.
	bool isRunning() const; // true if started and not stopped.
	void setMinDistance(double distance) { mMinDistance = distance; }
	int getSkippedPoints() { return mSkippedPoints; }
    void addManyPositions(TimedTransformMap trackerRecordedData_prMt);

private slots:
	void receiveTransforms(Transform3D prMt, double timestamp);
private:
	ToolTracer();
	void connectTool();
	void disconnectTool();

	bool mRunning;
	vtkPolyDataPtr mPolyData; ///< polydata representation of the probe, in space u
	vtkActorPtr mActor;
	ToolPtr mTool;
	vtkPolyDataMapperPtr mPolyDataMapper;
	vtkPropertyPtr mProperty;

	vtkPointsPtr mPoints;
	vtkCellArrayPtr mLines;

	bool mFirstPoint;
	int mSkippedPoints;
	Vector3D mPreviousPoint;
	double mMinDistance;

	SpaceProviderPtr mSpaceProvider;
};

} // namespace cx

#endif /*CXTOOLTRACER_H_*/
