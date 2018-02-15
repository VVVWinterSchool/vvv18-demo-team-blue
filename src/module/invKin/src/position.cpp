// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
//
// A tutorial on how to use the Gaze Interface.
//
// Author: Ugo Pattacini - <ugo.pattacini@iit.it>

#include <cmath>

#include <yarp/os/Network.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RpcServer.h>
#include <yarp/os/Time.h>

#include <yarp/dev/Drivers.h>
#include <yarp/dev/IControlLimits2.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IControlMode.h>
#include <yarp/dev/IPositionControl2.h>
#include <yarp/dev/PolyDriver.h>

using namespace yarp::os;
using namespace yarp::dev;


class CtrlModule: public RFModule
{
protected:
    PolyDriver         clienJoint;
    IControlLimits2   *ilim;
    IEncoders         *ienc;
    IControlMode2     *imod;
    IPositionControl2 *ipos;

    RpcServer rpc;
    int joint;
    int fingers;
    std::vector<int> jointList ={8,9,10,11,12,13,14,15};
    std::vector<int> finger_joint_values = {70,15,180,4,7,85,150,240};

    void right()
    {
        for (int i=0;i<jointList.size();i++)
        {
            fingers = jointList[i];
            imod->setControlMode(fingers,VOCAB_CM_POSITION);
            ipos->positionMove(fingers,finger_joint_values[i]);
            bool done=false;
                        double t0=Time::now();
                        while (!done&&(Time::now()-t0<10))
                        {
                            yInfo()<<"Waiting...";
                            Time::delay(0.1);   // release the quantum to avoid starving resources
                            ipos->checkMotionDone(&done);
                        }

                        if (done)
                        {
                            yInfo()<<"Highfive completed";

                        }
                        else
                        {
                            yWarning()<<"Timeout expired";

                        }
        }
        // retrieve joint bounds
        double min,max,range;
        ilim->getLimits(joint,&min,&max);
        range=max-min;

        // retrieve current joint position
        double enc;
        ienc->getEncoder(joint,&enc);

        // select target
        double target;
        if (fabs(enc-min)<fabs(enc-max))
            target=max-0.5*range;
        else
            target=min+0.5*range;

        // set control mode
        imod->setControlMode(joint,VOCAB_CM_POSITION);

        // set up the speed in [deg/s]
        ipos->setRefSpeed(joint,30.0);

        // set up max acceleration in [deg/s^2]
        ipos->setRefAcceleration(joint,100.0);

        // yield the actual movement
        yInfo()<<"Yielding new target: "<<target<<" [deg]";
        ipos->positionMove(joint,target);

        // wait (with timeout) until the movement is completed
        bool done=false;
        double t0=Time::now();
        while (!done&&(Time::now()-t0<10.0))
        {
            yInfo()<<"Waiting...";
            Time::delay(0.1);   // release the quantum to avoid starving resources
            ipos->checkMotionDone(&done);
        }

        if (done)
            yInfo()<<"Movement completed";
        else
            yWarning()<<"Timeout expired";
    }

    void left()
    {
        for (int i=0;i<jointList.size();i++)
        {
            fingers = jointList[i];
            imod->setControlMode(fingers,VOCAB_CM_POSITION);
            ipos->positionMove(fingers,finger_joint_values[i]);
            bool done=false;
                        double t0=Time::now();
                        while (!done&&(Time::now()-t0<10))
                        {
                            yInfo()<<"Waiting...";
                            Time::delay(0.1);   // release the quantum to avoid starving resources
                            ipos->checkMotionDone(&done);
                        }

                        if (done)
                        {
                            yInfo()<<"Highfive completed";

                        }
                        else
                        {
                            yWarning()<<"Timeout expired";

                        }
        }
        // retrieve joint bounds
        double min,max,range;
        ilim->getLimits(joint,&min,&max);
        range=max-min;

        // retrieve current joint position
        double enc;
        ienc->getEncoder(joint,&enc);

        // select target
        double target;
        if (fabs(enc-min)<fabs(enc-max))
            target=max-0.1*range;
        else
            target=min+0.1*range;

        // set control mode
        imod->setControlMode(joint,VOCAB_CM_POSITION);

        // set up the speed in [deg/s]
        ipos->setRefSpeed(joint,30.0);

        // set up max acceleration in [deg/s^2]
        ipos->setRefAcceleration(joint,100.0);

        // yield the actual movement
        yInfo()<<"Yielding new target: "<<target<<" [deg]";
        ipos->positionMove(joint,target);

        // wait (with timeout) until the movement is completed
        bool done=false;
        double t0=Time::now();
        while (!done&&(Time::now()-t0<10.0))
        {
            yInfo()<<"Waiting...";
            Time::delay(0.1);   // release the quantum to avoid starving resources
            ipos->checkMotionDone(&done);
        }

        if (done)
            yInfo()<<"Movement completed";
        else
            yWarning()<<"Timeout expired";
    }

public:
    virtual bool configure(ResourceFinder &rf)
    {
        // open a client interface to connect to the joint controller
        Property optJoint;
        optJoint.put("device","remote_controlboard");
        optJoint.put("remote","/icubSim/right_arm");
        optJoint.put("local","/robot/point_object/rpc/right_arm");

        if (!clienJoint.open(optJoint))
        {
            yError()<<"Unable to connect to /icubSim/right_arm";
            return false;
        }

        // open views
        bool ok=true;
        ok=ok && clienJoint.view(ilim);
        ok=ok && clienJoint.view(ienc);
        ok=ok && clienJoint.view(imod);
        ok=ok && clienJoint.view(ipos);

        if (!ok)
        {
            yError()<<"Unable to open views";
            return false;
        }

        // elbow
        joint=3;

        // open rpc port
        rpc.open("/robot/point_object/rpc");

        // attach the callback respond()
        attach(rpc);

        return true;
    }

    virtual bool close()
    {
        rpc.close();
        clienJoint.close();

        return true;
    }

    virtual bool respond(const Bottle &cmd, Bottle &reply)
    {
        if(cmd.get(0).asString()=="point")
        {
            if(cmd.size()==4)
            {
                if (cmd.get(2).asDouble()>0)//=="right")
                {
                    right();
                    reply.addString("pointed right");
                }
                else if (cmd.get(2).asDouble()<0)//=="left")
                {
                    left();
                    reply.addString("pointed left");
                }
                else reply.addString("Not pointing");
            } else reply.addString("Incorrect input");
        }


        else if (cmd.get(0).asString()=="high")
        {
            return true;
        }
        else if (cmd.get(0).asString()=="low")
        {
            return false;
        }
        else
            reply.addString("nack");

        return true;
    }

    virtual double getPeriod()
    {
        return 1.0;
    }

    virtual bool updateModule()
    {
        return true;
    }
};



int main(int argc, char *argv[])
{
    Network yarp;
    if (!yarp.checkNetwork())
    {
        yError()<<"YARP doesn't seem to be available";
        return 1;
    }

    ResourceFinder rf;
    rf.configure(argc,argv);

    CtrlModule mod;
    return mod.runModule(rf);
}
