#pragma once
#include <app.hpp>
#include <tinyxml2.h>

#include <cstdio>
#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <sdbusplus/message/types.hpp>
#include <sys/stat.h>


namespace crow
{
namespace mct_oem_rest
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

const char *notFoundMsg = "404 Not Found";
const char *badReqMsg = "400 Bad Request";
const char *methodNotAllowedMsg = "405 Method Not Allowed";
const char *forbiddenMsg = "403 Forbidden";
const char *methodFailedMsg = "500 Method Call Failed";
const char *methodOutputFailedMsg = "500 Method Output Error";
const char *notFoundDesc =
    "org.freedesktop.DBus.Error.FileNotFound: path or object not found";
const char *propNotFoundDesc = "The specified property cannot be found";
const char *noJsonDesc = "No JSON object could be decoded";
const char *methodNotFoundDesc = "The specified method cannot be found";
const char *methodNotAllowedDesc = "Method not allowed";
const char *forbiddenPropDesc = "The specified property cannot be created";
const char *forbiddenResDesc = "The specified resource cannot be created";

std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;

inline void solLogHandler(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::filesystem::path loc("/var/lib/obmc-console/obmc-console.log");
    std::ifstream readFile(loc);

    if (!std::filesystem::exists(loc) || std::filesystem::is_empty(loc) ||
        !std::filesystem::is_regular_file(loc))
    {
        BMCWEB_LOG_ERROR << loc << "SOL data Not found";
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.end();
        return;
    }

    if (readFile.good())
    {
        asyncResp->res.addHeader("Content-Type", "application/octet-stream");
        asyncResp->res.body() = {std::istreambuf_iterator<char>(readFile),
                      std::istreambuf_iterator<char>()};
        asyncResp->res.end();
        return;
    }
    else
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.end();
        return;
    }
}

inline void crashdumpsHandler(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::vector<std::string> crashdumpslist;
    nlohmann::json crashdumpslistData;
    std::string path = "/var/lib/crashdump/output";
    crow::Response& res = asyncResp->res;


    if(std::filesystem::exists(std::filesystem::path(path))){
        for (const auto & entry : std::filesystem::directory_iterator(path))
        {
            if(entry.path().filename().string().find("json",0) == std::string::npos)
            {
                continue;
            }
            std::ifstream ifs(entry.path().string());
            nlohmann::json crashdumpsData = nlohmann::json::parse(ifs);
            crashdumpslist.push_back(entry.path().filename().string());
            crashdumpslistData.push_back({entry.path().filename().string(),crashdumpsData});
        }
    }

    res.addHeader("Content-Type", "application/json");

    res.jsonValue = {{"status", "ok"},
                     {"message", "200 OK"},
                     {"list", std::move(crashdumpslist)},
                     {"data", std::move(crashdumpslistData)}
                     };
    res.end();
}

inline void uploadBiosHandler(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                               const std::string& filename)
{
    crow::Response& res = asyncResp->res;
    std::cout << "uploadBiosHandlers\n";
    // Only allow one FW update at a time
    if (fwUpdateMatcher != nullptr)
    {
        res.addHeader("Retry-After", "30");
        res.result(boost::beast::http::status::service_unavailable);
        res.end();
        return;
    }
    // Make this const static so it survives outside this method
    static boost::asio::deadline_timer timeout(*req.ioService,
                                               boost::posix_time::seconds(5));

    timeout.expires_from_now(boost::posix_time::seconds(15));

    auto timeoutHandler = [&res](const boost::system::error_code& ec) {
        fwUpdateMatcher = nullptr;
        if (ec == boost::asio::error::operation_aborted)
        {
            // expected, we were canceled before the timer completed.
            return;
        }
        BMCWEB_LOG_ERROR << "Timed out waiting for Version interface";

        if (ec)
        {
            BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
            return;
        }
          std::cout << "timeoutHandler\n";
        res.jsonValue = {{"data", 
                           {{"description",
                                "done"}}},
                        {"message", "200 OK"},
                        {"status", "ok"}};
        BMCWEB_LOG_DEBUG << "ending response";
        res.end();
        fwUpdateMatcher = nullptr;
 
    };

    std::function<void(sdbusplus::message::message&)> callback =
        [&res](sdbusplus::message::message& m) {
            BMCWEB_LOG_DEBUG << "Match fired";

            sdbusplus::message::object_path path;
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::variant<std::string>>>>>
                interfaces;
            m.read(path, interfaces);

            if (std::find_if(interfaces.begin(), interfaces.end(),
                             [](const auto& i) {
                                 return i.first ==
                                        "xyz.openbmc_project.Software.Version";
                             }) != interfaces.end())
            {
                boost::system::error_code ec;
                timeout.cancel(ec);
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "error canceling timer " << ec;
                }

                std::size_t index = path.str.rfind('/');
                if (index != std::string::npos)
                {
                    path.str.erase(0, index + 1);
                }
                res.jsonValue = {{"data", std::move(path.str)},
                                 {"message", "200 OK"},
                                 {"status", "ok"}};
                BMCWEB_LOG_DEBUG << "ending response";
                res.end();
                fwUpdateMatcher = nullptr;
            }
        };
    /*
    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);
        */
/*  note, this shared library is not default installell  
    boost::filesystem::path dir("/tmp/bios/");

    if(!(boost::filesystem::exists(dir))){
        std::cout<<"Doesn't Exists"<<std::endl;

        if (boost::filesystem::create_directory(dir))
            std::cout << "....Successfully Created !" << std::endl;
    }
*/   

    
    struct stat sb;
    if (stat("/tmp/bios", &sb) != 0) {
        mkdir("/tmp/bios", 0755);
    }

    std::string filepath(
        "/tmp/bios/bios.tgz");
    BMCWEB_LOG_DEBUG << "Writing file to " << filepath;
    std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                    std::ofstream::trunc);
    out << req.body;
    out.close();
    timeout.async_wait(timeoutHandler);
}

inline void requestRoutes(App& app)
{
    
    BMCWEB_ROUTE(app, "/hostconsole.log")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request &req, 
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            solLogHandler(req, asyncResp);
        });

    BMCWEB_ROUTE(app, "/crashdumps")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request &req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            crashdumpsHandler(req, asyncResp);
        });

    BMCWEB_ROUTE(app, "/upload/bios")
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req, 
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                uploadBiosHandler(req, asyncResp, "");
            });
    
}
} // namespace mct_oem_rest
} // namespace crow
