/**
 * John Idarraga <idarraga@cern.ch>
 * Nikhef, 2014.
 */

#ifndef THLSCAN_H
#define THLSCAN_H

class SpidrController;
class SpidrDaq;

class ThlScan {

public:
	ThlScan();
	ThlScan(SpidrController *, SpidrDaq *);
	~ThlScan();

	void DoScan();
	void SetMask(int spacing, int offset);

private:

	SpidrController * _spidrcntrl;
	SpidrDaq * _spidrdaq;
};

#endif

