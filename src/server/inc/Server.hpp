#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>

#include "data_structures/HashMap.hpp"
#include "protocol/Protocol.hpp"
#include "server/inc/Topic.hpp"

struct ClientMetadata
{
	std::string	client_id;
	std::string	topic;
	uint32_t	offset = 0;
	std::string	prefix;
	std::string	ipc_path;
	bool		is_active = false;
};

class Server
{
	private:
		std::string										ipc_path;

		// Mapa de topics activos: la clave es el nombre (ej. "user_events")
		HashMap<std::string, std::unique_ptr<Topic>>	topics;

		// Índice de metadatos de clientes (HashMap propio obligatorio)
		HashMap<std::string, ClientMetadata>			client_index;

		// Asocia el PID de un proceso productor con su topic conectado
		HashMap<uint32_t, std::string>					producer_topics;

		// Protege la creación/consulta de topics y metadatos de clientes
		std::mutex										server_mutex;

		// Controla el bucle del servidor
		std::atomic<bool>								running{true};

		// Envío de respuestas síncronas al FIFO del cliente (/tmp/treenity.client.CLIENT_PID)
		void	send_response(uint32_t client_pid, Protocol::StatusCode status, std::string_view payload = "");

		// Manejo de peticiones de protocolo binario (cabecera de 5 bytes)
		void	handle_request(uint8_t action, const uint8_t* payload, size_t payload_len);
		void	handle_create(const uint8_t* payload, size_t len);
		void	handle_list(const uint8_t* payload, size_t len);
		void	handle_info(const uint8_t* payload, size_t len);
		void	handle_producer_connect(const uint8_t* payload, size_t len);
		void	handle_publish(const uint8_t* payload, size_t len);
		void	handle_subscriber_connect(const uint8_t* payload, size_t len);
		void	handle_commit(const uint8_t* payload, size_t len);
		void	handle_disconnect(const uint8_t* payload, size_t len);

	public:
		explicit	Server(const std::string& path);
		~Server();

		void	run();   // Bucle principal de eventos
		void	stop();  // Para detener el bucle desde la señal

		// Métodos de gestión de topics
		bool	create_topic(const std::string& topic_name);
		Topic*	get_topic(const std::string& topic_name);
};
