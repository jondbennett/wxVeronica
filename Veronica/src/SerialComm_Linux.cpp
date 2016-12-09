////////////////////////////////////////////////////////////
// SerialIO - Platform specific implementation file
// This one is "NULL" to provide a starting point for
// other implementations, and to provide a way to
// test applications.
////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>

#include "../../../GaryCooper/ICommInterface.h"
#include "../../../GaryCooper/SlidingBuf.h"
#include "SerialComm.h"

// Read/write buffer size (right here)
#define BUFSIZE	(1024)

#define BAD_PORT	(-1)
class CSerialCommImp
{
	public:

	// Serial port
	int fd_serialport;
	int m_iLastError;

	// Data buffers
	CSlidingBuffer m_ReceivedData;
	CSlidingBuffer m_TransmitQueue;

	CSerialCommImp()
	{
		fd_serialport = BAD_PORT;
		m_iLastError = CSerialComm_ERROR_NOERROR;

		m_ReceivedData.setCanGrow(true);
		m_TransmitQueue.setCanGrow(true);
	}

	virtual ~CSerialCommImp()
	{
		close();
	}

	void close()
	{
		if(fd_serialport != BAD_PORT)
			::close(fd_serialport);
		fd_serialport = BAD_PORT;
	}

	void readFromPort()
	{
	int amountRead;
	unsigned int dataLen;
	unsigned char buffer[BUFSIZE];

		if(fd_serialport == BAD_PORT)
			return;

		amountRead = read(fd_serialport, buffer, sizeof(buffer));
		if((amountRead > 0) && (buffer[0] == 0))
			amountRead = 0;

		// Check for error
		if(amountRead < 0)
			m_iLastError = CSerialComm_ERROR_PLATFORM_03;

		// We have data, so put it in the buffer
		if(amountRead > 0)
		{
			dataLen = (unsigned int) amountRead;
			m_ReceivedData.write(buffer, dataLen);
		}
	}

	void writeToPort()
	{
		int bytesWritten = 0;
		unsigned int dataLen;
		unsigned char buffer[BUFSIZE];

		if(fd_serialport == BAD_PORT)
			return;

		// Write what we can
		do
		{
			// Nothing written yet
			bytesWritten = 0;

			// If there is nothing to send then we are done
			if(m_TransmitQueue.bytesAvailable() == 0)
				break;
			dataLen = m_TransmitQueue.read(buffer, sizeof(buffer));

			// Write it to the device
			bytesWritten = write(fd_serialport, buffer, dataLen);

			// Check for error
			if(bytesWritten < 0)
				m_iLastError = CSerialComm_ERROR_PLATFORM_04;

			// Remove whatever we have written from the transmit queue
			if(bytesWritten > 0)
				m_TransmitQueue.consume((unsigned int) bytesWritten);
		}
		while(bytesWritten > 0);
	}
};


////////////////////////////////////////////////////////////
// More Description and notes
////////////////////////////////////////////////////////////
CSerialComm::CSerialComm()
{
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
	struct termios options;
	int baudFlag;
	int bitsFlag;

	// Allow "NONE" to work
	if(strcmp(_pPort,CSerialComm_Null_Port) == 0)
	{
		close();
		return CSerialComm_ERROR_NOERROR;
	}

	// Open the device or return an error code
	m_pSerImp->fd_serialport = ::open(_pPort, O_RDWR | O_NOCTTY | O_NDELAY);
	if(m_pSerImp->fd_serialport == -1)
	{
		switch(errno)
		{
		case ENAMETOOLONG:
			m_pSerImp->close();
			return CSerialComm_ERROR_BAD_NAME;
		case EACCES:
			m_pSerImp->close();
			return CSerialComm_ERROR_PLATFORM_00;
		case EPERM:
			m_pSerImp->close();
			return CSerialComm_ERROR_PLATFORM_01;
		default:
			m_pSerImp->close();
			return CSerialComm_ERROR_PLATFORM_02;

		}

		m_pSerImp->fd_serialport = BAD_PORT;
	}

	// Figure the baud rates
	switch(_iBaud)
	{
	case 110:		baudFlag = B110;	break;
	case 300:		baudFlag = B300;	break;
	case 600:		baudFlag = B600;	break;
	case 1200:		baudFlag = B1200;	break;
	case 2400:		baudFlag = B2400;	break;
	case 4800:		baudFlag = B4800;	break;
	case 9600:		baudFlag = B9600;	break;
	case 19200:		baudFlag = B19200; 	break;
	case 38400:		baudFlag = B38400;	break;
	case 57600:		baudFlag = B57600; 	break;
	case 115200:	baudFlag = B115200; break;
	default:
		m_pSerImp->close();
		return CSerialComm_ERROR_INVALID_BAUD;
	}

	// Check character size
	switch(_iBits)
	{
	case 7: bitsFlag = CS7; break;
	case 8: bitsFlag = CS8; break;
	default:
		m_pSerImp->close();
		return CSerialComm_ERROR_INVALID_BITS;
	}

	// Check the stop bits
	if(_iStopBits != 1 && _iStopBits != 2)
	{
		m_pSerImp->close();
		return CSerialComm_ERROR_INVALID_STOPBITS;
	}

	// Get the options structure
	tcgetattr(m_pSerImp->fd_serialport, &options);

	// Set baud rate
	cfsetispeed(&options, baudFlag);
	cfsetospeed(&options, baudFlag);

	// Specify input processing
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                           | INLCR | IGNCR | ICRNL | IXON);

	// Output CR-LF
	options.c_oflag &= ~OPOST;
	options.c_oflag |= ONLCR;

	// Set local modes
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

	// Set control modes
	options.c_cflag |= CREAD;

	switch(_eParity)
	{
	case 0:	// None
		options.c_cflag &= ~PARENB;
		break;
	case 1:	// Even
		options.c_cflag |= PARENB;
		break;
	case 2:	// Odd
		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		break;
	default:
		m_pSerImp->close();
		return CSerialComm_ERROR_INVALID_PARITY;
	}

	if(_iStopBits == 1)
		options.c_cflag &= ~CSTOPB;
	else
		options.c_cflag |= ~CSTOPB;

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= bitsFlag;

	// Now set everything
	tcsetattr(m_pSerImp->fd_serialport, TCSANOW, &options);

	// Set for non-blocking
	fcntl(m_pSerImp->fd_serialport, F_SETFL, FNDELAY);
	tcflush(m_pSerImp->fd_serialport,TCIOFLUSH);

	return CSerialComm_ERROR_NOERROR;
}

bool CSerialComm::isOpen()
{
	return (m_pSerImp->fd_serialport == BAD_PORT)?false:true;
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
unsigned int amountRead = m_pSerImp->m_ReceivedData.read(_pBuf, _iBufSize, _bConsume);
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
