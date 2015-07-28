#include "StepperMotorController.h"
#include "ui_qcstmconfigmonitoring.h"

StepperMotorController::StepperMotorController() {

	_stepper = 0;

}

StepperMotorController::~StepperMotorController()
{
}


int CCONV AttachHandler(CPhidgetHandle stepper, void * userptr) {

	int serialNo;
	const char *name;

	CPhidget_getDeviceName (stepper, &name);
	CPhidget_getSerialNumber(stepper, &serialNo);
	printf("%s %10d attached!\n", name, serialNo);

	return 0;
}



int CCONV DetachHandler(CPhidgetHandle stepper, void *userptr)
{
	int serialNo;
	const char *name;

	CPhidget_getDeviceName (stepper, &name);
	CPhidget_getSerialNumber(stepper, &serialNo);
	printf("%s %10d detached!\n", name, serialNo);

	return 0;
}

int CCONV ErrorHandler(CPhidgetHandle stepper, void *userptr, int ErrorCode, const char *Description)
{
	printf("Error handled. %d - %s\n", ErrorCode, Description);
	return 0;
}

int CCONV PositionChangeHandler(CPhidgetStepperHandle stepper, void *usrptr, int Index, __int64 Value)
{
	printf("Motor: %d > Current Position: %lld\n", Index, Value);
	return 0;
}

//Display the properties of the attached phidget to the screen.  We will be displaying the name, serial number and version of the attached device.
int StepperMotorController::display_properties(CPhidgetStepperHandle phid)
{
	int serialNo, version, numMotors;
	const char* ptr;

	CPhidget_getDeviceType((CPhidgetHandle)phid, &ptr);
	CPhidget_getSerialNumber((CPhidgetHandle)phid, &serialNo);
	CPhidget_getDeviceVersion((CPhidgetHandle)phid, &version);

	CPhidgetStepper_getMotorCount (phid, &numMotors);

	printf("       Hardware: %s | Serial Number: %d | Version: %d | number of motors: %d\n", ptr, serialNo, version, numMotors);

	return 0;
}

void StepperMotorController::arm_stepper(Ui::QCstmConfigMonitoring * ui) {

	int result;
	const char *err;

	//create the stepper object
	_stepper = 0;
	CPhidgetStepper_create(&_stepper);

	// Set the handlers to be run when the device is plugged in or opened from software, unplugged or closed from software, or generates an error.
	CPhidget_set_OnAttach_Handler( (CPhidgetHandle)_stepper, AttachHandler, NULL );
	CPhidget_set_OnDetach_Handler( (CPhidgetHandle)_stepper, DetachHandler, NULL );
	CPhidget_set_OnError_Handler ( (CPhidgetHandle)_stepper, ErrorHandler , NULL );
	// Registers a callback that will run when the motor position is changed.
	// Requires the handle for the Phidget, the function that will be called, and an arbitrary pointer that will be supplied to the callback function (may be NULL).
	CPhidgetStepper_set_OnPositionChange_Handler( _stepper, PositionChangeHandler, NULL );

	//open the device for connections
	CPhidget_open((CPhidgetHandle)_stepper, -1);

	//get the program to wait for an stepper device to be attached
	printf("[STEP] Waiting for Phidget to be attached....");
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)_stepper, 10000)))
	{
		CPhidget_getErrorDescription(result, &err);
		printf("Problem waiting for attachment: %s\n", err);
		//return 0;
	}
	//Display the properties of the attached device
	display_properties( _stepper );

	// Extract the parameters for the motors connected
	int numMotors;
	double tval;
	CPhidgetStepper_getMotorCount (_stepper, &numMotors);
	for ( int motorid = 0 ; motorid < numMotors ; motorid++ ) {

		// Velocity
		CPhidgetStepper_getVelocityMin(_stepper, motorid, &tval);
		_parsMap[motorid].minVel = tval;
		CPhidgetStepper_getVelocityMax(_stepper, motorid, &tval);
		_parsMap[motorid].maxVel = tval;
		_parsMap[motorid].vel = _parsMap[motorid].maxVel / 2.; // set the limit at middle range

		CPhidgetStepper_setVelocityLimit(_stepper, motorid, _parsMap[motorid].vel);

		// Acc
		CPhidgetStepper_getAccelerationMin(_stepper, motorid, &tval);
		_parsMap[motorid].minAcc = tval;
		CPhidgetStepper_getAccelerationMax(_stepper, motorid, &tval);
		_parsMap[motorid].maxAcc = tval;
		_parsMap[motorid].acc = _parsMap[motorid].minAcc * 2.; // settle at double the minimum acc

		CPhidgetStepper_setAcceleration(_stepper, motorid, _parsMap[motorid].acc);

		// Pos
		long long int curr_pos = 0;
		if ( CPhidgetStepper_getCurrentPosition(_stepper, motorid, &curr_pos) == EPHIDGET_OK ) {
				printf("       Motor: %d > Current Position: %lld\n", motorid, curr_pos);
		}
		_parsMap[motorid].currPos = curr_pos;
		_parsMap[motorid].targetPos = _parsMap[motorid].currPos;

		// Set current position and engage
		CPhidgetStepper_setCurrentPosition( _stepper, motorid, _parsMap[motorid].currPos );
		CPhidgetStepper_setEngaged( _stepper, motorid, PTRUE );

	}


	// Reflex certain things in the GUI
	ui->motorIdSpinBox->setMinimum( 0 );
	ui->motorIdSpinBox->setMaximum( numMotors - 1 );

	//return 1;
}

