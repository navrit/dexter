// John Idarraga 2015

#include "StepperMotorController.h"

StepperMotorController::StepperMotorController() { _stepper = nullptr; }

StepperMotorController::~StepperMotorController() {}

int CCONV AttachHandler(CPhidgetHandle stepper, void *userptr) {
  (void)(userptr);

  int serialNo;
  const char *name;

  CPhidget_getDeviceName(stepper, &name);
  CPhidget_getSerialNumber(stepper, &serialNo);
  printf("%s %10d attached!\n", name, serialNo);

  return 0;
}

int CCONV DetachHandler(CPhidgetHandle stepper, void *userptr) {
  (void)(userptr);
  int serialNo;
  const char *name;

  CPhidget_getDeviceName(stepper, &name);
  CPhidget_getSerialNumber(stepper, &serialNo);
  printf("%s %10d detached!\n", name, serialNo);

  return 0;
}

int CCONV ErrorHandler(CPhidgetHandle stepper, void *userptr, int ErrorCode,
                       const char *Description) {
  (void)(stepper);
  (void)(userptr);
  printf("Error handled. %d - %s\n", ErrorCode, Description);
  return 0;
}

int CCONV PositionChangeHandler(CPhidgetStepperHandle stepper, void *usrptr,
                                int Index, __int64 Value) {
  (void)(stepper);
  (void)(usrptr);
  (void)(Index);
  (void)(Value);
  // printf("Motor: %d > Current Position: %lld\n", Index, Value);
  return 0;
}

// Display the properties of the attached phidget to the screen.  We will be
// displaying the name, serial number and version of the attached device.
int StepperMotorController::display_properties(CPhidgetStepperHandle phid) {
  int serialNo, version, numMotors;
  const char *ptr;

  CPhidget_getDeviceType(CPhidgetHandle(phid), &ptr);
  CPhidget_getSerialNumber(CPhidgetHandle(phid), &serialNo);
  CPhidget_getDeviceVersion(CPhidgetHandle(phid), &version);

  CPhidgetStepper_getMotorCount(phid, &numMotors);

  printf("       Hardware: %s | Serial Number: %d | Version: %d | number of "
         "motors: %d\n",
         ptr, serialNo, version, numMotors);

  return 0;
}

bool StepperMotorController::arm_stepper() {

  int result;
  const char *err;

  // create the stepper object
  _stepper = nullptr;
  CPhidgetStepper_create(&_stepper);

  // Set the handlers to be run when the device is plugged in or opened from
  // software, unplugged or closed from software, or generates an error.
  CPhidget_set_OnAttach_Handler(CPhidgetHandle(_stepper), AttachHandler,
                                nullptr);
  CPhidget_set_OnDetach_Handler(CPhidgetHandle(_stepper), DetachHandler,
                                nullptr);
  CPhidget_set_OnError_Handler(CPhidgetHandle(_stepper), ErrorHandler, nullptr);
  // Registers a callback that will run when the motor position is changed.
  // Requires the handle for the Phidget, the function that will be called, and
  // an arbitrary pointer that will be supplied to the callback function (may be
  // NULL).
  CPhidgetStepper_set_OnPositionChange_Handler(_stepper, PositionChangeHandler,
                                               nullptr);

  // open the device for connections
  CPhidget_open(CPhidgetHandle(_stepper), -1);

  // get the program to wait for an stepper device to be attached
  printf("[STEP] Waiting for Phidget to be attached....");
  if ((result = CPhidget_waitForAttachment(CPhidgetHandle(_stepper), 10000))) {
    CPhidget_getErrorDescription(result, &err);
    printf("Problem waiting for attachment: %s\n", err);
    return false;
  }
  // Display the properties of the attached device
  display_properties(_stepper);

  // Extract the parameters for the motors connected
  int numMotors;
  double tval;
  CPhidgetStepper_getMotorCount(_stepper, &numMotors);

  // Set the number of motors supported by the board connected
  _numMotors = numMotors;

  for (int motorid = 0; motorid < numMotors; motorid++) {

    // Velocity
    CPhidgetStepper_getVelocityMin(_stepper, motorid, &tval);
    _parsMap[motorid].minVel = tval;
    CPhidgetStepper_getVelocityMax(_stepper, motorid, &tval);
    _parsMap[motorid].maxVel = tval;
    _parsMap[motorid].vel =
        _parsMap[motorid].maxVel / 2.; // set the limit at middle range

    CPhidgetStepper_setVelocityLimit(_stepper, motorid, _parsMap[motorid].vel);

    // Acc
    CPhidgetStepper_getAccelerationMin(_stepper, motorid, &tval);
    _parsMap[motorid].minAcc = tval;
    CPhidgetStepper_getAccelerationMax(_stepper, motorid, &tval);
    _parsMap[motorid].maxAcc = tval;
    _parsMap[motorid].acc =
        _parsMap[motorid].minAcc * 2.; // settle at double the minimum acc

    CPhidgetStepper_setAcceleration(_stepper, motorid, _parsMap[motorid].acc);

    // CurrentLimit
    double limit = 0.0;
    CPhidgetStepper_getCurrentLimit(_stepper, motorid, &limit);
    cout << "limit : " << limit << endl;
    _parsMap[motorid].currentILimit = limit;

    CPhidgetStepper_setCurrentLimit(_stepper, motorid, limit);

    // Pos
    long long int curr_pos = 0;
    if (CPhidgetStepper_getCurrentPosition(_stepper, motorid, &curr_pos) ==
        EPHIDGET_OK) {
      printf("       Motor: %d > Current Position: %lld\n", motorid, curr_pos);
    }
    _parsMap[motorid].currPos = curr_pos;
    _parsMap[motorid].targetPos = _parsMap[motorid].currPos;

    // Set current position and engage
    CPhidgetStepper_setCurrentPosition(_stepper, motorid,
                                       _parsMap[motorid].currPos);
    CPhidgetStepper_setEngaged(_stepper, motorid, PTRUE);

    // Calibration not present yet
    _parsMap[motorid].calibOK = false;
  }

  return true;
}

