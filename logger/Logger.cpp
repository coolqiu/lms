#include <boost/foreach.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include </usr/include/boost/log/keywords/filter.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/date_time/posix_time/ptime.hpp>


#include "Logger.hpp"

Logger&
Logger::instance()
{
	static Logger instance;
	return instance;
}

Logger::Logger()
{
	// Initialiaz loggers
	static const struct LoggerDef {
		Module	module;
		std::string	name;
	} loggers[] = {
		{MOD_MAIN,	"MAIN"},
		{MOD_UI,	"UI"},
		{MOD_REMOTE,	"REMOTE"}
	};

	BOOST_FOREACH(const struct LoggerDef& logger, loggers) {
		std::cout << "Adding attribute" << std::endl;
	    _loggers[logger.module].add_attribute("Module", boost::log::attributes::constant< Module >(logger.module));
	}
}

void
Logger::init(const Config& config)
{
	boost::log::add_common_attributes();

	boost::log::register_simple_formatter_factory< Severity, char >("Severity");

	if (config.enableFileLogging)
	{
		boost::log::add_file_log
			(
			 boost::log::keywords::file_name = config.logPath + std::string(".%N"),
			 boost::log::keywords::rotation_size = 10 * 1024 * 1024,
			 boost::log::keywords::open_mode = std::ios_base::app,
			 boost::log::keywords::format = (
				 boost::log::expressions::stream
				 << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "[%Y-%m-%d %H:%M:%S]")
				 << " [" << boost::log::expressions::attr< Module >("Module") << "]"
				 << " [" << boost::log::expressions::attr< Severity >("Severity") << "]"
				 << " " << boost::log::expressions::smessage
				 )
			);
	}

	if (config.enableConsoleLogging)
	{
		boost::log::add_console_log(std::cout,
				boost::log::keywords::format = (
					boost::log::expressions::stream
					<< boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "[%Y-%m-%d %H:%M:%S]")
					<< " [" << boost::log::expressions::attr< Module >("Module") << "]"
					<< " [" << boost::log::expressions::attr< Severity >("Severity") << "]"
					<< " " << boost::log::expressions::smessage
					)
				);
	}

	boost::log::core::get()->set_filter
		(
		 boost::log::expressions::attr<Severity>("Severity") <= config.minSeverity
		);

}
