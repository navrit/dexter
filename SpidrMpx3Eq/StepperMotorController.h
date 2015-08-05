#ifndef STEPPERMOTORCONTROLLER_H
#define STEPPERMOTORCONTROLLER_H

#include <stdio.h>
#include <phidget21.h>

#include <iostream>
#include <vector>
#include <map>
using namespace std;


int CCONV AttachHandler(CPhidgetHandle stepper, void *userptr);
int CCONV DetachHandler(CPhidgetHandle stepper, void *userptr);
int CCONV ErrorHandler(CPhidgetHandle stepper, void *userptr, int ErrorCode, const char *Description);
int CCONV PositionChangeHandler(CPhidgetStepperHandle stepper, void *usrptr, int Index, __int64 Value);

typedef struct {
	double maxVel;
	double minVel;
	double vel;
	double maxAcc;
	double minAcc;
	double acc;
	long long int currPos;
	long long int targetPos;
	bool calibOK;
	double calibSlope;  // for Angle(step) function
	double calibCut;
	double calibSlopeInv;  // for Step(angle) function
	double calibCutInv;
} motorPars;

class StepperMotorController  {

public:

	StepperMotorController();
	~StepperMotorController();

	// getters
	int getNumMotors(){ return _numMotors; }

	// Motors commands
	bool arm_stepper();//<! returns the id's of motors attached
	void disarm_stepper();
	int display_properties(CPhidgetStepperHandle phid);
	void goToTarget(long long int targetPos, int motorid = 0);

	// Parameters
	map<int, motorPars> getPars();
	void ClearParsMap();
	void setCurrPos(int motorid, long long int currPos) { _parsMap[motorid].currPos = currPos; }
	long long int getCurrPos(int motorid);

	void setTargetPos(int motorid, long long int targetPos) { _parsMap[motorid].targetPos = targetPos; }
	long long int getTargetPos(int motorid) { return _parsMap[motorid].targetPos; }
	void setZero(int motorid);

	void SetAcceleration(int motorid, double val);
	void SetSpeed(int motorid, double val);

	// Calibration
	bool SetStepAngleCalibration(int motorid, vector<pair<double, double> > points);
	double StepToAngle(int motorid, double step);
	double AngleToStep(int motorid, double angle);


	// Others
	int stepper_simple(int motorid = 0); //<! Just a test routine
	bool isStepperReady();


private:

	int _numMotors;
	CPhidgetStepperHandle _stepper;
	map<int, motorPars> _parsMap;

};

#endif // STEPPERMOTORCONTROLLER_H