void StepperMotorController::PropagateParsToGUI(Ui::QCstmConfigMonitoring * ui){

	int motorid = ui->motorIdSpinBox->value();

	ui->speedSpinBox->setValue( _parsMap[motorid].vel );
	ui->accelerationSpinBox->setValue( _parsMap[motorid].acc );
	QString posS;
	posS = QString::number( _parsMap[motorid].currPos , 'lld', 0 );
	ui->motorCurrentPosLabel->setText( posS );

}

void StepperMotorController::disarm_stepper() {

	printf("[STEP] Closing and deleting stepper handler.\n");
	CPhidget_close((CPhidgetHandle)_stepper);
	CPhidget_delete((CPhidgetHandle)_stepper);
	_stepper = 0;

}

int StepperMotorController::stepper_simple(int motorid)
{

	int result;
	__int64 curr_pos;
	const char *err;
	double minAccel, maxVel;
	int stopped;

	//Declare an stepper handle
	CPhidgetStepperHandle stepper = 0;

	//create the stepper object
	CPhidgetStepper_create(&stepper);

	//Set the handlers to be run when the device is plugged in or opened from software, unplugged or closed from software, or generates an error.
	CPhidget_set_OnAttach_Handler( (CPhidgetHandle)stepper, AttachHandler, NULL );
	CPhidget_set_OnDetach_Handler( (CPhidgetHandle)stepper, DetachHandler, NULL );
	CPhidget_set_OnError_Handler ( (CPhidgetHandle)stepper, ErrorHandler , NULL );

	//Registers a callback that will run when the motor position is changed.
	//Requires the handle for the Phidget, the function that will be called, and an arbitrary pointer that will be supplied to the callback function (may be NULL).
	CPhidgetStepper_set_OnPositionChange_Handler( stepper, PositionChangeHandler, NULL );

	//open the device for connections
	CPhidget_open((CPhidgetHandle)stepper, -1);

	//get the program to wait for an stepper device to be attached
	printf("Waiting for Phidget to be attached....");
	if((result = CPhidget_waitForAttachment((CPhidgetHandle)stepper, 10000)))
	{
		CPhidget_getErrorDescription(result, &err);
		printf("Problem waiting for attachment: %s\n", err);
		return 0;
	}

	//Display the properties of the attached device
	display_properties( stepper );

	//read event data
	printf("Reading.....\n");

	//This example assumes stepper motor is attached to index 0

	//Set up some initial acceleration and velocity values
	CPhidgetStepper_getAccelerationMin(stepper, motorid, &minAccel);
	CPhidgetStepper_setAcceleration(stepper, motorid, minAccel*2);
	CPhidgetStepper_getVelocityMax(stepper, motorid, &maxVel);
	CPhidgetStepper_setVelocityLimit(stepper, motorid, maxVel/2);

	//display current motor position if available
	if ( CPhidgetStepper_getCurrentPosition(stepper, motorid, &curr_pos) == EPHIDGET_OK )
		printf("Motor: 0 > Current Position: %lld\n", curr_pos);

	//keep displaying stepper event data until user input is read
	printf("Press any key to continue\n");
	getchar();

	//change the motor position
	//we'll set it to a few random positions to move it around

	//Step 1: Position 0 - also engage stepper
	printf("Set to position 0 and engage. Press any key to Continue\n");
	getchar();

	CPhidgetStepper_setCurrentPosition(stepper, motorid, 0);
	CPhidgetStepper_setEngaged(stepper, motorid, 1);

	//Step 2: Position 200
	printf("Move to position 200. Press any key to Continue\n");
	getchar();

	CPhidgetStepper_setTargetPosition (stepper, motorid, 200);

	//Step 3: Position -1200
	//printf("Move to position -1200. Press any key to Continue\n");
	//getchar();
	//CPhidgetStepper_setTargetPosition (stepper, motorid, -1200);

	//Step 4: Set to 0, wait for it to reach position, Disengage
	printf("Reseting to 0 and disengaging motor. Press any key to Continue\n");
	getchar();

	CPhidgetStepper_setTargetPosition(stepper, motorid, 0);

	stopped = PFALSE;
	while(!stopped)
	{
		CPhidgetStepper_getStopped(stepper, motorid, &stopped);
		//usleep(100000);
	}

	CPhidgetStepper_setEngaged(stepper, motorid, 0);

	printf("Press any key to end\n");
	getchar();

	//since user input has been read, this is a signal to terminate the program so we will close the phidget and delete the object we created
	printf("Closing...\n");
	CPhidget_close((CPhidgetHandle)stepper);
	CPhidget_delete((CPhidgetHandle)stepper);

	//all done, exit
	return 0;
}

void StepperMotorController::goToTarget(long long int targetPos, int motorid) {

	CPhidgetStepper_setTargetPosition (_stepper, motorid, targetPos);

}
