#include "Client.hpp"

#include <functional>
#include <thread>

#include "opendnp3/ConsoleLogger.h"

namespace bennu {
namespace comms {
namespace dnp3 {

using bennu::comms::RegDescComp;
using bennu::comms::RegisterDescriptor;
using boost::property_tree::ptree;
using boost::property_tree::ptree_bad_path;
using boost::property_tree::ptree_error;

Client::Client(std::shared_ptr<field_device::DataManager> dm) :
    comms::CommsClient(),
    bennu::utility::DirectLoggable("dnp3-client")
{
    // Initialize master stack
    mManager.reset(new opendnp3::DNP3Manager(std::thread::hardware_concurrency(), opendnp3::ConsoleLogger::Create()));
    setDataManager(dm);
}


Client::~Client()
{
    mTagsToConnection.clear();
}

void Client::start()
{
    update(); 
}

void Client::update()
{
    int i = 1;
    while (1)
    {
	if (i % 10 == 0)
	{
	    std::cout << "we got the client looping!" << std::endl;
	    for (auto iter = getTags().begin(); iter != getTags().end(); ++iter)
	    {
		std::cout << "tag " << iter << std::endl;
	        //call read tag next	
	    } 
	    i=1;	
	}
	else
	{
	    i++;
	}
    }
}


void Client::addTagConnection(const std::string& tag, std::shared_ptr<ClientConnection> connection)
{
    mTagsToConnection[tag] = connection;
    mDataManager->addExternalData<double>(tag, tag);
}

void Client::addTagConnection(const std::string& tag, std::shared_ptr<ClientConnection> connection, const bool sbo = false)
{
    mTagsToConnection[tag] = connection;
    mTagsForSBO[tag] = sbo;
    mDataManager->addExternalData<double>(tag, tag);
}

std::set<std::string> Client::getTags() const
{
    std::set<std::string> tags;
    for (auto iter = mTagsToConnection.begin(); iter != mTagsToConnection.end(); ++iter)
    {
        tags.insert(iter->first);
    }
    return tags;
}


bool Client::isValidTag(const std::string& tag) const
{
    return (mTagsToConnection.find(tag) != mTagsToConnection.end());
}


StatusMessage Client::readTag(const std::string& tag, comms::RegisterDescriptor& rd) const
{
    auto iter = mTagsToConnection.find(tag);
    if (iter != mTagsToConnection.end())
    {
        StatusMessage value = iter->second->readRegisterByTag(tag, rd);
	std::cout << "MEG DEBUG: StatusMessage from readTag" << value.status << " " << value.message << std::endl;
	//mDataManager->setDataByTag(tag, value);
	return value;
    }
std::string msg = "readTag(): Unable to find tag -- " + tag;
    StatusMessage sm;
    sm.status = STATUS_FAIL;
    sm.message = &msg[0];
    return sm;
}


StatusMessage Client::writeBinaryTag(const std::string& tag, bool status)
{
    auto iter = mTagsToConnection.find(tag);
    if (iter != mTagsToConnection.end())
    {
        if (mTagsForSBO[tag])
        {
            return iter->second->selectBinary(tag, status);
        }

        return iter->second->writeBinary(tag, status);
    }
    std::string msg = "writeBinaryTag(): Unable to find tag -- " + tag;
    StatusMessage sm;
    sm.status = STATUS_FAIL;
    sm.message = &msg[0];
    return sm;
}


StatusMessage Client::writeAnalogTag(const std::string& tag, double value)
{
    auto iter = mTagsToConnection.find(tag);
    if (iter != mTagsToConnection.end())
    {
        if (mTagsForSBO[tag])
        {
            return iter->second->selectAnalog(tag, value);
        }

        return iter->second->writeAnalog(tag, value);
    }
    std::string msg = "writeAnalogTag(): Unable to find tag -- " + tag;
    StatusMessage sm;
    sm.status = STATUS_FAIL;
    sm.message = &msg[0];
    return sm;
}

} // namespace dnp3
} // namespace comms
} // namespace bennu
