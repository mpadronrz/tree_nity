#pragma once

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "server/inc/Message.hpp"
#include "data_structures/HashMap.hpp"
#include "data_structures/PrefixTree.hpp"

struct ActiveSubscriber
{
	std::string	client_id;
	std::string	prefix;
	std::string	fifo_path;
	int			fifo_fd = -1;
	uint32_t	next_offset = 0;
};

class Topic
{
	private:
		std::string								name;
		std::vector<Message>					messages;

		PrefixTree								prefix_tree;
		HashMap<std::string, ActiveSubscriber>	subscribers;

		std::thread								worker_thread;	// Un hilo dedicado por topic
		std::mutex								topic_mutex;	// Mutex para lectura/escritura de mensajes y suscriptores
		std::condition_variable					cv;				// Para notificar la llegada de nuevos mensajes
		bool									stop_flag;

		// Función privada que ejecutará el hilo dedicado en segundo plano
		void	run();

	public:
		explicit	Topic(const std::string& topic_name);
		~Topic();

		// Guardar un mensaje nuevo en la cola
		void					append_message(const std::string& key, const std::string& value);

		// Obtener mensajes a partir de un offset
		std::vector<Message>	get_messages_from(uint32_t start_offset);

		// Gestión de suscriptores del topic
		bool	add_subscriber(const std::string& client_id, const std::string& prefix, const std::string& fifo_path, uint32_t start_offset);
		bool	remove_subscriber(const std::string& client_id);

		// Apagado del topic notificando mensaje centinela a los suscriptores
		void	shutdown();
};
