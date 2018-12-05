#include "server.h"

Server::Server(QObject *parent, int port) :QTcpServer(parent)
{
	listen(QHostAddress::LocalHost, port);
}

void Server::incomingConnection(qintptr socketDescriptor)
//void Server::incomingConnection(int socketDescriptor)
{
	TcpClientSocket *tcpClientSocket = new TcpClientSocket(this);
	//(a)
	connect(tcpClientSocket, SIGNAL(updateClients(QString, int, int)),
		this, SLOT(updateClients(QString, int, int)));			//(b)
	connect(tcpClientSocket, SIGNAL(disconnected(int)), this,
		SLOT(slotDisconnected(int)));					//(c)
	tcpClientSocket->setSocketDescriptor(socketDescriptor);
	//(d)
	tcpClientSocketList.append(tcpClientSocket);			//(e)

}

void Server::updateClients(QString msg, int length, int descriptor)
{
	emit updateServer(msg, length, descriptor);                          //(a)

	/*
	for (int i = 0; i<tcpClientSocketList.count(); i++)          //(b)
	{
		QTcpSocket *item = tcpClientSocketList.at(i);
	//	if (item->write(msg.toLatin1(), length) != length)
		{
			continue;
		}
	}
	*/
}

void Server::slotDisconnected(int descriptor)
{
	for (int i = 0; i<tcpClientSocketList.count(); i++)
	{
		QTcpSocket *item = tcpClientSocketList.at(i);
		if (item->socketDescriptor() == descriptor)
		{
			tcpClientSocketList.removeAt(i);
			return;
		}
	}
	return;
}
