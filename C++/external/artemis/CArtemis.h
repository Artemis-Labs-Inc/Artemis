#pragma once
#include <functional>
#include <map>
#include <queue>
#include <mutex>
#include <map>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <conio.h>
#include <iostream>


namespace Json {
	class Value;
}


struct CArtemisState {

	// State
	std::map<std::string, std::string> state;

	// Constructor
	CArtemisState() = default;
	CArtemisState(
		Json::Value in_stateJSON
	);

	// Print
	friend std::ostream& operator<< (std::ostream& out, const CArtemisState& data);		
};

class CArtemis {

private:

	// Version
	static const char* VERSION;

	// Generic wait
	static bool m_bGenericWait;
	static CArtemisState m_genericWaitData;

	// Query wait
	static bool m_bQueryWait;
	static CArtemisState m_queryWaitData;
	static void QueryUnlock(
		CArtemisState in_value
	);

	// Query handler
	static void QueryHandler(
		Json::Value in_message
	);
	static void PushOntoQueryQueue(
		std::function<void(Json::Value)> in_callback
	);
	static std::function<void(Json::Value)> PopFromQueryQueue();

	// Ping handler
	static void PingHandler(
		Json::Value in_message
	);

	// Callback handler
	static void CallbackHandler(
		Json::Value in_message
	);
	static void SetCallback(
		const std::string& in_callbackName,
		std::function<void(Json::Value)> in_callback
	);
	static std::function<void(Json::Value)> GetCallback(
		const std::string& in_callbackName
	);

	// Get local file path
	static std::string GetLocalFilePath();

public:
	

	// Event types
	enum class EEventType {
		Click,
		DoubleClick,
		MouseEnter,
		MouseLeave,
		KeyPress,
		KeyDown,
		KeyUp,
		Focus,
		Input
	};

	// Print version
	static void PrintVersion();

	// Query queue
	static std::mutex m_queryQueueMutex;
	static std::queue<std::function<void(Json::Value)>> m_queryQueue;

	// Callback queue
	static std::mutex m_callbackMapMutex;
	static std::map<std::string, std::function<void(Json::Value)>> m_callbackMap;

	// Event names
	static std::map<EEventType, std::string> m_eventNameMap;


	// Initialize Artemis and launch GUI
	static void Initialize();

	// Navigate to page in GUI
	static void Navigate(
		const std::string& in_pageName
	);

	// Set event listener
	static void On(
		const EEventType& in_eventType,
		const std::string& in_componentName,
		std::function<void(CArtemisState)> in_callback
	);

	// Wait for event synchronously
	static CArtemisState Wait(
		const EEventType& in_eventType,
		const std::string& in_componentName
	);

	static void WaitUnlock(
		CArtemisState in_data
	);

	// Query GUI state
	static void Query(
		std::function<void(CArtemisState)> in_callback
	);

	// Query GUI synchronously
	static CArtemisState QueryWait();

	// Update GUI state
	static void Update(
		const std::string& in_componentName,
		const std::string& in_componentValue
	);

	// Load image into base 64 format
	static std::string LoadImage(
		const std::string& in_filePath
	);
};