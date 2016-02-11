#include "cxUr5Robot.h"
#include "cxLogger.h"

namespace cx
{

Ur5Robot::Ur5Robot():
    isMoveInProgress(false),
    isVelocityMoveInProgress(false),
    mBlendRadius(1),
    rtPort(30003),
    secPort(30002),
    motionSpace(Transform3D::Identity()),
    eMt(Transform3D::Identity())
{
    connect(&mRTMonitor,&Ur5Connection::stateChanged,this,&Ur5Robot::updateCurrentState);
    connect(&mSecMonitor,&Ur5Connection::stateChanged,this,&Ur5Robot::updateCurrentState);

    this->mCurrentState.jointConfiguration << 0,-M_PI/2,0,-M_PI/2,0,0;
    this->mCurrentState.bMee = Ur5Kinematics::forward(mCurrentState.jointConfiguration);

    emit(stateUpdated());
}

Ur5Robot::~Ur5Robot()
{
}

void Ur5Robot::atTargetSlot()
{
    this->nextMove();
}

    if(isMoveInProgress || isVelocityMoveInProgress)
        if(this->isAtTargetState())
void Ur5Robot::nextMove()
{
    if(moveInProgress && !mProgramEncoder.jointPositionQueue.empty())
    {
        mProgramEncoder.jointPositionQueue.erase(mProgramEncoder.jointPositionQueue.begin());
        if(mProgramEncoder.jointPositionQueue.empty())
        {
            moveInProgress=false;
        }
        else
        {
            mTargetState.jointConfiguration = mProgramEncoder.jointPositionQueue[0];
            this->move("movej",mTargetState.jointConfiguration,moveAcceleration,moveVelocity);
        }
    }
    else if(moveInProgress && !mProgramEncoder.poseQueue.empty())
    {
        mProgramEncoder.poseQueue.erase(mProgramEncoder.poseQueue.begin());
        if(mProgramEncoder.poseQueue.empty())
        {
            moveInProgress=false;
        }
        else
        {
            mTargetState.cartAxis = mProgramEncoder.poseQueue[0].cartAxis; //+ mStartPosition.cartAxis;
            mTargetState.cartAngles = mStartPosition.cartAngles;
            this->move("movej",mTargetState,moveAcceleration,moveVelocity);
        }
    }
    else if(velocityMoveInProgress && !mProgramEncoder.poseQueue.empty())
    {
        mProgramEncoder.poseQueue.erase(mProgramEncoder.poseQueue.begin());
        if(mProgramEncoder.poseQueue.empty())
        {
            velocityMoveInProgress=false;
            this->move("stopj",mTargetState,moveAcceleration,moveVelocity);
        }
        else
        {
            if(mProgramEncoder.poseQueue.size()>1)
            {
                mTargetState.cartAxis = mProgramEncoder.poseQueue[0].cartAxis;
                mTargetState.cartAngles = mStartPosition.cartAngles;
                Vector3D tangent = (mProgramEncoder.poseQueue[1].cartAxis-mCurrentState.cartAxis)/1000;
                Eigen::RowVectorXd velocityEndEffector(6);
                velocityEndEffector << moveVelocity*tangent(0)/(1000*tangent.norm()),moveVelocity*tangent(1)/(1000*tangent.norm()),moveVelocity*tangent(2)/(1000*tangent.norm())
                                    ,0,0,0;
                mTargetState.jointVelocity = mCurrentState.jacobian.inverse()*velocityEndEffector.transpose();
                this->move("speedj",mTargetState,moveAcceleration/1000,moveVelocity/1000,20,0);
            }
        }
    }
}

void Ur5Robot::updateCurrentState()
{
    this->setPreviousState(getCurrentState());

    Ur5State currentState;

    currentState.timeSinceStart=mRTMonitor.getCurrentState().timeSinceStart;
    currentState.jointConfiguration=mRTMonitor.getCurrentState().jointConfiguration;
    currentState.jointVelocity=mRTMonitor.getCurrentState().jointVelocity;    

    currentState.bMee = Ur5Kinematics::forward(currentState.jointConfiguration);
    currentState.cartAxis= Ur5Kinematics::T2transl(currentState.bMee);
    currentState.cartAngles = Ur5Kinematics::T2rangles(currentState.bMee);
    currentState.jacobian = Ur5Kinematics::jacobian2(currentState.jointConfiguration);

    emit transform("RobotTracker",currentState.bMee,currentState.timeSinceStart);

    this->setCurrentState(currentState);
    emit(stateUpdated());

    if(moveInProgress || velocityMoveInProgress)
        this->atTargetState();
}

Ur5State Ur5Robot::getCurrentState()
{
    return (this->mCurrentState);
}

Ur5State Ur5Robot::getPreviousState()
{
    return (this->mPreviousState);
}

void Ur5Robot::setCurrentState(Ur5State currentState)
{
    this->mCurrentState=currentState;
}

void Ur5Robot::setPreviousState(Ur5State previousState)
{
    this->mPreviousState=previousState;
}

void Ur5Robot::setAddress(QString address)
{
    IPaddress = address;
}

QString Ur5Robot::getAddress()
{
    return (this->IPaddress);
}

void Ur5Robot::connectToRobot(QString IPaddress)
{
    mRTMonitor.setAddress(IPaddress,rtPort);
    mSecMonitor.setAddress(IPaddress,secPort);

    connectToPort(rtPort);
    connectToPort(secPort);

    if(isConnectedToRobot())
        emit(connected());
}

void Ur5Robot::disconnectFromRobot()
{
    disconnectFromPort(rtPort);
    disconnectFromPort(secPort);

    if(!isConnectedToRobot())
        emit(disconnected());
}

void Ur5Robot::connectToPort(int port)
{
    if(port == 30003)
    {
        mRTMonitor.requestConnect();
    }
    else if(port == 30002)
    {
        mSecMonitor.requestConnect();
    }
}

void Ur5Robot::disconnectFromPort(int port)
{
    if(port == 30003)
    {
        mRTMonitor.requestDisconnect();
    }
    else if(port == 30002)
    {
        mSecMonitor.requestDisconnect();
    }
}

bool Ur5Robot::isConnectedToRobot()
{
    return (mRTMonitor.isConnectedToRobot() && mSecMonitor.isConnectedToRobot());
}

void Ur5Robot::shutdown()
{
    sendMessage(mMessageEncoder.powerdown());
    disconnectFromRobot();
    emit(shuttingdown());
}

void Ur5Robot::sendMessage(QString message)
{
    mSecMonitor.sendMessage(message);
}

void Ur5Robot::move(QString typeOfMovement, Ur5State targetState, double acc, double vel, double t, double rad)
{
    mTargetState = targetState;

    if(typeOfMovement=="movej")
        sendMessage(mMessageEncoder.movej(targetState,acc,vel,0));
    else if(typeOfMovement=="speedl")
        sendMessage(mMessageEncoder.speedl(targetState,acc,t));
    else if(typeOfMovement =="speedj")
        sendMessage(mMessageEncoder.speedj(targetState,acc,t));
    else if(typeOfMovement =="stopj")
        sendMessage(mMessageEncoder.stopj(acc));
}

void Ur5Robot::move(QString typeOfMovement, Eigen::RowVectorXd targetState, double acc, double vel, double t, double rad)
{
    mTargetState.jointConfiguration = targetState;

    if(typeOfMovement=="movej")
        sendMessage(mMessageEncoder.movej(targetState,acc,vel,t,rad));
    else if(typeOfMovement=="movejp")
        sendMessage(mMessageEncoder.movejp(targetState,acc, vel, t, rad));
    else if(typeOfMovement=="speedl")
        sendMessage(mMessageEncoder.speedl(targetState,acc,t));
    else if(typeOfMovement =="speedj")
        sendMessage(mMessageEncoder.speedj(targetState,acc,t));
    else if(typeOfMovement =="stopj")
        sendMessage(mMessageEncoder.stopj(acc));

}

void Ur5Robot::move(Ur5MovementInfo movementInfo)
{

}

void Ur5Robot::addToProgramQueue(QString str)
{
    mProgramEncoder.programQueue.push_back(str);
}

void Ur5Robot::clearProgramQueue()
{
    if(!mProgramEncoder.programQueue.empty())
    {
        mProgramEncoder.programQueue.clear();
        mProgramEncoder.poseQueue.clear();
    }
    if(!mProgramEncoder.poseQueue.empty())
    {
        mProgramEncoder.clearQueues();
    }
}

void Ur5Robot::stopMove(QString typeOfStop, double acc)
{
    if(typeOfStop=="stopl")
        sendMessage(mMessageEncoder.stopl(acc));
    else if(typeOfStop=="stopj")
        sendMessage(mMessageEncoder.stopj(acc));
}

void Ur5Robot::openVTKfile(QString filename)
{
    mProgramEncoder.openVTKfile(filename);
}

void Ur5Robot::moveProgram(QString typeOfProgram,double acceleration,double velocity, double radius, double t)
{
    if(typeOfProgram == "movej")
    {
        mInitialState = this->getCurrentState();
        Ur5State initState;
        initState.cartAxis = mProgramEncoder.poseQueue[0].cartAxis; //+ mStartPosition.cartAxis;
        initState.cartAngles = mInitialState.cartAngles;
        this->move("movej",initState,acceleration,velocity);
        isMoveInProgress=true;
        moveAcceleration=acceleration;
        moveVelocity=velocity;
    }
    else if(typeOfProgram == "movej2")
    {
        this->move("movej",mProgramEncoder.jointPositionQueue[0],acceleration,velocity);
        isMoveInProgress=true;
        moveAcceleration=acceleration;
        moveVelocity=velocity;
    }
    else if(typeOfProgram == "speedj")
    {
        mInitialState = this->getCurrentState();
        Ur5State initState;
        initState.cartAxis = mProgramEncoder.poseQueue[0].cartAxis;
        initState.cartAngles = mInitialState.cartAngles;
        this->move("movej",initState,acceleration,velocity);
        moveAcceleration=acceleration;
        moveVelocity=velocity;
        isVelocityMoveInProgress=true;
    }
    else
    {
        return;
    }
}

{
    if((mCurrentState.cartAxis-mTargetState.cartAxis).length()<mBlendRadius)
    {
        emit atTarget();
        return true;
    }
    return false;
}

void Ur5Robot::setBlendRadius(double blendRadius)
{
    std::cout << "Blend radius set to " << blendRadius << std::endl;
    mBlendRadius=blendRadius;
}

std::vector<QString> Ur5Robot::getProgramQueue()
{
    return this->mProgramEncoder.programQueue;
}

bool Ur5Robot::isValidWorkspace()
{
    return(abs(this->getCurrentState().jointConfiguration.maxCoeff())<=2*3.15);
}

bool Ur5Robot::isValidWorkspace(Eigen::RowVectorXd jointPosition)
{
    return(abs(jointPosition.maxCoeff())<=2*3.15);
}

void Ur5Robot::set_eMt(Transform3D eMt)
{
    this->eMt = eMt;
    this->set_tcp(eMt);
    emit eMtChanged(eMt);
}

Transform3D Ur5Robot::get_eMt()
{
    return (this->eMt);
}

void Ur5Robot::set_tcp(Transform3D eMt)
{
    this->sendMessage(mMessageEncoder.set_tcp(eMt));
}

} // cx

