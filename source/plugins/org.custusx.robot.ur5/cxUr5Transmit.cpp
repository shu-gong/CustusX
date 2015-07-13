#include "cxUr5Transmit.h"


namespace cx
{

void Ur5Transmit::addToPoseQueue(Ur5State pose)
{
    poseQueue.push_back(pose);
}

void Ur5Transmit::movejProgram(std::vector<Ur5State> poseQueue,double a, double v, double r)
{
    for(int i=0;i<poseQueue.size();i++)
    {
        programQueue.push_back(movej(poseQueue[i],a,v,r));
    }
}

QString Ur5Transmit::movej(Ur5State p,double a, double v,double r)
{
    return QString("movej(p[%1,%2,%3,%4,%5,%6],a=%7,v=%8,r=%9)")
            .arg(p.cartAxis(0)).arg(p.cartAxis(1)).arg(p.cartAxis(2)).arg(p.cartAngles(0))
            .arg(p.cartAngles(1)).arg(p.cartAngles(2)).arg(a).arg(v).arg(r);
}

QString Ur5Transmit::movel(Ur5State p,double a, double v)
{
    return QString("movel(p[%1,%2,%3,%4,%5,%6],a=%7,v=%8)")
            .arg(p.cartAxis(0)).arg(p.cartAxis(1)).arg(p.cartAxis(2)).arg(p.cartAngles(0))
            .arg(p.cartAngles(1)).arg(p.cartAngles(2)).arg(a).arg(v);
}

QString Ur5Transmit::speedj(double* velocityField, double a, double t)
{
    return QString("speedj([%1,%2,%3,%4,%5,%6],a=%7,t_min=%8)")
            .arg(velocityField[0]).arg(velocityField[1]).arg(velocityField[2]).arg(velocityField[3])
            .arg(velocityField[4]).arg(velocityField[5]).arg(a).arg(t);
}



} //cx
