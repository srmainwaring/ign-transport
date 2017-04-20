/*
 * Copyright (C) 2014 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <ignition/msgs.hh>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "ignition/transport/config.hh"
#include "ignition/transport/ign.hh"
#include "ignition/transport/Helpers.hh"
#include "ignition/transport/Node.hh"

#ifdef _MSC_VER
# pragma warning(disable: 4503)
#endif

using namespace ignition;
using namespace transport;

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicList()
{
  Node node;

  std::vector<std::string> topics;
  node.TopicList(topics);

  for (auto const &topic : topics)
    std::cout << topic << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicInfo(const char *_topic)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }

  Node node;

  // Get the publishers on the requested topic
  std::vector<MessagePublisher> publishers;
  node.TopicInfo(_topic, publishers);

  if (!publishers.empty())
  {
    std::cout << "Publishers [Address, Message Type]:\n";

    /// List the publishers
    for (std::vector<MessagePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
        << (*iter).MsgTypeName() << std::endl;
    }
  }
  else
  {
    std::cout << "No publishers on topic [" << _topic << "]\n";
  }

  // TODO: Add subscribers lists
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdServiceList()
{
  Node node;

  std::vector<std::string> services;
  node.ServiceList(services);

  for (auto const &service : services)
    std::cout << service << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdServiceInfo(const char *_service)
{
  if (!_service || std::string(_service).empty())
  {
    std::cerr << "Invalid service. Service must not be empty.\n";
    return;
  }

  Node node;

  // Get the publishers on the requested topic
  std::vector<ServicePublisher> publishers;
  node.ServiceInfo(_service, publishers);

  if (!publishers.empty())
  {
    std::cout << "Service providers [Address, Request Message Type, "
              << "Response Message Type]:\n";

    /// List the publishers
    for (std::vector<ServicePublisher>::iterator iter = publishers.begin();
        iter != publishers.end(); ++iter)
    {
      std::cout << "  " << (*iter).Addr() << ", "
        << (*iter).ReqTypeName() << ", " << (*iter).RepTypeName()
        << std::endl;
    }
  }
  else
  {
    std::cout << "No service providers on service [" << _service << "]\n";
  }
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicPub(const char *_topic,
  const char *_msgType, const char *_msgData)
{
  if (!_topic)
  {
    std::cerr << "Topic name is null\n";
    return;
  }

  if (!_msgType)
  {
    std::cerr << "Message type is null\n";
    return;
  }

  if (!_msgData)
  {
    std::cerr << "Message data is null\n";
    return;
  }

  // Create the message, and populate the field with _msgData
  auto msg = ignition::msgs::Factory::New(_msgType, _msgData);
  if (msg)
  {
    // Create the node and advertise the topic
    ignition::transport::Node node;
    auto pub = node.Advertise(_topic, msg->GetTypeName());

    // Publish the message
    if (pub)
    {
      // \todo Change this sleep to a WaitForSubscribers() call.
      // See issue #xxx
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      pub.Publish(*msg);
    }
    else
    {
      std::cerr << "Unable to publish on topic[" << _topic << "] "
        << "with message type[" << _msgType << "].\n";
    }
  }
  else
  {
    std::cerr << "Unable to create message of type[" << _msgType << "] "
      << "with data[" << _msgData << "].\n";
  }
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdServiceReq(const char *_service,
  const char *_reqType, const char *_repType, const int _timeout,
  const char *_reqData)
{
  if (!_service)
  {
    std::cerr << "Service name is null\n";
    return;
  }

  if (!_reqType)
  {
    std::cerr << "Request type is null\n";
    return;
  }

  if (!_repType)
  {
    std::cerr << "Response type is null\n";
    return;
  }

  if (!_reqData)
  {
    std::cerr << "Request data is null\n";
    return;
  }

  // Create the request, and populate the field with _reqData
  auto req = ignition::msgs::Factory::New(_reqType, _reqData);
  if (!req)
  {
    std::cerr << "Unable to create request of type[" << _reqType << "] "
              << "with data[" << _reqData << "].\n";
    return;
  }

  // Create the response.
  auto rep = ignition::msgs::Factory::New(_repType);
  if (!rep)
  {
    std::cerr << "Unable to create response of type[" << _repType << "].\n";
    return;
  }

  // Create the node.
  ignition::transport::Node node;
  bool result;

  // Request the service.
  bool executed = node.Request(_service, *req, _timeout, *rep, result);
  if (executed)
  {
    if (result)
      std::cout << rep->DebugString() << std::endl;
    else
      std::cout << "Service call failed" << std::endl;
  }
  else
    std::cerr << "Service call timed out" << std::endl;
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE void cmdTopicEcho(const char *_topic,
  const double _duration)
{
  if (!_topic || std::string(_topic).empty())
  {
    std::cerr << "Invalid topic. Topic must not be empty.\n";
    return;
  }

  std::function<void(const ProtoMsg&)> cb = [](const ProtoMsg &_msg)
  {
    std::cout << _msg.DebugString() << std::endl;
  };

  Node node;
  if (!node.Subscribe(_topic, cb))
    return;

  if (_duration >= 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(
      static_cast<int64_t>(_duration * 1000)));
    return;
  }

  // Wait forever.
  ignition::transport::waitForShutdown();
}

//////////////////////////////////////////////////
extern "C" IGNITION_TRANSPORT_VISIBLE char *ignitionVersion()
{
  int majorVersion = IGNITION_TRANSPORT_MAJOR_VERSION;
  int minorVersion = IGNITION_TRANSPORT_MINOR_VERSION;
  int patchVersion = IGNITION_TRANSPORT_PATCH_VERSION;

  return ign_strdup((std::to_string(majorVersion) + "." +
                     std::to_string(minorVersion) + "." +
                     std::to_string(patchVersion)).c_str());
}
