#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/log.hpp>

#include <cstdio>
#include <iostream>

boost::asio::io_context io;
boost::asio::steady_timer rotateEventLogsTimer(io);

// Time interval for sending system call.
const int rotateInterval = 60;
bool popenHasFailed = false;

using namespace phosphor::logging;

// Call logrotate per minute.
void timerForRotate()
{
    rotateEventLogsTimer.expires_from_now(
        boost::asio::chrono::seconds(rotateInterval));
    rotateEventLogsTimer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            log<level::ERR>("Timer got error.",
                            entry("ERROR=%d", ec.value()));
            return;
        }
        else
        {
            auto fp = popen(
                "/usr/sbin/logrotate /etc/logrotate.d/logrotate.rsyslog", "w");

            if (!fp)
            {
                if (!popenHasFailed)
                {
                    log<level::ERR>("Can't open logrotate.",
                                    entry("ERROR=%s", strerror(errno)));
                    popenHasFailed = true;
                }
            }
            else
            {
                pclose(fp);
                popenHasFailed = false;
            }
            // Restart timer for next time rotation.
            timerForRotate();
        }
    });
}

int main()
{
    timerForRotate();
    io.run();

    return 0;
}
