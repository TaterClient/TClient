#ifndef GAME_CLIENT_COMPONENTS_TATER_H
#define GAME_CLIENT_COMPONENTS_TATER_H
#include <game/client/component.h>

#include <fstream>
#include <string>
#include <iostream>

class CTater : public CComponent
{
	static void ConRandomTee(IConsole::IResult *pResult, void *pUserData);
	static void ConchainRandomColor(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void RandomBodyColor();
	static void RandomFeetColor();
	static void RandomSkin(void *pUserData);
	static void RandomFlag(void *pUserData);

public:
	CTater();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;

	void OnConsoleInit() override;

	void CheckUpdates();
};


class Logger
{
public:
	/**
	 * Constructs a logger that writes to the given file in append mode.
	 */
	explicit Logger(const std::string &filename)
	{
		m_logFile.open(filename, std::ios::out | std::ios::app);
		if(!m_logFile.is_open())
		{
			std::cerr << "Failed to open log file: " << filename << std::endl;
		}
	}

	/**
	 * Closes the file stream in the destructor.
	 */
	~Logger()
	{
		if(m_logFile.is_open())
		{
			m_logFile.close();
		}
	}

	/**
	 * The callback function to register with `vpkc_set_logger`.
	 *
	 * @param p_user_data  Pointer to the Logger instance (passed by the user).
	 * @param psz_level    Log level string (e.g. "INFO", "WARN", "ERROR").
	 * @param psz_message  The actual log message.
	 */
	static void LogCallback(void *p_user_data, const char *psz_level, const char *psz_message)
	{
		if(!p_user_data)
			return;

		Logger *logger = static_cast<Logger *>(p_user_data);
		logger->logMessage(psz_level, psz_message);
	}

private:
	/**
	 * Writes the log message to the file. You can extend this to add timestamps or other formatting.
	 */
	void logMessage(const char *psz_level, const char *psz_message)
	{
		if(m_logFile.is_open())
		{
			m_logFile << "[" << psz_level << "] " << psz_message << std::endl;
		}
	}

	std::ofstream m_logFile;
};

#endif
