#include <json/json.h>

#include "CArtemis.h"
#include "CArtemisSocket.h"
#include <iostream>
#include <conio.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#define BROWSER_OPEN "start https://artemisardesignerdev.com/launcher_local.html"
#else
#define BROWSER_OPEN "xdg-open https://artemisardesignerdev.com/launcher_local.html"
#include <unistd.h>
#include <stdlib.h>
#endif

#include <assert.h>
#include "../websocketpp/base64/base64.hpp"

// State
CArtemisState::CArtemisState(
	Json::Value in_stateJSON
)
{
	for (auto key : in_stateJSON.getMemberNames()) {
		state[key] = in_stateJSON[key].asString();
	}
}

std::ostream& operator<<(std::ostream& os, const CArtemisState& state)
{
	os << "State:\n";
	for (auto const& it : state.state)
	{
		os << "\tKey: " << it.first << " Value: " << it.second << "\n";
	}
	return os;
}

// Version
const char* CArtemis::VERSION = "1.12";

// Generic wait
bool CArtemis::m_bGenericWait;
CArtemisState CArtemis::m_genericWaitData;

// Query wait
bool CArtemis::m_bQueryWait;
CArtemisState CArtemis::m_queryWaitData;

// Query queue
std::mutex CArtemis::m_queryQueueMutex;
std::queue<std::function<void(Json::Value)>> CArtemis::m_queryQueue;

// Callback queue
std::mutex CArtemis::m_callbackMapMutex;
std::map<std::string, std::function<void(Json::Value)>> CArtemis::m_callbackMap;

// Event Names
std::map<CArtemis::EEventType, std::string> CArtemis::m_eventNameMap = {
		{ CArtemis::EEventType::Click, "click" },
		{ CArtemis::EEventType::DoubleClick, "dblclick" },
		{ CArtemis::EEventType::MouseEnter, "mouseenter" },
		{ CArtemis::EEventType::MouseLeave, "mouseleave" },
		{ CArtemis::EEventType::KeyPress, "keypress" },
		{ CArtemis::EEventType::KeyDown, "keydown" },
		{ CArtemis::EEventType::KeyUp, "keyup" },
		{ CArtemis::EEventType::Focus, "focus" },
		{ CArtemis::EEventType::Input, "input" }
};

// Version
void CArtemis::PrintVersion() {
	std::clog << VERSION << std::endl;
}

// Ping Handler
void CArtemis::PingHandler(
	Json::Value in_message
)
{
}

// Query Handler
void CArtemis::QueryHandler(
	Json::Value in_message
)
{

	// Get query callback
	auto queryCallback = PopFromQueryQueue();

	// Extract state from JSON packet
	assert(in_message.isMember("state"));
	Json::Value state;
	Json::Reader reader;
	reader.parse(in_message["state"].asString(), state);

	// Call callback with GUI state
	if (queryCallback != nullptr) {
		queryCallback(state);
	}
}

void CArtemis::PushOntoQueryQueue(
	std::function<void(Json::Value)> in_callback
)
{
	std::lock_guard<std::mutex>lock(m_queryQueueMutex);
	m_queryQueue.push(in_callback);
}

std::function<void(Json::Value)> CArtemis::PopFromQueryQueue()
{
	std::lock_guard<std::mutex>lock(m_queryQueueMutex);
	if (!m_queryQueue.empty()) {
		auto callback = m_queryQueue.front();
		m_queryQueue.pop();
		return callback;
	}
	return nullptr;
}

// Callback Handler
void CArtemis::CallbackHandler(
	Json::Value in_message
)
{

	// Get component name
	assert(in_message.isMember("name"));
	std::string componentName = in_message["name"].asString();

	// Extract component state
	assert(in_message.isMember("state"));
	Json::Value state;
	Json::Reader reader;
	reader.parse(in_message["state"].asString(), state);

	// Get callback and call with GUI state
	auto callback = CArtemis::GetCallback(componentName);
	if (callback != nullptr) {
		callback(state);
	}
}

void CArtemis::SetCallback(
	const std::string& in_callbackName,
	std::function<void(Json::Value)> in_callback
)
{
	std::lock_guard<std::mutex>lock(m_callbackMapMutex);
	m_callbackMap[in_callbackName] = in_callback;
}


std::function<void(Json::Value)> CArtemis::GetCallback(const std::string& in_componentName)
{
	std::lock_guard<std::mutex>lock(m_callbackMapMutex);
	if (m_callbackMap.find(in_componentName) != m_callbackMap.end()) {
		auto callback = m_callbackMap[in_componentName];
		return callback;
	}
	else {
		return nullptr;
	}
}

