#ifndef STEPPERMOTORCONTROLLER_H
#define STEPPERMOTORCONTROLLER_H

#include <stdio.h>
#include <phidget21.h>

#include <iostream>
#include <vector>
using namespace std;

#include "qcstmconfigmonitoring.h"

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

	int display_properties(CPhidgetStepperHandle phid);
	void arm_stepper(Ui::QCstmConfigMonitoring * ui);//<! returns the id's of motors attached
	void disarm_stepper();

	void goToTarget(long long int targetPos, int motorid = 0);

	int stepper_simple(int motorid = 0); //<! Just a test routine
	void PropagateParsToGUI(Ui::QCstmConfigMonitoring * ui);

private:

	CPhidgetStepperHandle _stepper;
	map<int, motorPars> _parsMap;

};

#endif // STEPPERMOTORCONTROLLER_H
