/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) SINTEF Department of Medical Technology.
All rights reserved.

CustusX is released under a BSD 3-Clause license.

See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
=========================================================================*/

#include "cxAirwaysFromCenterline.h"
#include <boost/math/special_functions/round.hpp>
#include <vtkPolyData.h>
#include "cxBranchList.h"
#include "cxBranch.h"
#include "vtkCardinalSpline.h"
#include <cxImage.h>
#include "cxContourFilter.h"
#include <vtkImageData.h>
#include <vtkPointData.h>
#include "cxVolumeHelpers.h"
#include "vtkCardinalSpline.h"
#include "cxLogger.h"

typedef vtkSmartPointer<class vtkCardinalSpline> vtkCardinalSplinePtr;


namespace cx
{

AirwaysFromCenterline::AirwaysFromCenterline():
    mBranchListPtr(new BranchList),
    mAirwaysVolumeBoundaryExtention(10),
    mAirwaysVolumeBoundaryExtentionTracheaStart(2),
    mAirwaysVolumeSpacing(0.5)
{
}

AirwaysFromCenterline::~AirwaysFromCenterline()
{
}

void AirwaysFromCenterline::setTypeToBloodVessel(bool bloodVessel)
{
	mBloodVessel = bloodVessel;
}

Eigen::MatrixXd AirwaysFromCenterline::getCenterlinePositions(vtkPolyDataPtr centerline_r)
{

    int N = centerline_r->GetNumberOfPoints();
    Eigen::MatrixXd CLpoints(3,N);
    for(vtkIdType i = 0; i < N; i++)
        {
        double p[3];
        centerline_r->GetPoint(i,p);
        Eigen::Vector3d position;
		position(0) = p[0]; position(1) = p[1]; position(2) = p[2];
		CLpoints.block(0 , i , 3 , 1) = position;
		}
	return CLpoints;
}

void AirwaysFromCenterline::setBranches(BranchListPtr branches)
{
	mBranchListPtr = branches;
}

void AirwaysFromCenterline::setSegmentedVolume(vtkImageDataPtr segmentedVolume)
{
    mOriginalSegmentedVolume = segmentedVolume;
}

void AirwaysFromCenterline::processCenterline(vtkPolyDataPtr centerline_r)
{
	if (mBranchListPtr)
		mBranchListPtr->deleteAllBranches();

	Eigen::MatrixXd CLpoints_r = getCenterlinePositions(centerline_r);

	mBranchListPtr->findBranchesInCenterline(CLpoints_r);

	mBranchListPtr->smoothBranchPositions(40);
	mBranchListPtr->interpolateBranchPositions(5);

	mBranchListPtr->calculateOrientations();
	mBranchListPtr->smoothOrientations();
}

BranchListPtr AirwaysFromCenterline::getBranchList()
{
	return mBranchListPtr;
}

//Not in use? Delete?
void AirwaysFromCenterline::processCenterline(Eigen::MatrixXd CLpoints_r)
{
	if (mBranchListPtr)
		mBranchListPtr->deleteAllBranches();

    mBranchListPtr->findBranchesInCenterline(CLpoints_r);

    mBranchListPtr->smoothBranchPositions(40);
    mBranchListPtr->interpolateBranchPositions(5);
}

/*
    AirwaysFromCenterline::generateTubes makes artificial airway tubes around the input centerline. The radius
    of the tubes is decided by the generation number, based on Weibel's model of airways. In contradiction to the model,
    it is set a lower boundary for the tube radius (2 mm) making the peripheral airways larger than in reality,
    which makes it possible to virtually navigate inside the tubes. The airways are generated by adding a sphere to
    a volume (image) at each point along every branch. The output is a surface model generated from the volume.
*/
vtkPolyDataPtr AirwaysFromCenterline::generateTubes(double staticRadius, bool mergeWithOriginalAirways) // if staticRadius == 0, radius is retrieved from branch generation number
{
    mMergeWithOriginalAirways = mergeWithOriginalAirways;
    vtkImageDataPtr airwaysVolumePtr;

    if (mergeWithOriginalAirways)
    {
        if (mOriginalSegmentedVolume)
        {
            airwaysVolumePtr = this->initializeAirwaysVolumeFromOriginalSegmentation();
        }
        else
        {
            CX_LOG_WARNING() << "AirwaysFromCenterline::generateTubes: Segmented airways volume not set. Creating pure artificaial tubes around centerlines.";
             airwaysVolumePtr = this->initializeEmptyAirwaysVolume();
        }
    }
    else
        airwaysVolumePtr = this->initializeEmptyAirwaysVolume();

    airwaysVolumePtr = addSpheresAlongCenterlines(airwaysVolumePtr, staticRadius);

    //create contour from image
    vtkPolyDataPtr rawContour = ContourFilter::execute(
                airwaysVolumePtr,
            1, //treshold
            false, // reduce resolution
            true, // smoothing
            true, // keep topology
            0, // target decimation
            30, // number of iterations smoothing
            0.10 // band pass smoothing
    );

    return rawContour;
}

vtkImageDataPtr AirwaysFromCenterline::initializeEmptyAirwaysVolume()
{
    std::vector<BranchPtr> branches = mBranchListPtr->getBranches();
    vtkPointsPtr pointsPtr = vtkPointsPtr::New();

    int numberOfPoints = 0;
    for (int i = 0; i < branches.size(); i++)
        numberOfPoints += branches[i]->getPositions().cols();

    pointsPtr->SetNumberOfPoints(numberOfPoints);

    int pointIndex = 0;
    for (int i = 0; i < branches.size(); i++)
    {
        Eigen::MatrixXd positions = branches[i]->getPositions();
        for (int j = 0; j < positions.cols(); j++)
        {
            pointsPtr->SetPoint(pointIndex, positions(0,j), positions(1,j), positions(2,j));
            pointIndex += 1;
        }
    }

    pointsPtr->GetBounds(mBounds);

    //Extend bounds to make room for surface model extended from centerline
    mBounds[0] -= mAirwaysVolumeBoundaryExtention;
    mBounds[1] += mAirwaysVolumeBoundaryExtention;
    mBounds[2] -= mAirwaysVolumeBoundaryExtention;
    mBounds[3] += mAirwaysVolumeBoundaryExtention;
    mBounds[4] -= mAirwaysVolumeBoundaryExtention;
    mBounds[5] -= mAirwaysVolumeBoundaryExtentionTracheaStart; // to make top of trachea open
    if (mBloodVessel)
    	mBounds[5] += mAirwaysVolumeBoundaryExtention;

    mSpacing[0] = mAirwaysVolumeSpacing;  //Smaller spacing improves resolution but increases run-time
    mSpacing[1] = mAirwaysVolumeSpacing;
    mSpacing[2] = mAirwaysVolumeSpacing;

    // compute dimensions
    for (int i = 0; i < 3; i++)
        mDim[i] = static_cast<int>(std::ceil((mBounds[i * 2 + 1] - mBounds[i * 2]) / mSpacing[i]));

    mOrigin[0] = mBounds[0] + mSpacing[0] / 2;
    mOrigin[1] = mBounds[2] + mSpacing[1] / 2;
    mOrigin[2] = mBounds[4] + mSpacing[2] / 2;

    vtkImageDataPtr airwaysVolumePtr = generateVtkImageData(mDim, mSpacing, 0);
    airwaysVolumePtr->SetOrigin(mOrigin);

    return airwaysVolumePtr;
}

vtkImageDataPtr AirwaysFromCenterline::initializeAirwaysVolumeFromOriginalSegmentation()
{
    vtkImageDataPtr airwaysVolumePtr;
    if (!mOriginalSegmentedVolume)
        return airwaysVolumePtr;

    airwaysVolumePtr = mOriginalSegmentedVolume;

    Vector3D origin(mOriginalSegmentedVolume->GetOrigin());
    mOrigin[0] = origin[0];
    mOrigin[1] = origin[1];
    mOrigin[2] = origin[2];

    Vector3D spacing(mOriginalSegmentedVolume->GetSpacing());
    mSpacing = spacing;

    airwaysVolumePtr->GetBounds(mBounds);

    // compute dimensions
    for (int i = 0; i < 3; i++)
        mDim[i] = static_cast<int>(std::ceil((mBounds[i * 2 + 1] - mBounds[i * 2]) / mSpacing[i]));

    return mOriginalSegmentedVolume;
}


vtkImageDataPtr AirwaysFromCenterline::addSpheresAlongCenterlines(vtkImageDataPtr airwaysVolumePtr, double staticRadius)
{
    std::vector<BranchPtr> branches = mBranchListPtr->getBranches();

    for (int i = 0; i < branches.size(); i++)
    {
        Eigen::MatrixXd positions = branches[i]->getPositions();
        vtkPointsPtr pointsPtr = vtkPointsPtr::New();
        int numberOfPositionsInBranch = positions.cols();
        pointsPtr->SetNumberOfPoints(numberOfPositionsInBranch);

        double radius = staticRadius;
        if (similar(staticRadius, 0))
        {
        	radius = branches[i]->findBranchRadius();
            //if (mMergeWithOriginalAirways)
            //    radius = radius/2;
        }

        for (int j = 0; j < numberOfPositionsInBranch; j++)
        {
            double spherePos[3];
            spherePos[0] = positions(0,j);
            spherePos[1] = positions(1,j);
            spherePos[2] = positions(2,j);
            airwaysVolumePtr = addSphereToImage(airwaysVolumePtr, spherePos, radius);
        }
    }
    return airwaysVolumePtr;
}

vtkImageDataPtr AirwaysFromCenterline::addSphereToImage(vtkImageDataPtr airwaysVolumePtr, double position[3], double radius)
{
    int value = 1;
    int centerIndex[3];
    int sphereBoundingBoxIndex[6];

    for (int i=0; i<3; i++)
    {
        centerIndex[i] = static_cast<int>(boost::math::round( (position[i]-mOrigin[i]) / mSpacing[i] ));
        sphereBoundingBoxIndex[2*i] = std::max(
                    static_cast<int>(boost::math::round( (position[i]-mOrigin[i] - radius) / mSpacing[i] )),
                    0);
        sphereBoundingBoxIndex[2*i+1] = std::min(
                    static_cast<int>(boost::math::round( (position[i]-mOrigin[i] + radius) / mSpacing[i] )),
                    mDim[i]-1);
    }


    for (int x = sphereBoundingBoxIndex[0]; x<=sphereBoundingBoxIndex[1]; x++)
        for (int y = sphereBoundingBoxIndex[2]; y<=sphereBoundingBoxIndex[3]; y++)
            for (int z = sphereBoundingBoxIndex[4]; z<=sphereBoundingBoxIndex[5]; z++)
            {
                double distanceFromCenter = sqrt((x-centerIndex[0])*mSpacing[0]*(x-centerIndex[0])*mSpacing[0] +
                                                 (y-centerIndex[1])*mSpacing[1]*(y-centerIndex[1])*mSpacing[1] +
                                                 (z-centerIndex[2])*mSpacing[2]*(z-centerIndex[2])*mSpacing[2]);

                if (distanceFromCenter < radius)
                {
                    //if( x >= mBounds[0] && x <= mBounds[1] && y >= mBounds[2] && y <= mBounds[3] && z >= mBounds[4] && z <= mBounds[5])
                    //{
                        unsigned char* dataPtrImage = static_cast<unsigned char*>(airwaysVolumePtr->GetScalarPointer(x,y,z));
                        dataPtrImage[0] = value;
                   // }
                }
            }

    return airwaysVolumePtr;
}

void AirwaysFromCenterline::smoothAllBranchesForVB()
{

	std::vector<BranchPtr> branches = mBranchListPtr->getBranches();
	for (int i=0; i<branches.size(); i++)
	{
		Eigen::MatrixXd positions = branches[i]->getPositions();
		std::vector< Eigen::Vector3d > smoothedPositions = smoothBranch(branches[i], 0, positions.col(0));
		for (int j=0; j<smoothedPositions.size(); j++)
		{
			positions(0,j) = smoothedPositions[j](0);
			positions(1,j) = smoothedPositions[j](1);
			positions(2,j) = smoothedPositions[j](2);
		}
		branches[i]->setPositions(positions);
	}
}

vtkPolyDataPtr AirwaysFromCenterline::getVTKPoints()
{
    vtkPolyDataPtr retval = vtkPolyDataPtr::New();
    vtkPointsPtr points = vtkPointsPtr::New();
    vtkCellArrayPtr lines = vtkCellArrayPtr::New();

    if (!mBranchListPtr)
            return retval;

		double minPointDistance = 0.5; //mm
		mBranchListPtr->excludeClosePositionsInCTCenterline(minPointDistance); // to reduce number of positions in smoothed centerline

    std::vector<BranchPtr> branches  = mBranchListPtr->getBranches();
    int pointIndex = 0;

    for (int i = 0; i < branches.size(); i++)
    {
        Eigen::MatrixXd positions = branches[i]->getPositions();
        int numberOfPositions = positions.cols();

        if (branches[i]->getParentBranch()) // Add parents last position to get connected centerlines
        {
            Eigen::MatrixXd parentPositions = branches[i]->getParentBranch()->getPositions();
            points->InsertNextPoint(parentPositions(0,parentPositions.cols()-1),parentPositions(1,parentPositions.cols()-1),parentPositions(2,parentPositions.cols()-1));
            pointIndex += 1;
        }

        for (int j = 0; j < numberOfPositions; j++)
        {
            points->InsertNextPoint(positions(0,j),positions(1,j),positions(2,j));

            if (j>1 || branches[i]->getParentBranch())
            {
                vtkIdType connection[2] = {pointIndex-1, pointIndex};
                lines->InsertNextCell(2, connection);
            }
            pointIndex += 1;
        }
    }

    retval->SetPoints(points);
    retval->SetLines(lines);
    return retval;
}

/*
		smoothBranch is smoothing the positions of a centerline branch by using vtkCardinalSpline.
		The degree of smoothing is dependent on the branch radius and the shape of the branch.
		First, the method tests if a straight line from start to end of the branch is sufficient by the condition of
		all positions along the line being within the lumen of the airway (max distance from original centerline
		is set to branch radius).
		If this fails, one more control point is added to the spline at the time, until the condition is fulfilled.
		The control point added for each iteration is the position with the larges deviation from the original/unfiltered
		centerline.
*/
std::vector< Eigen::Vector3d > smoothBranch(BranchPtr branchPtr, int startIndex, Eigen::MatrixXd startPosition)
{
	vtkCardinalSplinePtr splineX = vtkSmartPointer<vtkCardinalSpline>::New();
	vtkCardinalSplinePtr splineY = vtkSmartPointer<vtkCardinalSpline>::New();
	vtkCardinalSplinePtr splineZ = vtkSmartPointer<vtkCardinalSpline>::New();

		double branchRadius = branchPtr->findBranchRadius();

		//add control points to spline

		//add first position
		Eigen::MatrixXd positions = branchPtr->getPositions();
		splineX->AddPoint(0,startPosition(0));
		splineY->AddPoint(0,startPosition(1));
		splineZ->AddPoint(0,startPosition(2));


		// Add last position if no parent branch, else add parents position closest to current branch.
		// Branch positions are stored in order from head to feet (e.g. first position is top of trachea),
		// while route-to-target is generated from target to top of trachea.
		if(!branchPtr->getParentBranch())
		{
				splineX->AddPoint(startIndex,positions(0,0));
				splineY->AddPoint(startIndex,positions(1,0));
				splineZ->AddPoint(startIndex,positions(2,0));
		}
		else
		{
				Eigen::MatrixXd parentPositions = branchPtr->getParentBranch()->getPositions();
				splineX->AddPoint(startIndex,parentPositions(0,parentPositions.cols()-1));
				splineY->AddPoint(startIndex,parentPositions(1,parentPositions.cols()-1));
				splineZ->AddPoint(startIndex,parentPositions(2,parentPositions.cols()-1));

		}

		//Add points until all filtered/smoothed postions are minimum 1 mm inside the airway wall, (within r - 1 mm).
		//This is to make sure the smoothed centerline is within the lumen of the airways.
		double maxAcceptedDistanceToOriginalPosition = std::max(branchRadius - 1, 1.0);
		double maxDistanceToOriginalPosition = maxAcceptedDistanceToOriginalPosition + 1;
		int maxDistanceIndex = -1;
		std::vector< Eigen::Vector3d > smoothingResult;

		//add positions to spline
		while (maxDistanceToOriginalPosition >= maxAcceptedDistanceToOriginalPosition && splineX->GetNumberOfPoints() < startIndex)
		{
				if(maxDistanceIndex > 0)
				{
						//add to spline the position with largest distance to original position
						splineX->AddPoint(maxDistanceIndex,positions(0,startIndex - maxDistanceIndex));
						splineY->AddPoint(maxDistanceIndex,positions(1,startIndex - maxDistanceIndex));
						splineZ->AddPoint(maxDistanceIndex,positions(2,startIndex - maxDistanceIndex));
				}

		//evaluate spline - get smoothed positions
				maxDistanceToOriginalPosition = 0.0;
				smoothingResult.clear();
				for(int j=0; j<=startIndex; j++)
		{
			double splineParameter = j;
			Eigen::Vector3d tempPoint;
			tempPoint(0) = splineX->Evaluate(splineParameter);
			tempPoint(1) = splineY->Evaluate(splineParameter);
			tempPoint(2) = splineZ->Evaluate(splineParameter);
			smoothingResult.push_back(tempPoint);

						//calculate distance to original (non-filtered) position
						double distance = findDistanceToLine(tempPoint, positions).second;
						//finding the index with largest distance
						if (distance > maxDistanceToOriginalPosition)
						{
								maxDistanceToOriginalPosition = distance;
								maxDistanceIndex = j;
						}
				}
		}

		return smoothingResult;
}

std::pair<int, double> findDistanceToLine(Eigen::Vector3d point, Eigen::MatrixXd line)
{
	int index = 0;
	double minDistance = findDistance(point, line.col(0));
	for (int i=1; i<line.cols(); i++)
		if (minDistance > findDistance(point, line.col(i)))
		{
			minDistance = findDistance(point, line.col(i));
			index = i;
		}

	return std::make_pair(index , minDistance);
}

double findDistance(Eigen::MatrixXd p1, Eigen::MatrixXd p2)
{
	double d0 = p1(0) - p2(0);
	double d1 = p1(1) - p2(1);
	double d2 = p1(2) - p2(2);

	double D = sqrt( d0*d0 + d1*d1 + d2*d2 );

	return D;
}

} /* namespace cx */