map<int, motorPars> StepperMotorController::getPars() { return _parsMap; }

long long int StepperMotorController::getCurrPos(int motorid) {

  // check the hardware the motor may have moved
  long long int curr_pos = 0;
  if (CPhidgetStepper_getCurrentPosition(_stepper, motorid, &curr_pos) ==
      EPHIDGET_OK) {
    _parsMap[motorid].currPos = curr_pos;
  }
  return _parsMap[motorid].currPos;
}

void StepperMotorController::setZero(int motorid) {

  // set in the hardware
  CPhidgetStepper_setCurrentPosition(_stepper, motorid, 0);

  // reset data structure
  _parsMap[motorid].targetPos = 0;
  _parsMap[motorid].currPos = 0;
}

void StepperMotorController::SetAcceleration(int motorid, double val) {

  _parsMap[motorid].acc = val;
  CPhidgetStepper_setAcceleration(_stepper, motorid, _parsMap[motorid].acc);
}

long long int StepperMotorController::getPositionMin(int motorid) {

  long long int pos;
  CPhidgetStepper_getPositionMin(_stepper, motorid, &pos);

  return pos;
}

long long int StepperMotorController::getPositionMax(int motorid) {

  long long int pos;
  CPhidgetStepper_getPositionMax(_stepper, motorid, &pos);

  return pos;
}

void StepperMotorController::SetSpeed(int motorid, double val) {

  _parsMap[motorid].vel = val;
  CPhidgetStepper_setVelocityLimit(_stepper, motorid, _parsMap[motorid].vel);
}

void StepperMotorController::SetCurrentILimit(int motorid, double val) {

  _parsMap[motorid].currentILimit = val;
  CPhidgetStepper_setCurrentLimit(_stepper, motorid,
                                  _parsMap[motorid].currentILimit);

  // cout << "current limit : " << _parsMap[motorid].currentILimit << endl;
}

void StepperMotorController::disarm_stepper() {

  printf("[STEP] Closing and deleting stepper handler.\n");
  CPhidget_close(CPhidgetHandle(_stepper));
  CPhidget_delete(CPhidgetHandle(_stepper));
  _stepper = nullptr;
}

bool StepperMotorController::isStepperReady() {
  if (_stepper)
    return true;
  return false;
}

