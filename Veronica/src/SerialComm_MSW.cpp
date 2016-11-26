////////////////////////////////////////////////////////////
// SerialIO - Platform specific implementation file
// This one supports Microsoft windoze under Mingw
////////////////////////////////////////////////////////////
#include <windows.h>
#include <winnt.h>
#include <string.h>

#include "../../GaryCooper/ICommInterface.h"
#include "../../GaryCooper/SlidingBuf.h"
#include "SerialComm.h"

// Read/write buffer size (right here)
#define BUFSIZE	(4096)

// Operating system buffer size (kernel space)
#define BUFFERSIZE	(16 * 1024)

#define MAX_CYCLE_READ	(1024)
#define MAX_CYCLE_WRITE	(1024)

////////////////////////////////////////////////////////////
// Application needs to link with:
// WSock32
////////////////////////////////////////////////////////////
// Private implementation of the platform-specific handles
// and such
class CSerialCommImp
{
	public:
	// Serial port
	HANDLE m_hComm;
	int m_iLastError;

	// Data buffers
	CSlidingBuffer m_ReceivedData;
	CSlidingBuffer m_TransmitQueue;

	CSerialCommImp()
	{
		m_hComm = INVALID_HANDLE_VALUE;
		m_iLastError = CSerialComm_ERROR_NOERROR;
	}

	virtual ~CSerialCommImp()
	{
		close();
	}

	void close()
	{
		if(m_hComm != INVALID_HANDLE_VALUE)
		{
			// Clear the buffer
			FlushFileBuffers(m_hComm);
			PurgeComm(m_hComm,	PURGE_TXABORT | PURGE_TXCLEAR |
								PURGE_RXABORT | PURGE_RXCLEAR);

			CloseHandle(m_hComm);
			m_hComm = INVALID_HANDLE_VALUE;
		}
	}

	void readFromPort()
	{
		unsigned long bytesRead = 0;
		DWORD errors = 0;
		unsigned char buffer[BUFSIZE];
		int totalReadThisCycle = 0;

		// If the port is not open then there
		// is nothing to do
		if(m_hComm == INVALID_HANDLE_VALUE)
			return;

//		do
		{
			// Start out with the assumption that nothing
			// was read, and that there are no errors.
			bytesRead = 0;
			ClearCommError(m_hComm, &errors, 0);
			if(errors)
			{
#if 0
				#define CE_RXOVER           0x0001  // Receive Queue overflow
				#define CE_OVERRUN          0x0002  // Receive Overrun Error
				#define CE_RXPARITY         0x0004  // Receive Parity Error
				#define CE_FRAME            0x0008  // Receive Framing error
				#define CE_BREAK            0x0010  // Break Detected
				#define CE_TXFULL           0x0100  // TX Queue is full
				#define CE_PTO              0x0200  // LPTx Timeout
				#define CE_IOE              0x0400  // LPTx I/O Error
				#define CE_DNS              0x0800  // LPTx Device not selected
				#define CE_OOP              0x1000  // LPTx Out-Of-Paper
				#define CE_MODE             0x8000  // Requested mode unsupported


				if(errors & CE_RXOVER)
					errorMsg = wxString("CE_RXOVER");

				if(errors & CE_OVERRUN)
					errorMsg = wxString("CE_OVERRUN");

				if(errors & CE_RXPARITY)
					errorMsg = wxString("CE_RXPARITY");

		//		if(errors & CE_FRAME)
		//			errorMsg = wxString("CE_FRAME");

				if(errors & CE_BREAK)
					errorMsg = wxString("CE_BREAK");

				if(errors & CE_TXFULL)
					errorMsg = wxString("CE_TXFULL");

				if(errors & CE_PTO)
					errorMsg = wxString("CE_PTO");

				if(errors & CE_IOE)
					errorMsg = wxString("CE_IOE");

				if(errors & CE_DNS)
					errorMsg = wxString("CE_DNS");

				if(errors & CE_OOP)
					errorMsg = wxString("CE_OOP");

				if(errors & CE_MODE)
					errorMsg = wxString("CE_MODE");

				if(!errorMsg.IsEmpty() && !errorHasBeenDisplayed)
				{
					errorHasBeenDisplayed = true;
					wxMessageBox(errorMsg, wxString("Error CMCCom:Notify"),
								wxOK | wxICON_ERROR);
				}
#endif

				// Completely generic error
				m_iLastError = CSerialComm_ERROR_PLATFORM_00;

				// Clear the buffers on error
				PurgeComm(m_hComm,	PURGE_TXABORT | PURGE_TXCLEAR |
									PURGE_RXABORT | PURGE_RXCLEAR);
			}
			else
			{
				// Read data from the port
				ReadFile(m_hComm,buffer, sizeof(buffer), &bytesRead,0);

			}

			// And put it in the receive buffer
			if(bytesRead > 0)
			{
				m_ReceivedData.write(buffer, bytesRead);
				totalReadThisCycle += bytesRead;
			}
		}
//		while(bytesRead > 0 && totalReadThisCycle < MAX_CYCLE_READ);
	}

