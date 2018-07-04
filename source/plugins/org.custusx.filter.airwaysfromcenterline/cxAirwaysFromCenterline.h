/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) SINTEF Department of Medical Technology.
All rights reserved.

CustusX is released under a BSD 3-Clause license.

See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
=========================================================================*/

#ifndef CXAIRWAYSFROMCENTERLINE_H
#define CXAIRWAYSFROMCENTERLINE_H

#include "cxMesh.h"
#include <QDomElement>
#include "org_custusx_filter_airwaysfromcenterline_Export.h"

namespace cx
{

typedef std::vector< Eigen::Matrix4d > M4Vector;
typedef boost::shared_ptr<class RouteToTarget> RouteToTargetPtr;
typedef boost::shared_ptr<class BranchList> BranchListPtr;
typedef boost::shared_ptr<class Branch> BranchPtr;


class org_custusx_filter_airwaysfromcenterline_EXPORT AirwaysFromCenterline
{
public:
    AirwaysFromCenterline();
    virtual ~AirwaysFromCenterline();
    Eigen::MatrixXd getCenterlinePositions(vtkPolyDataPtr centerline_r);
    void processCenterline(vtkPolyDataPtr centerline_r);
    vtkPolyDataPtr generateTubes();
    void createEmptyImage();
    void addSphereToImage(double position[3], double radius);
    vtkPolyDataPtr addVTKPoints(std::vector< Eigen::Vector3d > positions);
    vtkPolyDataPtr getVTKPoints();

private:
	Eigen::MatrixXd mCLpoints;
	BranchListPtr mBranchListPtr;
    vtkImageDataPtr mResultImagePtr;
    double mOrigin[3];
    double mSpacing[3];
    double mBounds[6];
    int mDim[3];
};

} /* namespace cx */

#endif // CXAIRWAYSFROMCENTERLINE_H
