#include <json/json.h>

#include "CArtemisSocket.h"
#include "WebsocketServer.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <asio/io_service.hpp>


// Receive handler
std::vector<std::string> CArtemisSocket::m_handlerNames;
std::map<std::string, std::function<void(Json::Value)>> CArtemisSocket::m_receiveHandlerMap;

// Message queue
std::mutex CArtemisSocket::m_messageQueueMutex;
std::queue<Json::Value> CArtemisSocket::m_messageQueue;

// Store if connection initialized
bool CArtemisSocket::m_bConnectionStarted = false;

// Receive handler
void CArtemisSocket::RegisterMessageHandler(
	std::string m_handlerName,
	std::function<void(Json::Value)> in_handler
)
{
	m_handlerNames.push_back(m_handlerName);
	m_receiveHandlerMap[m_handlerName] = in_handler;
}

// Run socket on main thread
void CArtemisSocket::Run() {

	//Create the event loop for the main thread, and the WebSocket server
	asio::io_service mainEventLoop;
	WebsocketServer server;

	// Register callback on connect, ensuring the logic is run on the main thread's event loop
	server.connect([&mainEventLoop, &server](ClientConnection conn)
		{
			mainEventLoop.post([conn, &server]()
				{
					std::clog << "Connection opened." << std::endl;
					std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;

					//Send a hello message to the client
					server.sendMessage(conn, "hello", Json::Value());
				});
		});

	// Register callback on disconnect, ensuring the logic is run on the main thread's event loop
	server.disconnect([&mainEventLoop, &server](ClientConnection conn)
		{
			mainEventLoop.post([conn, &server]()
				{
					std::clog << "Connection closed." << std::endl;
					std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;
				});
		});

	/*
	// Register callback on ping message, ensuring the logic is run on the main thread's event loop
	server.message("ping", [&mainEventLoop, &server](ClientConnection conn, const Json::Value& args)
	{
		mainEventLoop.post([conn, args, &server]()
		{
			std::clog << "message handler on the main thread" << std::endl;
			std::clog << "Message payload:" << std::endl;
			for (auto key : args.getMemberNames()) {
				std::clog << "\t" << key << ": " << args[key].asString() << std::endl;
			}

			//Echo the message pack to the client
			server.sendMessage(conn, "message", args);
		});
	});
	*/

	// Register ping callback
	server.message("ping", [&mainEventLoop, &server](ClientConnection conn, const Json::Value& args)
		{
			mainEventLoop.post([conn, args, &server]()
				{
					// Indicate connection started
					m_bConnectionStarted = true;

					// Respond ping
					if (m_bPrintPing) {
						std::clog << "[Browser] Ping" << std::endl;
					}
					server.sendMessage(conn, "ping", args);
				});
		});

	// Register additional callbacks, ensuring the logic is run on the main thread's event loop
	for (int i = 0; i < m_handlerNames.size(); i++) {
		std::string& messageType = m_handlerNames[i];
		server.message(messageType, [&messageType, &mainEventLoop, &server](ClientConnection conn, const Json::Value& args)
			{
				mainEventLoop.post([conn, args, &messageType, &server]()
					{
						m_receiveHandlerMap[messageType](args);
					});
			});
	}


	//Start the networking thread
	std::thread serverThread([&server]() {
		server.run(m_portNumber);
		});

	// Main server function
	std::thread mainThread([&server, &mainEventLoop]()
		{

			// Wait for connection to come online
			while (!m_bConnectionStarted) {
				Sleep(10);
			}
			std::clog << "[Client] Connection alive" << std::endl;

			// Main thread
			string input;
			while (1)
			{
				// Broacast message if there is one
				if (!CArtemisSocket::IsMessageQueueEmpty()) {
					const Json::Value message = CArtemisSocket::DequeueMessage();
					assert(message.isMember("type"));
					server.broadcastMessage(message["type"].asString(), message);
				}

				Sleep(1);
			}
		});

	//Start the event loop for the main thread
	asio::io_service::work work(mainEventLoop);
	mainEventLoop.run();
}

// Run socket on new thread
void CArtemisSocket::RunOnNewThread() {

	// Launch run on new thread
	std::thread runThread([&]() {
		CArtemisSocket::Run();
		});
	runThread.detach();
}

// Check if heartbeat is alive
bool CArtemisSocket::HasConnectionStarted() {
	return m_bConnectionStarted;
}

// Send message
void CArtemisSocket::Send(const Json::Value& in_message)
{
	CArtemisSocket::EnqueueMessage(in_message);
}

// Enqueue message in message queue
void CArtemisSocket::EnqueueMessage(const Json::Value& in_value)
{
	std::lock_guard<std::mutex> lock(m_messageQueueMutex);
	m_messageQueue.push(in_value);
}

// Dequeue message from message queue
Json::Value CArtemisSocket::DequeueMessage() {
	std::lock_guard<std::mutex> lock(m_messageQueueMutex);
	assert(!m_messageQueue.empty());
	Json::Value message = m_messageQueue.front();
	m_messageQueue.pop();
	return message;
}

// Check if message queue is empty
bool CArtemisSocket::IsMessageQueueEmpty()
{
	std::lock_guard<std::mutex> lock(m_messageQueueMutex);
	return m_messageQueue.empty();
}
