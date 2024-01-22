#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/object_server.hpp>

using namespace phosphor::logging;

// Indicate bit 3 of BMC boot flag valid bit clearing.
static constexpr uint8_t dontClearValidTimeout = 0x8;

static boost::asio::io_context io;
static boost::asio::steady_timer validTimer(io);

uint8_t bootFlagValidBitClearing = 0;
bool bootFlagsValid = false;

static void startValidTimer()
{
    validTimer.expires_after(std::chrono::seconds(180));
    validTimer.async_wait([&](const boost::system::error_code& ec) {
        if (ec)
        {
            if (ec == boost::asio::error::operation_aborted)
            {
                // We will reach here when boot flags valid is set to false.
                return;
            }

            // For the other errors.
            log<level::ERR>("Boot flags valid timer error",
                            entry("ERROR=%d", ec.value()));
            return;
        }

        if (!(bootFlagValidBitClearing & dontClearValidTimeout))
        {
            bootFlagsValid = false;
        }
    });
}

int main()
{
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name("xyz.openbmc_project.BootOption");

    auto server = sdbusplus::asio::object_server(conn);
    auto bootFlagsValidIntf =
        server.add_interface("/xyz/openbmc_project/control/boot",
                             "xyz.openbmc_project.Control.Valid");

    bootFlagsValidIntf->register_property(
        "BootFlagValidBitClearing", bootFlagValidBitClearing,
        [](const uint8_t& newValue, uint8_t&) {
            bootFlagValidBitClearing = newValue;
            return 0;
        },
        [](const uint8_t&) { return bootFlagValidBitClearing; });

    bootFlagsValidIntf->register_property(
        "BootFlagsValid", bootFlagsValid,
        [](const bool& newValue, bool&) {
            bootFlagsValid = newValue;
            if (bootFlagsValid)
            {
                startValidTimer();
            }
            else
            {
                validTimer.cancel();
            }
            return 0;
        },
        [](const bool&) { return bootFlagsValid; });

    bootFlagsValidIntf->register_method("RestartValidTimer", []() {
        if (bootFlagsValid)
        {
            startValidTimer();
        }
    });

    bootFlagsValidIntf->initialize();

    io.run();
}