std::string CArtemis::GetLocalFilePath()
{
		// Load exe path
		std::string filePath = "";
#ifdef _WIN32
		char pBuf[256];
		size_t len = sizeof(pBuf);
		int bytes = GetModuleFileNameA(NULL, pBuf, len);
		filePath = std::string(pBuf);
		filePath = filePath.substr(0, filePath.find_last_of("\\"));
#endif
		return filePath;
	
}

std::string CArtemis::LoadImage(const std::string& in_filePath)
{
	std::string localFilePath = CArtemis::GetLocalFilePath();
	std::ifstream input(localFilePath + "\\" + in_filePath, std::ios::binary);
	if (!input.is_open()) {
		std::clog << "[Error] Unable to open file " << localFilePath + "\\" + in_filePath << std::endl;
		return "";
	}

	std::vector<char> bytes(
		(std::istreambuf_iterator<char>(input)),
		(std::istreambuf_iterator<char>()));

	input.close();
	auto base64_png = reinterpret_cast<const unsigned char*>(bytes.data());
	std::string encoded_png = "data:image/jpeg;base64," + websocketpp::base64_encode(base64_png, bytes.size());
	return encoded_png;
}

void CArtemis::Initialize()
{

	// Launch GUI
	std::clog << "Launching Artemis" << std::endl;
	CArtemis::PrintVersion();
	system(BROWSER_OPEN);


	 
	// Load app from file
	std::string localFilePath = CArtemis::GetLocalFilePath();
	std::clog << localFilePath + "\\app.json" << std::endl;
	std::ifstream t(localFilePath + "\\app.json");
	if (!t.is_open()) {
		std::clog << "[Error] Unable to locate app.json" << std::endl;
	}	

	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string appJson = buffer.str();

	// Register message handlers
	CArtemisSocket::RegisterMessageHandler("ping", PingHandler);
	CArtemisSocket::RegisterMessageHandler("query", QueryHandler);
	CArtemisSocket::RegisterMessageHandler("callback", CallbackHandler);

	// Run artemis socket
	CArtemisSocket::RunOnNewThread();

	// Wait for connection to start
	while (!CArtemisSocket::HasConnectionStarted()) {
		Sleep(10);
	}

	// Send app.json
	Json::Value packet;
	packet["type"] = "init";
	packet["state"] = appJson;
	CArtemisSocket::Send(packet);
}

void CArtemis::Navigate(
	const std::string& in_pageName
)
{
	Json::Value packet;
	packet["type"] = "navigate";
	packet["pageName"] = in_pageName;
	CArtemisSocket::Send(packet);
}

void CArtemis::On(
	const EEventType& in_eventType,
	const std::string& in_componentName,
	std::function<void(CArtemisState)> in_callback
)
{
	SetCallback(in_componentName, in_callback);
	Json::Value packet;
	packet["type"] = "callback";
	packet["name"] = in_componentName;
	packet["attribute"] = m_eventNameMap[in_eventType];
	CArtemisSocket::Send(packet);
}

void CArtemis::Query(
	std::function<void(CArtemisState)> in_callback
)
{
	PushOntoQueryQueue(in_callback);
	Json::Value packet;
	packet["type"] = "query";
	CArtemisSocket::Send(packet);
}

CArtemisState CArtemis::QueryWait()
{
	m_bQueryWait = true;
	CArtemis::Query(CArtemis::QueryUnlock);
	while (m_bQueryWait) {
		Sleep(1);
	}
	return m_queryWaitData;
}

void CArtemis::QueryUnlock(
	CArtemisState in_value
)
{
	m_bQueryWait = false;
	m_queryWaitData = in_value;
}

CArtemisState CArtemis::Wait(
	const EEventType& in_eventType,
	const std::string& in_componentName
)
{
	m_bGenericWait = true;
	CArtemis::On(in_eventType, in_componentName, WaitUnlock);
	while (m_bGenericWait) {
		Sleep(1);
	}
	return CArtemisState(m_genericWaitData);
}

void CArtemis::WaitUnlock(
	CArtemisState in_data
)
{
	m_bGenericWait = false;
	m_genericWaitData = in_data;
}

void CArtemis::Update(
	const std::string& in_componentName,
	const std::string& in_componentValue
)
{
	Json::Value packet;
	packet["type"] = "update";
	packet["name"] = in_componentName;
	packet["value"] = in_componentValue;
	CArtemisSocket::Send(packet);
}
