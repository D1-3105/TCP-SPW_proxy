//
// Created by konin on 31.05.24.
//
#include "logging.h"

void logging::init_logging(boost::log::trivial::severity_level level) {
    boost::log::add_console_log(std::cout);
    boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= level
    );
}
