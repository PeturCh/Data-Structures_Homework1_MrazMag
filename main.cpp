#include <iostream>
#include "implementation.hpp"

int main()
{
    MyStore ms(true);
    int workers, clientsCount, arrivalTime, bananas, schweppes, waitingTime;
    std::cout << "Input workers count: ";
    std::cin >> workers;
    std::cout << "Input clients count: ";
    std::cin >> clientsCount;
    Client *clients = new Client[clientsCount];
    ms.init(workers, 0, 0);
    
    for (size_t i = 0; i < clientsCount; i++)
    {
        std::cin >> arrivalTime >> bananas >> schweppes >> waitingTime;
        clients[i] = Client{arrivalTime, bananas, schweppes, waitingTime};
    }
    
    ms.addClients(clients, clientsCount);
    ms.advanceTo(clients[clientsCount-1].maxWaitTime + clients[clientsCount-1].arriveMinute);
    delete []clients;
}
