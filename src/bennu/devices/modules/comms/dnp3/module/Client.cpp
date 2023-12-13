#include "Client.hpp"

#include <functional>
#include <thread>
#include <unistd.h>

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
    std::cout << "MEG DEBUG: this theoretically where client should loop" << std::endl;	
    mUpdateThread.reset(new std::thread(std::bind(&Client::update, this)));
}

void Client::update()
{
    int i = 0;
    while (1)
    {
	if (i % 100000 == 0)
	{
	    sleep(5);
	    std::set<std::string> tags = getTags();
	    for (auto iter = tags.begin(); iter != tags.end(); ++iter)
	    {
	        //call read tag next
		std::shared_ptr<ClientConnection> connection = mTagsToConnection[*iter];
		comms::RegisterDescriptor rd;
		auto status = connection->readRegisterByTag(*iter, rd);
	
		if (rd.mRegisterType == 1 || rd.mRegisterType == 2) //Status
		{
		    mDataManager->setDataByPoint<bool>(*iter, rd.mStatus);
		}
		else if (rd.mRegisterType == 3 || rd.mRegisterType == 4) // value
		{
	    	    mDataManager->setDataByPoint<double>(*iter, rd.mFloatValue);
		}
		else if (rd.mRegisterType == 5 || rd.mRegisterType == 6) //int
		{
		    mDataManager->setDataByPoint<double>(*iter, rd.mIntValue);
		}	
	    } 
	    i=0;	
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
}

void Client::addTagConnection(const std::string& tag, std::shared_ptr<ClientConnection> connection, const bool sbo = false)
{
    mTagsToConnection[tag] = connection;
    mTagsForSBO[tag] = sbo;
}

void Client::addTagDataManager(const std::string& tag, std::shared_ptr<ClientConnection> connection)
{
    std::cout << "MEG DEBUG: dnp3 Client.cpp -- addTagConnection" << std::endl;
    mTagsToConnection[tag] = connection;
    comms::RegisterDescriptor rd;
    connection->readRegisterByTag(tag, rd);
    std::cout << "MEG DEBUG: dnp3 Client.cpp -- addTagConnection -- register type " << rd.mRegisterType << std::endl;
    if (rd.mRegisterType == 1 || rd.mRegisterType == 2)
    {
    	mDataManager->addExternalData<bool>(tag, tag);
	std::cout << "add binary  " << tag << std::endl;
    }
    else if (rd.mRegisterType == 3 || rd.mRegisterType == 4 || rd.mRegisterType == 5 || rd.mRegisterType == 6)
    {
    	mDataManager->addExternalData<double>(tag, tag);
	std::cout << "add analog " << tag << std::endl;
    }
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
        return iter->second->readRegisterByTag(tag, rd);
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
