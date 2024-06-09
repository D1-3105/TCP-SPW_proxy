//
// Created by oleg on 27.05.24.
//

#ifndef SPW_ETHERNET_PRODUCER_LOGGING_H
#define SPW_ETHERNET_PRODUCER_LOGGING_H

#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging
{
    void init_logging(boost::log::trivial::severity_level level = boost::log::trivial::debug);
}

#endif //SPW_ETHERNET_PRODUCER_LOGGING_H
