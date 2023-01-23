#include "Logger.h"

std::ostream* Logger::m_OutputStream = nullptr;
std::mutex Logger::m_OutputMutex;
