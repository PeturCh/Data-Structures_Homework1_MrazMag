#include <iostream>
#include "implementation.cpp"

int main()
{
    MyStore ms(true);
    size_t workers, clientsCount, arrivalTime, bananas, schweppes, waitingTime;
    std::cin >> workers >> clientsCount;
    MyClient *clients = new MyClient[clientsCount];
    ms.init(workers, 0, 0);
    
    for (size_t i = 0; i < clientsCount; i++)
    {
        std::cin >> arrivalTime >> bananas >> schweppes >> waitingTime;
        clients[i] = MyClient(arrivalTime, bananas, schweppes, waitingTime);
    }
    
    ms.addClients(clients, clientsCount);
    ms.advanceTo(clients[clientsCount-1].maxWaitTime + clients[clientsCount-1].arriveMinute + 61);
    delete []clients;
}