int StepperMotorController::stepper_simple(int motorid) {

  int result;
  __int64 curr_pos;
  const char *err;
  double minAccel, maxVel;
  int stopped;

  // Declare an stepper handle
  CPhidgetStepperHandle stepper = nullptr;

  // create the stepper object
  CPhidgetStepper_create(&stepper);

  // Set the handlers to be run when the device is plugged in or opened from
  // software, unplugged or closed from software, or generates an error.
  CPhidget_set_OnAttach_Handler(CPhidgetHandle(stepper), AttachHandler,
                                nullptr);
  CPhidget_set_OnDetach_Handler(CPhidgetHandle(stepper), DetachHandler,
                                nullptr);
  CPhidget_set_OnError_Handler(CPhidgetHandle(stepper), ErrorHandler, nullptr);

  // Registers a callback that will run when the motor position is changed.
  // Requires the handle for the Phidget, the function that will be called, and
  // an arbitrary pointer that will be supplied to the callback function (may be
  // NULL).
  CPhidgetStepper_set_OnPositionChange_Handler(stepper, PositionChangeHandler,
                                               nullptr);

  // open the device for connections
  CPhidget_open(CPhidgetHandle(stepper), -1);

  // get the program to wait for an stepper device to be attached
  printf("Waiting for Phidget to be attached....");
  if ((result = CPhidget_waitForAttachment(CPhidgetHandle(stepper), 10000))) {
    CPhidget_getErrorDescription(result, &err);
    printf("Problem waiting for attachment: %s\n", err);
    return 0;
  }

  // Display the properties of the attached device
  display_properties(stepper);

  // read event data
  printf("Reading.....\n");

  // This example assumes stepper motor is attached to index 0

  // Set up some initial acceleration and velocity values
  CPhidgetStepper_getAccelerationMin(stepper, motorid, &minAccel);
  CPhidgetStepper_setAcceleration(stepper, motorid, minAccel * 2);
  CPhidgetStepper_getVelocityMax(stepper, motorid, &maxVel);
  CPhidgetStepper_setVelocityLimit(stepper, motorid, maxVel / 2);

  // display current motor position if available
  if (CPhidgetStepper_getCurrentPosition(stepper, motorid, &curr_pos) ==
      EPHIDGET_OK)
    printf("Motor: %d > Current Position: %lld\n", motorid, curr_pos);

  // keep displaying stepper event data until user input is read
  printf("Press any key to continue\n");
  getchar();

  // change the motor position
  // we'll set it to a few random positions to move it around

  // Step 1: Position 0 - also engage stepper
  printf("Set to position 0 and engage. Press any key to Continue\n");
  getchar();

  CPhidgetStepper_setCurrentPosition(stepper, motorid, 0);
  CPhidgetStepper_setEngaged(stepper, motorid, 1);

  // Step 2: Position 200
  printf("Move to position 200. Press any key to Continue\n");
  getchar();

  CPhidgetStepper_setTargetPosition(stepper, motorid, 200);

  // Step 3: Position -1200
  // printf("Move to position -1200. Press any key to Continue\n");
  // getchar();
  // CPhidgetStepper_setTargetPosition (stepper, motorid, -1200);

  // Step 4: Set to 0, wait for it to reach position, Disengage
  printf("Reseting to 0 and disengaging motor. Press any key to Continue\n");
  getchar();

  CPhidgetStepper_setTargetPosition(stepper, motorid, 0);

  stopped = PFALSE;
  while (!stopped) {
    CPhidgetStepper_getStopped(stepper, motorid, &stopped);
    // usleep(100000);
  }

  CPhidgetStepper_setEngaged(stepper, motorid, 0);

  printf("Press any key to end\n");
  getchar();

  // since user input has been read, this is a signal to terminate the program
  // so we will close the phidget and delete the object we created
  printf("Closing...\n");
  CPhidget_close(CPhidgetHandle(stepper));
  CPhidget_delete(CPhidgetHandle(stepper));

  // all done, exit
  return 0;
}

void StepperMotorController::goToTarget(long long int targetPos, int motorid) {

  CPhidgetStepper_setTargetPosition(_stepper, motorid, targetPos);
}

/**
 * This establishes the relation Angle(steps).  The user may pass 2 or more
 * points. With 2 points a straight line can be made.  With more points a linear
 * regression.
 */

bool StepperMotorController::SetStepAngleCalibration(
    int motorid, vector<pair<double, double>> points) {

  // At least two points
  if (points.size() < 2) {
    cout << "[STEP] two few points to calibrate" << endl;
    return false;
  }

  // Data comes   pos, angle
  _parsMap[motorid].calibSlope = ((points[0].second - points[1].second) /
                                  (points[0].first - points[1].first));
  _parsMap[motorid].calibCut =
      points[1].second - (_parsMap[motorid].calibSlope * points[1].first);

  // Prepare the inverse
  _parsMap[motorid].calibSlopeInv = ((points[0].first - points[1].first) /
                                     (points[0].second - points[1].second));
  _parsMap[motorid].calibCutInv =
      points[1].first - (_parsMap[motorid].calibSlopeInv * points[1].second);

  // calib ok
  _parsMap[motorid].calibOK = true;

  return true;
}

double StepperMotorController::StepToAngle(int motorid, double step) {
  if (_parsMap[motorid].calibOK)
    return (_parsMap[motorid].calibSlope * step) + _parsMap[motorid].calibCut;
  return 0.0;
}

double StepperMotorController::AngleToStep(int motorid, double angle) {
  if (_parsMap[motorid].calibOK)
    return (_parsMap[motorid].calibSlopeInv * angle) +
           _parsMap[motorid].calibCutInv;
  return 0.0;
}

void StepperMotorController::ClearParsMap() {
  //_parsMap[motorid].
}
