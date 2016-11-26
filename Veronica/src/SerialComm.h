////////////////////////////////////////////////////////////
// "Portable" Serial (RS-232) IO handler
////////////////////////////////////////////////////////////
#ifndef SERIALCOMM_H
#define SERIALCOMM_H

////////////////////////////////////////////////////////////
// This is the interface file, the implementation will vary
// from one platform to another
////////////////////////////////////////////////////////////

// Parity enum
typedef enum
{
	CSerialComm_Parity_None = 0,
	CSerialComm_Parity_Even,
	CSerialComm_Parity_Odd
} CSerialComm_ParityT;

class CSerialCommImp;
class CSerialComm : public ICommunicationInterface
{
	protected:
		CSerialCommImp *m_pSerImp;

	public:
		CSerialComm();
		virtual ~CSerialComm();

		int open(const char * _pPort, int _iBaud, int _iBits, CSerialComm_ParityT _eParity, int _iStopBits);

		bool isOpen();
		void close();

		// Polled IO
		void tick();

		virtual unsigned int read(unsigned char *_pBuf, unsigned int _iBufSize, bool _bConsume = true);
		virtual unsigned int write(const unsigned char *_pBuf, unsigned int _iBufSize);
		virtual int getError();

		virtual int bytesInReceiveBuffer();
		virtual int bytesInTransmitBuffer();

		virtual int gets(char *_pBuf, int _iBufLen);
		virtual bool puts(const char * _pBuf);
};

///////////////////////////////////////////////////////////
// Special port name for no port selected (Actually == "NONE"
#define CSerialComm_Null_Port	"NONE"

///////////////////////////////////////////////////////////
// Errors that Open might return (there may be others)
// Some are quite standard
#define CSerialComm_ERROR_BASE				(0)
#define CSerialComm_ERROR_NOERROR			(CSerialComm_ERROR_BASE + 0)
#define CSerialComm_ERROR_BAD_NAME			(CSerialComm_ERROR_BASE + 1)
#define CSerialComm_ERROR_INVALID_BAUD		(CSerialComm_ERROR_BASE + 2)
#define CSerialComm_ERROR_INVALID_PARITY	(CSerialComm_ERROR_BASE + 3)
#define CSerialComm_ERROR_INVALID_BITS		(CSerialComm_ERROR_BASE + 4)
#define CSerialComm_ERROR_INVALID_STOPBITS	(CSerialComm_ERROR_BASE + 5)

// Implementation Specific - see the source
#define CSerialComm_ERROR_PLATFORM_00		(CSerialComm_ERROR_BASE + 100)
#define CSerialComm_ERROR_PLATFORM_01		(CSerialComm_ERROR_BASE + 101)
#define CSerialComm_ERROR_PLATFORM_02		(CSerialComm_ERROR_BASE + 102)
#define CSerialComm_ERROR_PLATFORM_03		(CSerialComm_ERROR_BASE + 103)
#define CSerialComm_ERROR_PLATFORM_04		(CSerialComm_ERROR_BASE + 104)
#define CSerialComm_ERROR_PLATFORM_05		(CSerialComm_ERROR_BASE + 105)
#define CSerialComm_ERROR_PLATFORM_06		(CSerialComm_ERROR_BASE + 106)
#define CSerialComm_ERROR_PLATFORM_07		(CSerialComm_ERROR_BASE + 107)
#define CSerialComm_ERROR_PLATFORM_08		(CSerialComm_ERROR_BASE + 108)
#define CSerialComm_ERROR_PLATFORM_09		(CSerialComm_ERROR_BASE + 109)
#define CSerialComm_ERROR_PLATFORM_10		(CSerialComm_ERROR_BASE + 110)

#endif