	void writeToPort()
	{
		unsigned long bytesWritten = 0;
		unsigned long dataLen;
		unsigned char buffer[BUFSIZE];
		int totalWrittenThisCycle = 0;

		// If the port is not open then there
		// is nothing to do
		if(m_hComm == INVALID_HANDLE_VALUE)
			return;

//		do
		{
			// Nothing written yet
			bytesWritten = 0;

			// If there is nothing to send then we are done
			if(m_TransmitQueue.bytesAvailable() > 0)
			{
				// Read from transmit queue
				dataLen = m_TransmitQueue.read(buffer, sizeof(buffer));

				// Write it to the file
				WriteFile(m_hComm, buffer, dataLen, &bytesWritten,0);
			}
			// Remove whatever we have written from the transmit queue
			if(bytesWritten > 0)
			{
				m_TransmitQueue.consume(bytesWritten);
				totalWrittenThisCycle += bytesWritten;
			}
		}
//		while(bytesWritten > 0 && totalWrittenThisCycle < MAX_CYCLE_WRITE);
	}
};

////////////////////////////////////////////////////////////
CSerialComm::CSerialComm()
{
	// Create the implementation
	m_pSerImp = new CSerialCommImp;
}

CSerialComm::~CSerialComm()
{
	if(isOpen())
		close();

	delete m_pSerImp;
}

