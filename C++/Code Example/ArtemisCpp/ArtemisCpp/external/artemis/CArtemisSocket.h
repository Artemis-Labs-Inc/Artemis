#pragma once
#include <mutex>
#include <queue>
#include <json/json.h>
#include <functional>

class CArtemisSocket {

private:

	// Settings
	static const bool m_bPrintPing = false;

	//The port number the WebSocket server listens on
	static const int m_portNumber = 5678;

	// Store if connection initialized
	static bool m_bConnectionStarted;

	// Message queue
	static std::mutex m_messageQueueMutex;
	static std::queue<Json::Value> m_messageQueue;

	// Enqueue message in message queue
	static void EnqueueMessage(const Json::Value& in_value);

	// Dequeue message from message queue
	static Json::Value DequeueMessage();

	// Check if message queue is empty
	static bool IsMessageQueueEmpty();

	// Receive handler
	static std::vector<std::string> m_handlerNames;
	static std::map<std::string, std::function<void(Json::Value)>> m_receiveHandlerMap;

public:

	// Initialize Artemis socket receive handlers
	static void RegisterMessageHandler(
		std::string in_handlerName,
		std::function<void(Json::Value)> in_handler
	);

	// Run Artemis socket on main thread
	static void Run();

	// Runs Artemis socket on new thread
	static void RunOnNewThread();

	// Check if ping connection has started
	static bool HasConnectionStarted();

	// Receive handler


	// Send message
	static void Send(const Json::Value& in_message);
};