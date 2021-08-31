/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.
                 
Copyright (c) SINTEF Department of Medical Technology.
All rights reserved.
                 
CustusX is released under a BSD 3-Clause license.
                 
See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
=========================================================================*/

#ifndef CXCOLORVARIATIONFILTER_H
#define CXCOLORVARIATIONFILTER_H

#include "cxFilterImpl.h"

namespace cx
{

/** Filter to apply variation in colors to a mesh
 *
 * \ingroup cxResourceAlgorithms
 * \date Aug 26, 2021
 * \author Erlend F. Hofstad
 */
class cxResourceFilter_EXPORT ColorVariationFilter : public FilterImpl
{
	Q_OBJECT

public:
	ColorVariationFilter(VisServicesPtr services);
	virtual ~ColorVariationFilter() {}

	virtual QString getType() const;
	virtual QString getName() const;
	virtual QString getHelp() const;

	virtual bool execute();
	virtual bool postProcess();

protected:
	virtual void createOptions();
	virtual void createInputTypes();
	virtual void createOutputTypes();

private:
	DoublePropertyPtr getGlobalVarianceOption(QDomElement root);
	DoublePropertyPtr getLocalVarianceOption(QDomElement root);
	void sortPolyData(vtkPolyDataPtr polyData);

	std::vector<std::vector<int>> mPolyToPointsArray;
	std::vector<std::vector<int>> mPointToPolysArray;
};

} // namespace cx


#endif // CXCOLORVARIATIONFILTER_H