int CSerialComm::open(const char * _pPort, int _iBaud, int _iBits, CSerialComm_ParityT _eParity, int _iStopBits)
{
	DCB dcb;
	COMMTIMEOUTS timeouts;
	DWORD baudFlag;
	DWORD parityFlag;
	DWORD stopBitsFlag;

	// Suppress warning
	_iBits = _iBits;

	// Close it if it is already open
	if(m_pSerImp->m_hComm != INVALID_HANDLE_VALUE)
		close();

	// If nothing is provided then return an error
	if(strlen(_pPort) == 0)
		return CSerialComm_ERROR_BAD_NAME;

	// Allow "NONE" to work
	if(strcmp(_pPort,CSerialComm_Null_Port) == 0)
	{
		close();
		return CSerialComm_ERROR_NOERROR;
	}

	// Figure the baud rates
	switch(_iBaud)
	{
	case 110:		baudFlag = CBR_110;		break;
	case 300:		baudFlag = CBR_300;		break;
	case 600:		baudFlag = CBR_600;		break;
	case 1200:		baudFlag = CBR_1200;	break;
	case 2400:		baudFlag = CBR_2400;	break;
	case 4800:		baudFlag = CBR_4800;	break;
	case 9600:		baudFlag = CBR_9600;	break;
	case 14400:		baudFlag = CBR_14400; 	break;
	case 19200:		baudFlag = CBR_19200; 	break;
	case 38400:		baudFlag = CBR_38400;	break;
	case 56000:		baudFlag = CBR_56000; 	break;
	case 57600:		baudFlag = CBR_57600; 	break;
	case 115200:	baudFlag = CBR_115200; 	break;
	case 128000:	baudFlag = CBR_128000; 	break;
	case 256000:	baudFlag = CBR_256000; 	break;
	default: return CSerialComm_ERROR_INVALID_BAUD;
	}

	// Figure the parity
	// 0=N, 1=Even, 2=Odd
	switch(_eParity)
	{
	case CSerialComm_Parity_None: parityFlag = NOPARITY;	break;
	case CSerialComm_Parity_Even: parityFlag = EVENPARITY;	break;
	case CSerialComm_Parity_Odd: parityFlag = ODDPARITY;	break;
	default: return CSerialComm_ERROR_INVALID_PARITY;
	}

	// Figure the stop bits
	switch(_iStopBits)
	{
	case 1:	stopBitsFlag = ONESTOPBIT;	break;
	case 2: stopBitsFlag = TWOSTOPBITS;	break;
	default: return CSerialComm_ERROR_INVALID_STOPBITS;
	}


	// Now open the new one
	m_pSerImp->m_hComm = CreateFile(_pPort,
								GENERIC_READ | GENERIC_WRITE,
								0,    // exclusive access
								NULL, // no security attributes
								OPEN_EXISTING,
								0,
								NULL
								);

	if (m_pSerImp->m_hComm == INVALID_HANDLE_VALUE)
	{
		return CSerialComm_ERROR_BAD_NAME;
	}

	if(!SetupComm(m_pSerImp->m_hComm, BUFFERSIZE, BUFFERSIZE))
	{
		return CSerialComm_ERROR_PLATFORM_00;
	}

	dcb.DCBlength = sizeof(DCB);
	if(!GetCommState(m_pSerImp->m_hComm, &dcb))
	{
		close();
		return CSerialComm_ERROR_PLATFORM_01;
	}

	// Set baud and such
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = baudFlag;	// set the baud rate
	dcb.fParity = 0;
	dcb.fBinary = 1;			// Binary
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow  = 0;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity =0;
	dcb.fTXContinueOnXoff = 1;
	dcb.fOutX = 0;
	dcb.fInX = 0;
	dcb.fErrorChar = 0;
	dcb.fNull = 0;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;
	dcb.ByteSize = 8;				// data size, xmit, and rcv
	dcb.Parity = parityFlag;		// no parity bit
	dcb.StopBits = stopBitsFlag;	// one stop bit
	dcb.XonChar = 0x11;				// ^Q (CS1)
	dcb.XoffChar = 0x13;			// ^S (DC3)

	if(!SetCommState(m_pSerImp->m_hComm, &dcb))
	{
		close();
		return CSerialComm_ERROR_PLATFORM_02;
	}

	// Set the event mask
	if(!SetCommMask(m_pSerImp->m_hComm, EV_RXCHAR | EV_CTS | EV_DSR))
	{
		close();
		return CSerialComm_ERROR_PLATFORM_03;
	}

	// Set timeout structure
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if(!SetCommTimeouts(m_pSerImp->m_hComm, &timeouts))
	{
		close();
		return CSerialComm_ERROR_PLATFORM_04;
	}

	// Clear the receive buffer
	PurgeComm(m_pSerImp->m_hComm,	PURGE_TXABORT | PURGE_TXCLEAR |
				PURGE_RXABORT | PURGE_RXCLEAR);

	return CSerialComm_ERROR_NOERROR;
}

bool CSerialComm::isOpen()
{
	if(m_pSerImp->m_hComm != INVALID_HANDLE_VALUE)
		return true;

	return false;
}

void CSerialComm::close()
{
	m_pSerImp->close();
}

// Polled IO
void CSerialComm::tick()
{
	m_pSerImp->readFromPort();
	m_pSerImp->writeToPort();
}

///////////////////////////////////////////////////////////////////////
// Read and write really just queue and un-queue the data
unsigned int CSerialComm::read(unsigned char *_pBuf, unsigned int _iBufSize, bool _bConsume)
{
unsigned int amountRead = m_pSerImp->m_ReceivedData.read(_pBuf, _iBufSize);
	if((amountRead > 0) && (_bConsume))
		m_pSerImp->m_ReceivedData.consume(amountRead);
	return amountRead;
}

unsigned int CSerialComm::write(const unsigned char *_pBuf, unsigned int _iBufSize)
{
	return m_pSerImp->m_TransmitQueue.write(_pBuf, _iBufSize);
}

int CSerialComm::getError()
{
	return m_pSerImp->m_iLastError;
}

int CSerialComm::bytesInReceiveBuffer()
{
	return m_pSerImp->m_ReceivedData.bytesAvailable();
}

int CSerialComm::bytesInTransmitBuffer()
{
	return m_pSerImp->m_TransmitQueue.bytesAvailable();
}

// String oriented management
int CSerialComm::gets(char *_pBuf, int _iBufLen)
{
    return m_pSerImp->m_ReceivedData.gets(_pBuf, _iBufLen);
}

bool CSerialComm::puts(const char * _pBuf)
{
    return m_pSerImp->m_TransmitQueue.puts(_pBuf);
}
