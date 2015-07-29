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
} motorPars;

class StepperMotorController  {

public:

	StepperMotorController();
	~StepperMotorController();

	// getters
	int getNumMotors(){ return _numMotors; }

	// Motors commands
	void arm_stepper();//<! returns the id's of motors attached
	void disarm_stepper();
	int display_properties(CPhidgetStepperHandle phid);
	void goToTarget(long long int targetPos, int motorid = 0);

	// Parameters
	map<int, motorPars> getPars();

	// Others
	int stepper_simple(int motorid = 0); //<! Just a test routine

private:

	int _numMotors;
	CPhidgetStepperHandle _stepper;
	map<int, motorPars> _parsMap;

};

#endif // STEPPERMOTORCONTROLLER_H
